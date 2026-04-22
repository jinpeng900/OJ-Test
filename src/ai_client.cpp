#include "ai_client.h"
#include "../include/db_manager.h"
#include <cstdio>
#include <array>
#include <memory>
#include <sstream>
#include <fstream>

using namespace std;

AIClient::AIClient() : sessionId_("default"), db_manager_(nullptr)
{
    // 优先使用虚拟环境中的 Python
    pythonPath_ = "ai/venv/bin/python";
    pythonScriptPath_ = "ai/ai_service.py";

    ifstream pfile(pythonPath_);
    ifstream sfile(pythonScriptPath_);

    if (!pfile.good() || !sfile.good())
    {
        // 尝试从 build 目录运行时的相对路径
        pythonPath_ = "../ai/venv/bin/python";
        pythonScriptPath_ = "../ai/ai_service.py";
    }
}

AIClient::~AIClient() {}

void AIClient::setDatabaseManager(DatabaseManager *db)
{
    db_manager_ = db;
}

string AIClient::queryProblemInfo(int problemId)
{
    if (!db_manager_)
        return "题目ID: " + to_string(problemId);

    string sql = "SELECT title, description, time_limit, memory_limit FROM problems WHERE id = " + to_string(problemId);
    auto results = db_manager_->query(sql);

    if (results.empty())
        return "题目ID: " + to_string(problemId);

    string info = "【题目信息】\n";
    info += "题号: " + results[0]["id"] + "\n";
    info += "标题: " + results[0]["title"] + "\n";
    info += "描述: " + results[0]["description"] + "\n";
    info += "时间限制: " + results[0]["time_limit"] + " ms\n";
    info += "内存限制: " + results[0]["memory_limit"] + " MB";
    return info;
}

string AIClient::queryProblemList()
{
    if (!db_manager_)
        return "";

    string sql = "SELECT id, title, category, description FROM problems ORDER BY id";
    auto results = db_manager_->query(sql);

    string list = "【题库列表】\n";
    for (const auto &p : results)
    {
        string desc = p.at("description");
        if (desc.length() > 80)
            desc = desc.substr(0, 80);
        list += "题号:" + p.at("id") + " | 标题:" + p.at("title") + " | 类别:" + p.at("category") + " | 描述:" + desc + "\n";
    }
    return list;
}

string AIClient::askWithProblemContext(const string &message,
                                       const string &codeContext,
                                       int problemId,
                                       const string &errorContext)
{
    // 1. 查询题目信息
    string problemContext = queryProblemInfo(problemId);

    // 2. 附加评测错误上下文
    if (!errorContext.empty())
        problemContext += "\n\n" + errorContext;

    // 3. 调用 AI
    string response = ask(message, codeContext, problemContext);

    // 4. 检测是否需要题库列表，按需补充后重新调用
    if (response.find("[NEED_PROBLEMS]") != string::npos)
    {
        string contextWithList = problemContext + "\n\n" + queryProblemList();
        response = ask(message, codeContext, contextWithList);
    }

    return response;
}

string AIClient::escapeString(const string &str) // 简单转义，适用于命令行参数
{
    ostringstream escaped;
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

string AIClient::executePython(const string &args)
{
    string command = pythonPath_ + " " + pythonScriptPath_ + " " + args;

    array<char, 4096> buffer; // 用于存储命令输出的缓冲区
    string result;

    unique_ptr<FILE, decltype(&pclose)> pipe(
        popen(command.c_str(), "r"),
        pclose); // pipe 是一个 FILE*，需要在结束时关闭 , popen 会执行命令并打开一个管道读取输出

    if (!pipe)
    {
        return "错误：无法执行 AI 服务";
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) // 从管道中读取输出，直到结束
    {
        result += buffer.data();
    }

    if (!result.empty() && result.back() == '\n')
    {
        result.pop_back();
    }

    return result;
}

string AIClient::ask(const string &message,
                     const string &codeContext,
                     const string &problemContext)
{
    ostringstream args;
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

    string result = executePython(args.str());

    // 如果结果为空，返回错误信息
    if (result.empty())
    {
        return "错误： AI 返回空响应，请检查网络连接或 API Key 配置";
    }

    return result;
}

bool AIClient::isAvailable()
{
    ifstream pfile(pythonPath_);
    ifstream sfile(pythonScriptPath_);
    if (!pfile.good() || !sfile.good())
    {
        return false;
    }
    return true;
}
