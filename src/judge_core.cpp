#include "../include/judge_core.h"
#include "../include/container_pool.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

// ============================================================
// JudgeCore::Impl 定义
// ============================================================

class JudgeCore::Impl
{
public:
    JudgeConfig config_;
    std::string source_code_;
    std::string test_data_path_;
    std::string work_dir_;
    SecurityConfig security_;
    JudgeReport last_report_;
    ContainerPool pool_;
    bool pool_ready_ = false;

    /**
     * @brief 惰性初始化容器池（第一次调用 judge() 时触发）
     * 常驻 1 个容器，最多扩容到 4 个。
     */
    bool ensurePool()
    {
        if (pool_ready_)
            return true;
        pool_ready_ = pool_.initialize(1, 4);
        return pool_ready_;
    }

    /**
     * @brief 将 JudgeResult 转为数据库 status 字段字符串
     */
    static std::string resultToString(JudgeResult r)
    {
        switch (r)
        {
        case JudgeResult::ACCEPTED:
            return "AC";
        case JudgeResult::WRONG_ANSWER:
            return "WA";
        case JudgeResult::TIME_LIMIT_EXCEEDED:
            return "TLE";
        case JudgeResult::MEMORY_LIMIT_EXCEEDED:
            return "MLE";
        case JudgeResult::RUNTIME_ERROR:
            return "RE";
        case JudgeResult::COMPILE_ERROR:
            return "CE";
        case JudgeResult::SYSTEM_ERROR:
            return "SE";
        default:
            return "Pending";
        }
    }

    // ---- 测试数据 ----

    struct TestCase
    {
        std::string input;
        std::string expected;
    };

    /**
     * @brief 读取测试数据目录下所有 1.in/1.out ... N.in/N.out 文件对
     */
    std::vector<TestCase> loadTestCases()
    {
        std::vector<TestCase> cases;
        if (test_data_path_.empty())
            return cases;

        for (int i = 1; i <= 100; ++i)
        {
            std::string in_path = test_data_path_ + "/" + std::to_string(i) + ".in";
            std::string out_path = test_data_path_ + "/" + std::to_string(i) + ".out";

            std::ifstream fin(in_path), fout(out_path);
            if (!fin.is_open() || !fout.is_open())
                break;

            std::stringstream sin, sout;
            sin << fin.rdbuf();
            sout << fout.rdbuf();
            cases.push_back({sin.str(), sout.str()});
        }
        return cases;
    }

    /**
     * @brief 比对程序输出与期望输出（忽略行尾空白）
     */
    static bool compareOutput(const std::string &actual, const std::string &expected)
    {
        auto trim = [](std::string s)
        {
            while (!s.empty() &&
                   (s.back() == ' ' || s.back() == '\n' || s.back() == '\r'))
                s.pop_back();
            return s;
        };
        return trim(actual) == trim(expected);
    }
};

// ============================================================
// JudgeCore 公共接口实现
// ============================================================

JudgeCore::JudgeCore() : impl_(std::make_unique<Impl>()) {}
JudgeCore::~JudgeCore() = default;

void JudgeCore::setConfig(const JudgeConfig &config) { impl_->config_ = config; }
void JudgeCore::setSourceCode(const std::string &src) { impl_->source_code_ = src; }
void JudgeCore::setTestDataPath(const std::string &path) { impl_->test_data_path_ = path; }
void JudgeCore::setWorkDirectory(const std::string &dir) { impl_->work_dir_ = dir; }
void JudgeCore::setSecurityConfig(const SecurityConfig &s) { impl_->security_ = s; }
JudgeReport JudgeCore::getLastReport() const { return impl_->last_report_; }

JudgeReport JudgeCore::judge()
{
    JudgeReport report{};
    report.result = JudgeResult::SYSTEM_ERROR;

    // 1. 确保容器池就绪
    if (!impl_->ensurePool())
    {
        report.error_message = "容器池初始化失败，请检查 Docker 是否可用";
        impl_->last_report_ = report;
        return report;
    }

    // 2. 获取可用容器（优先常驻，否则临时）
    bool is_temporary = false;
    auto container = impl_->pool_.acquire(is_temporary);
    if (!container)
    {
        report.error_message = "无可用容器，系统繁忙";
        impl_->last_report_ = report;
        return report;
    }

    // 3. 写入源代码
    if (!container->writeSourceCode(impl_->source_code_))
    {
        report.error_message = "写入源代码失败";
        impl_->pool_.release(container);
        impl_->last_report_ = report;
        return report;
    }

    // 4. 编译
    std::string compile_error;
    report.result = JudgeResult::COMPILING;
    if (!container->compile(compile_error))
    {
        report.result = JudgeResult::COMPILE_ERROR;
        report.error_message = compile_error;
        if (is_temporary)
            impl_->pool_.destroyTemporary(container);
        else
            impl_->pool_.release(container);
        impl_->last_report_ = report;
        return report;
    }

    // 5. 加载测试数据
    auto test_cases = impl_->loadTestCases();
    if (test_cases.empty())
    {
        report.result = JudgeResult::SYSTEM_ERROR;
        report.error_message = "未找到测试数据（路径: " + impl_->test_data_path_ + "）";
        if (is_temporary)
            impl_->pool_.destroyTemporary(container);
        else
            impl_->pool_.release(container);
        impl_->last_report_ = report;
        return report;
    }

    // 6. 逐点测试
    report.result = JudgeResult::ACCEPTED;
    report.total_test_cases = static_cast<int>(test_cases.size());
    report.passed_test_cases = 0;
    report.time_used_ms = 0;
    report.memory_used_mb = 0;

    for (int i = 0; i < static_cast<int>(test_cases.size()); ++i)
    {
        const auto &tc = test_cases[i];
        std::string actual_output;
        int time_ms = 0, memory_mb = 0;

        JudgeResult tc_result = container->run(
            tc.input,
            impl_->config_.time_limit_ms,
            impl_->config_.memory_limit_mb,
            actual_output,
            time_ms,
            memory_mb);

        // 运行通过时比对输出
        if (tc_result == JudgeResult::ACCEPTED)
        {
            if (!Impl::compareOutput(actual_output, tc.expected))
                tc_result = JudgeResult::WRONG_ANSWER;
        }

        // 记录测试点详情
        TestCaseResult detail;
        detail.case_id = i + 1;
        detail.result = tc_result;
        detail.time_ms = time_ms;
        detail.memory_mb = memory_mb;
        if (tc_result == JudgeResult::WRONG_ANSWER)
            detail.output_diff = "期望: " + tc.expected.substr(0, 100) +
                                 "\n实际: " + actual_output.substr(0, 100);
        report.details.push_back(detail);

        // 更新最大资源使用
        report.time_used_ms = std::max(report.time_used_ms, time_ms);
        report.memory_used_mb = std::max(report.memory_used_mb, memory_mb);

        if (tc_result == JudgeResult::ACCEPTED)
        {
            ++report.passed_test_cases;
        }
        else
        {
            report.result = tc_result;
            break; // 遇到第一个失败点即停止
        }
    }

    // 7. 归还或销毁容器
    if (is_temporary)
        impl_->pool_.destroyTemporary(container);
    else
        impl_->pool_.release(container);

    impl_->last_report_ = report;
    return report;
}

void JudgeCore::saveResult(const JudgeReport &report, int submission_id)
{
    // TODO: 通过 DatabaseManager 写入 submissions 表
    std::cout << "[JudgeCore] 保存结果: submission=" << submission_id
              << " status=" << Impl::resultToString(report.result)
              << " time=" << report.time_used_ms << "ms"
              << " memory=" << report.memory_used_mb << "MB" << std::endl;
}

void JudgeCore::cleanup()
{
    impl_->pool_.healthCheck();
}
