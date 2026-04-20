#include "../include/sandbox_container.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <sys/wait.h>

// ----------------------------------------------------------------
// Shell 工具方法
// ----------------------------------------------------------------

std::string SandboxContainer::runShell(const std::string &cmd)
{
    std::string output;
    runShellWithCode(cmd, output);
    return output;
}

int SandboxContainer::runShellWithCode(const std::string &cmd, std::string &output)
{
    output.clear();
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return -1;

    char buf[256];
    while (fgets(buf, sizeof(buf), pipe))
        output += buf;

    int status = pclose(pipe);
    return WEXITSTATUS(status);
}

int SandboxContainer::execInContainer(const std::string &cmd, std::string &output) const
{
    std::string full = "docker exec " + container_id_ +
                       " sh -c \"" + cmd + "\" 2>&1";
    return runShellWithCode(full, output);
}

bool SandboxContainer::copyTextToContainer(const std::string &content,
                                           const std::string &container_path) const
{
    // 写到宿主机临时文件，再 docker cp 进容器
    std::string tmp = "/tmp/oj_sc_" + container_id_.substr(0, 8) + ".tmp";
    std::ofstream f(tmp);
    if (!f.is_open())
        return false;
    f << content;
    f.close();

    std::string cmd = "docker cp " + tmp + " " + container_id_ + ":" + container_path;
    std::string out;
    int code = runShellWithCode(cmd, out);
    std::remove(tmp.c_str());
    return code == 0;
}

// ----------------------------------------------------------------
// 容器生命周期
// ----------------------------------------------------------------

bool SandboxContainer::start(const std::string &image)
{
    // 常驻模式：sleep infinity 保持容器存活
    std::string cmd =
        "docker run -d "
        "--network none "                  // 禁止网络访问
        "--memory=256m "                   // 内存上限（评测时再精确控制）
        "--pids-limit=64 "                 // 进程数上限
        "--cap-drop=ALL "                  // 丢弃所有 capabilities
        "--read-only "                     // 只读根文件系统
        "--tmpfs /sandbox:exec,size=128m " // 沙箱目录：内存文件系统可执行
        + image + " sleep infinity 2>&1";

    std::string output;
    int code = runShellWithCode(cmd, output);
    if (code != 0)
    {
        std::cerr << "[SandboxContainer] 启动失败: " << output << std::endl;
        return false;
    }

    // 去除末尾换行，得到容器 ID
    container_id_ = output;
    while (!container_id_.empty() &&
           (container_id_.back() == '\n' || container_id_.back() == '\r'))
        container_id_.pop_back();

    state_ = ContainerState::IDLE;
    last_used_ = std::chrono::steady_clock::now();
    return true;
}

void SandboxContainer::destroy()
{
    if (container_id_.empty())
        return;
    runShell("docker rm -f " + container_id_ + " >/dev/null 2>&1");
    container_id_.clear();
    state_ = ContainerState::ERROR;
}

bool SandboxContainer::isAlive() const
{
    if (container_id_.empty())
        return false;
    std::string out = runShell(
        "docker inspect -f '{{.State.Running}}' " + container_id_ + " 2>/dev/null");
    return out.find("true") != std::string::npos;
}

// ----------------------------------------------------------------
// 评测操作
// ----------------------------------------------------------------

bool SandboxContainer::writeSourceCode(const std::string &source_code)
{
    return copyTextToContainer(source_code, "/sandbox/main.cpp");
}

bool SandboxContainer::compile(std::string &error_output)
{
    std::string out;
    int code = execInContainer(
        "g++ -O2 -std=c++17 /sandbox/main.cpp -o /sandbox/program 2>&1", out);
    error_output = out;
    return code == 0;
}

JudgeResult SandboxContainer::run(const std::string &input,
                                  int time_limit_ms,
                                  int memory_limit_mb,
                                  std::string &output,
                                  int &time_used_ms,
                                  int &memory_used_mb)
{
    // 写入标准输入
    copyTextToContainer(input, "/sandbox/input.txt");

    float time_sec = time_limit_ms / 1000.0f;
    std::string cmd =
        "timeout " + std::to_string(time_sec) +
        " /usr/bin/time -f '%e %M'"
        " /sandbox/program < /sandbox/input.txt"
        " > /sandbox/output.txt 2>/sandbox/time.txt ;"
        " echo EXIT_CODE:$?";

    std::string exec_out;
    execInContainer(cmd, exec_out);

    // 解析退出码
    int exit_code = -1;
    size_t pos = exec_out.find("EXIT_CODE:");
    if (pos != std::string::npos)
        exit_code = std::stoi(exec_out.substr(pos + 10));

    // 读取程序输出
    std::string cat_out;
    execInContainer("cat /sandbox/output.txt 2>/dev/null", cat_out);
    output = cat_out;

    // 读取 time/memory 统计（格式：elapsed_sec peak_kb）
    std::string time_raw;
    execInContainer("cat /sandbox/time.txt 2>/dev/null", time_raw);
    float elapsed_sec = 0.0f;
    long peak_kb = 0;
    if (!time_raw.empty())
        sscanf(time_raw.c_str(), "%f %ld", &elapsed_sec, &peak_kb);
    time_used_ms = static_cast<int>(elapsed_sec * 1000);
    memory_used_mb = static_cast<int>(peak_kb / 1024);

    // 判断结果
    if (exit_code == 124)
        return JudgeResult::TIME_LIMIT_EXCEEDED;
    if (memory_used_mb > memory_limit_mb)
        return JudgeResult::MEMORY_LIMIT_EXCEEDED;
    if (exit_code != 0)
        return JudgeResult::RUNTIME_ERROR;

    return JudgeResult::ACCEPTED;
}

bool SandboxContainer::reset()
{
    std::string out;
    int code = execInContainer("rm -rf /sandbox/*", out);
    state_ = ContainerState::IDLE;
    last_used_ = std::chrono::steady_clock::now();
    return code == 0;
}
