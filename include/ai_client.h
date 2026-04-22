#ifndef AI_CLIENT_H
#define AI_CLIENT_H
#include "../include/db_manager.h"
#include <string>
using namespace std;

class AIClient
{
public:
    AIClient();
    ~AIClient();

    // 设置数据库管理器（用于查询题目信息）
    void setDatabaseManager(DatabaseManager *db);

    string ask(const string &message,
               const string &codeContext = "",
               const string &problemContext = "");

    /**
     * @brief 自动查询题目上下文并调用 AI
     * @param message 用户问题
     * @param codeContext 代码上下文
     * @param problemId 题目ID
     * @param errorContext 评测错误上下文（可选）
     * @return AI 回答
     */
    string askWithProblemContext(const string &message,
                                 const string &codeContext,
                                 int problemId,
                                 const string &errorContext = "");

    bool isAvailable();

private:
    string sessionId_; // 会话 ID，用于保持上下文
    string pythonPath_;
    string pythonScriptPath_;
    DatabaseManager *db_manager_ = nullptr;

    string executePython(const string &args);
    string escapeString(const string &str);

    string queryProblemInfo(int problemId);
    string queryProblemList();
};

#endif // AI_CLIENT_H
