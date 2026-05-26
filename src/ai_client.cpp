#include "ai_client.h"
#include <cstdio>
#include <array>
#include <memory>
#include <sstream>
#include <fstream>

AIClient::AIClient() : sessionId_("default")
{
    // 优先使用虚拟环境中的 Python
    pythonPath_ = "ai/venv/bin/python";
    pythonScriptPath_ = "ai/ai_service.py";

    std::ifstream pfile(pythonPath_);
    std::ifstream sfile(pythonScriptPath_);

    if (!pfile.good() || !sfile.good())
    {
        // 尝试从 build 目录运行时的相对路径
        pythonPath_ = "../ai/venv/bin/python";
        pythonScriptPath_ = "../ai/ai_service.py";
    }
}

AIClient::~AIClient() {}

std::string AIClient::escapeString(const std::string &str)
{
    std::ostringstream escaped;
    for (char c : str)
    {
        switch (c)
        {
        case '"':
            escaped << "\\\"";
            break;
        case '\\':
            escaped << "\\\\";
            break;
        case '\n':
            escaped << "\\n";
            break;
        case '\r':
            escaped << "\\r";
            break;
        case '\t':
            escaped << "\\t";
            break;
        default:
            escaped << c;
        }
    }
    return escaped.str();
}

std::string AIClient::executePython(const std::string &args)
{
    std::string command = pythonPath_ + " " + pythonScriptPath_ + " " + args;

    std::array<char, 4096> buffer;
    std::string result;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(
        popen(command.c_str(), "r"),
        pclose);

    if (!pipe)
    {
        return "错误：无法执行 AI 服务";
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }

    if (!result.empty() && result.back() == '\n')
    {
        result.pop_back();
    }

    return result;
}

std::string AIClient::ask(const std::string &message,
                          const std::string &codeContext,
                          const std::string &problemContext)
{
    std::ostringstream args;
    args << "--session " << sessionId_;
    args << " --message \"" << escapeString(message) << "\"";

    if (!codeContext.empty())
    {
        args << " --code \"" << escapeString(codeContext) << "\"";
    }

    if (!problemContext.empty())
    {
        args << " --problem \"" << escapeString(problemContext) << "\"";
    }

    std::string result = executePython(args.str());

    // 如果结果为空，返回错误信息
    if (result.empty())
    {
        return "错误：AI 返回空响应，请检查网络连接或 API Key 配置";
    }

    return result;
}

bool AIClient::isAvailable()
{
    std::ifstream pfile(pythonPath_);
    std::ifstream sfile(pythonScriptPath_);
    if (!pfile.good() || !sfile.good())
    {
        return false;
    }
    return true;
}
