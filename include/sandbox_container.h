#ifndef SANDBOX_CONTAINER_H
#define SANDBOX_CONTAINER_H

#include "judge_core.h"
#include <string>
using namespace std;

// 容器运行状态
enum class ContainerState
{
    IDLE, // 空闲，可接受新评测任务
    BUSY, // 评测中，不可复用
    ERROR // 异常，需销毁重建
};

/**
 * @brief 沙箱容器封装
 *
 * 封装单个 Docker 容器的生命周期与文件交互。
 * 通过 docker run / docker exec / docker cp 操作容器，无需额外 HTTP 库。
 *
 * 常驻模式：容器以 "sleep infinity" 保持存活，
 * 每次评测通过 docker exec 在容器内编译/运行，
 * 评测完成后调用 reset() 清理临时文件以便复用。
 */
class SandboxContainer
{
public:
    /**
     * @brief 创建并启动容器（常驻模式）
     * @param image Docker 镜像名称，默认 judge-sandbox:latest
     * @return 启动成功返回 true
     */
    bool start(const string &image = "judge-sandbox:latest");

    // 强制停止并销毁容器
    void destroy();

    // 检查容器是否仍在运行
    bool isAlive() const;

    /**
     * @brief 将源代码写入容器 /sandbox/main.cpp
     * @param source_code 源代码字符串
     * @return 写入成功返回 true
     */
    bool writeSourceCode(const string &source_code);

    /**
     * @brief 在容器内编译源代码
     * @param error_output 编译错误信息（输出参数）
     * @return 编译成功返回 true
     */
    bool compile(string &error_output);

    /**
     * @brief 在容器内运行已编译程序
     * @param input            标准输入内容
     * @param time_limit_ms    时间限制（毫秒）
     * @param memory_limit_mb  内存限制（MB）
     * @param output           程序输出（输出参数）
     * @param time_used_ms     实际耗时（输出参数，毫秒）
     * @param memory_used_mb   峰值内存（输出参数，MB）
     * @return 运行结果（AC / TLE / MLE / RE）
     */
    JudgeResult run(const string &input,
                    int time_limit_ms,
                    int memory_limit_mb,
                    string &output,
                    int &time_used_ms,
                    int &memory_used_mb);

    /**
     * @brief 清理 /sandbox/ 目录，重置容器供下次复用
     * @return 清理成功返回 true
     */
    bool reset();

    // ---- 状态访问 ----
    ContainerState getState() const { return state_; }
    void setState(ContainerState s) { state_ = s; }
    string getId() const { return container_id_; }

private:
    string container_id_;
    ContainerState state_ = ContainerState::IDLE;

    /**
     * @brief 在容器内执行 shell 命令，返回退出码
     * @param cmd    容器内的命令字符串
     * @param output 命令的合并输出（stdout + stderr）
     */
    int execInContainer(const string &cmd, string &output) const;

    /**
     * @brief 将字符串内容写入临时文件后通过 docker exec + stdin 管道写入容器
     * @param content        要写入的内容
     * @param container_path 容器内目标路径
     */
    bool copyTextToContainer(const string &content,
                             const string &container_path) const;

    // 执行宿主机 shell 命令并返回输出字符串
    static string runShell(const string &cmd);

    // 执行宿主机 shell 命令，返回退出码，输出通过参数返回
    static int runShellWithCode(const string &cmd, string &output);
};

#endif // SANDBOX_CONTAINER_H
