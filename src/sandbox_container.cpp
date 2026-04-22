#include "../include/sandbox_container.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <sys/wait.h>

using namespace std;

// Shell 工具方法

string SandboxContainer::runShell(const string &cmd)
{
    string output;
    runShellWithCode(cmd, output);
    return output;
}

int SandboxContainer::runShellWithCode(const string &cmd, string &output)
{
    output.clear();
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return -1;

    char buf[256];
    while (fgets(buf, sizeof(buf), pipe))
        output += buf;

    int status = pclose(pipe);
    return WEXITSTATUS(status); // WEXITSTATUS 宏从状态码中提取实际的退出码 , 0 表示命令成功执行，非 0 表示发生错误
}

int SandboxContainer::execInContainer(const string &cmd, string &output) const
{
    // docker exec 运行命令，捕获输出和退出码 , sh -c 用于执行复杂命令，2>&1 将 stderr 重定向到 stdout 以捕获所有输出
    string full = "docker exec " + container_id_ +
                  " sh -c \"" + cmd + "\" 2>&1";
    return runShellWithCode(full, output);
}

bool SandboxContainer::copyTextToContainer(const string &content,
                                           const string &container_path) const
{
    // 写到宿主机临时文件，通过 docker exec + stdin 管道写入容器
    // （docker cp 在 --read-only 容器上会失败）
    string tmp = "/tmp/oj_sc_" + container_id_.substr(0, 8) + ".tmp";
    ofstream f(tmp);
    if (!f.is_open())
        return false;
    f << content;
    f.close();

    string cmd = "docker exec -i " + container_id_ +
                 " sh -c 'cat > " + container_path + "' < " + tmp;
    string out;
    int code = runShellWithCode(cmd, out);
    remove(tmp.c_str()); // 删除临时文件
    return code == 0;
}

// 容器生命周期
bool SandboxContainer::start(const string &image)
{
    // 常驻模式：sleep infinity 保持容器存活
    string cmd =
        "docker run -d "
        "--network none "                            // 禁止网络访问
        "--memory=256m "                             // 内存上限（评测时再精确控制）
        "--pids-limit=64 "                           // 进程数上限
        "--cap-drop=ALL "                            // 丢弃所有 capabilities
        "--read-only "                               // 只读根文件系统
        "--tmpfs /sandbox:exec,size=128m,mode=1777 " // 沙箱目录：内存文件系统可执行，所有用户可写
        + image + " sleep infinity 2>&1";

    string output;
    int code = runShellWithCode(cmd, output);
    if (code != 0)
    {
        cerr << "[SandboxContainer] 启动失败: " << output << endl;
        return false;
    }

    // 去除末尾换行，得到容器 ID
    container_id_ = output;
    while (!container_id_.empty() &&
           (container_id_.back() == '\n' || container_id_.back() == '\r'))
        container_id_.pop_back();

    state_ = ContainerState::IDLE;
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
    string out = runShell(
        "docker inspect -f '{{.State.Running}}' " + container_id_ + " 2>/dev/null");
    return out.find("true") != string::npos;
}

// 评测操作

bool SandboxContainer::writeSourceCode(const string &source_code)
{
    return copyTextToContainer(source_code, "/sandbox/main.cpp");
}

bool SandboxContainer::compile(string &error_output)
{
    string out;
    int code = execInContainer(
        "g++ -O2 -std=c++17 /sandbox/main.cpp -o /sandbox/program 2>&1", out);
    error_output = out;
    return code == 0;
}

JudgeResult SandboxContainer::run(const string &input,
                                  int time_limit_ms,
                                  int memory_limit_mb,
                                  string &output,
                                  int &time_used_ms,
                                  int &memory_used_mb)
{
    // 写入标准输入
    copyTextToContainer(input, "/sandbox/input.txt");

    float time_sec = time_limit_ms / 1000.0f;
    string cmd = // 使用 timeout 和 /usr/bin/time 来限制时间和监控资源 , timeout 命令会在程序运行超过指定时间后强制终止，并返回 124 的退出码 , /usr/bin/time 的 -f 选项指定输出格式，这里输出了实际耗时（秒）和峰值内存（KB），分别写入 time.txt 文件中供后续读取
        "timeout " + to_string(time_sec) +
        " /usr/bin/time -f '%e %M'"
        " /sandbox/program < /sandbox/input.txt"
        " > /sandbox/output.txt 2>/sandbox/time.txt ;"
        " echo EXIT_CODE:$?";

    string exec_out;
    execInContainer(cmd, exec_out);

    // 解析退出码
    int exit_code = -1;
    size_t pos = exec_out.find("EXIT_CODE:");
    if (pos != string::npos)
        exit_code = stoi(exec_out.substr(pos + 10));

    // 读取程序输出
    string cat_out;
    execInContainer("cat /sandbox/output.txt 2>/dev/null", cat_out);
    output = cat_out;

    // 读取 time/memory 统计（格式：elapsed_sec peak_kb）
    string time_raw;
    execInContainer("cat /sandbox/time.txt 2>/dev/null", time_raw);
    float elapsed_sec = 0.0f;
    long peak_kb = 0;
    if (!time_raw.empty()) // 解析 time.txt 中的时间和内存数据 , sscanf 函数从 time_raw 字符串中提取出实际耗时（秒）和峰值内存（KB），并分别存储在 elapsed_sec 和 peak_kb 变量中
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
    string out;
    int code = execInContainer("rm -rf /sandbox/*", out);
    state_ = ContainerState::IDLE;
    return code == 0;
}
