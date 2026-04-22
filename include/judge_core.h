#ifndef JUDGE_CORE_H
#define JUDGE_CORE_H

#include <string>
#include <vector>
#include <memory>
using namespace std;

// 评测结果枚举
enum class JudgeResult
{
    PENDING,               // 等待评测
    COMPILING,             // 编译中
    RUNNING,               // 运行中
    ACCEPTED,              // 通过 (AC)
    WRONG_ANSWER,          // 答案错误 (WA)
    TIME_LIMIT_EXCEEDED,   // 超时 (TLE)
    MEMORY_LIMIT_EXCEEDED, // 超内存 (MLE)
    RUNTIME_ERROR,         // 运行时错误 (RE)
    COMPILE_ERROR,         // 编译错误 (CE)
    SYSTEM_ERROR           // 系统错误 (SE)
};

// 评测配置结构体
struct JudgeConfig
{
    int time_limit_ms;   // 时间限制 (毫秒)
    int memory_limit_mb; // 内存限制 (MB)
};

// 单个测试点结果
struct TestCaseResult
{
    int case_id;        // 测试点编号
    JudgeResult result; // 该点结果
    int time_ms;        // 时间使用 (ms)
    int memory_mb;      // 内存使用 (MB)
    string output_diff; // 差异信息 (WA 时) , 当评测结果为 WA 时，output_diff 字段可以包含用户输出与标准输出之间的差异信息，帮助用户定位错误原因，例如显示具体的行数和内容差异，或者提供一个 diff 格式的文本，指出哪些部分不匹配。这对于用户调试代码非常有帮助，可以快速找到问题所在。
};

// 评测报告
struct JudgeReport
{
    JudgeResult result;             // 总体评测结果
    int time_used_ms;               // 最大时间使用 (毫秒)
    int memory_used_mb;             // 最大内存使用 (MB)
    string error_message;           // 错误信息 (CE/RE 等)
    int passed_test_cases;          // 通过的测试点数量
    int total_test_cases;           // 总测试点数量
    vector<TestCaseResult> details; // 每个测试点详情
};

/**
 * @brief 评测核心类
 *
 * 基于 Docker 容器化实现安全隔离的代码评测。
 * 支持编译、运行、资源监控和结果比对。
 * 使用 PIMPL 模式隐藏实现细节。
 */
class JudgeCore
{
public:
    JudgeCore();
    ~JudgeCore();

    // ========== 配置接口 ==========

    /**
     * @brief 设置评测配置
     * @param config 评测配置（时间/内存限制、语言等）
     */
    void setConfig(const JudgeConfig &config);

    /**
     * @brief 设置源代码
     * @param source_code 用户提交的源代码
     */
    void setSourceCode(const string &source_code);

    /**
     * @brief 设置测试数据路径
     * @param test_data_path 测试数据文件夹路径（包含 .in/.out 文件）
     */
    void setTestDataPath(const string &test_data_path);

    // ========== 评测接口 ==========

    /**
     * @brief 执行评测（主入口）
     * @return 评测报告
     */
    JudgeReport judge();

private:
    class Impl;
    unique_ptr<Impl> impl_;

    // 禁止拷贝和赋值
    JudgeCore(const JudgeCore &) = delete;
    JudgeCore &operator=(const JudgeCore &) = delete;
};

#endif // JUDGE_CORE_H
