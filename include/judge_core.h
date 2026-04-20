#ifndef JUDGE_CORE_H
#define JUDGE_CORE_H

#include <string>
#include <vector>
#include <memory>

/**
 * @brief 评测结果枚举
 */
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

/**
 * @brief 评测配置结构体
 */
struct JudgeConfig
{
    int time_limit_ms;    // 时间限制 (毫秒)
    int memory_limit_mb;  // 内存限制 (MB)
    int output_limit_mb;  // 输出文件大小限制 (MB)
    std::string language; // 编程语言 (目前仅支持 C++)
};

/**
 * @brief 资源限制结构体
 */
struct ResourceLimits
{
    float cpu_quota = 1.0;    // CPU 核心数
    int cpu_period = 100000;  // 调度周期 (us)
    int memory_limit_mb;      // 内存限制 (MB)
    int memory_swap_mb = 0;   // 禁止交换分区
    int time_limit_ms;        // 时间限制 (毫秒)
    int wall_time_limit_ms;   // 墙上时间限制
    int output_limit_mb = 64; // 输出文件大小限制
    int max_processes = 1;    // 最大进程数
};

/**
 * @brief 安全配置结构体
 */
struct SecurityConfig
{
    bool disable_network = true;                 // 禁止网络访问
    bool read_only_rootfs = true;                // 只读文件系统
    bool no_privileged = true;                   // 禁止特权模式
    std::vector<std::string> cap_drop = {"ALL"}; // 丢弃所有 capabilities
    std::vector<std::string> cap_add = {};       // 添加最小必要 capabilities
    std::string seccomp_profile;                 // Seccomp 配置文件路径
    int max_pids = 50;                           // 最大进程数
    int max_open_files = 64;                     // 最大打开文件数
};

/**
 * @brief 资源使用情况
 */
struct ResourceUsage
{
    int64_t memory_usage_bytes; // 当前内存使用
    int64_t memory_peak_bytes;  // 内存使用峰值
    float cpu_percent;          // CPU 使用率
    int64_t cpu_time_us;        // CPU 时间 (微秒)
    bool oom_killed;            // 是否被 OOM killer 终止
};

/**
 * @brief 单个测试点结果
 */
struct TestCaseResult
{
    int case_id;             // 测试点编号
    JudgeResult result;      // 该点结果
    int time_ms;             // 时间使用 (ms)
    int memory_mb;           // 内存使用 (MB)
    std::string output_diff; // 差异信息 (WA 时)
};

/**
 * @brief 评测报告
 */
struct JudgeReport
{
    JudgeResult result;                  // 总体评测结果
    int time_used_ms;                    // 最大时间使用 (毫秒)
    int memory_used_mb;                  // 最大内存使用 (MB)
    std::string error_message;           // 错误信息 (CE/RE 等)
    int passed_test_cases;               // 通过的测试点数量
    int total_test_cases;                // 总测试点数量
    std::vector<TestCaseResult> details; // 每个测试点详情
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
    void setSourceCode(const std::string &source_code);

    /**
     * @brief 设置测试数据路径
     * @param test_data_path 测试数据文件夹路径（包含 .in/.out 文件）
     */
    void setTestDataPath(const std::string &test_data_path);

    /**
     * @brief 设置工作目录
     * @param work_dir 评测工作目录（用于存放临时文件）
     */
    void setWorkDirectory(const std::string &work_dir);

    /**
     * @brief 设置安全配置
     * @param security 安全配置（网络、文件系统、capabilities）
     */
    void setSecurityConfig(const SecurityConfig &security);

    // ========== 评测接口 ==========

    /**
     * @brief 执行评测（主入口）
     * @return 评测报告
     */
    JudgeReport judge();

    /**
     * @brief 获取最后一次评测结果
     * @return 评测报告
     */
    JudgeReport getLastReport() const;

    // ========== 结果持久化 ==========

    /**
     * @brief 保存评测结果到数据库
     * @param report 评测报告
     * @param submission_id 提交记录ID
     */
    void saveResult(const JudgeReport &report, int submission_id);

    // ========== 资源管理 ==========

    /**
     * @brief 清理工作目录中的临时文件
     */
    void cleanup();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // 禁止拷贝和赋值
    JudgeCore(const JudgeCore &) = delete;
    JudgeCore &operator=(const JudgeCore &) = delete;
};

#endif // JUDGE_CORE_H
