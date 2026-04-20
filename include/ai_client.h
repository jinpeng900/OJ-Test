#ifndef AI_CLIENT_H
#define AI_CLIENT_H

#include <string>

class AIClient
{
public:
    AIClient();
    ~AIClient();

    std::string ask(const std::string &message,
                    const std::string &codeContext = "",
                    const std::string &problemContext = "");

    bool isAvailable();

private:
    std::string sessionId_;
    std::string pythonPath_;
    std::string pythonScriptPath_;

    std::string executePython(const std::string &args);
    std::string escapeString(const std::string &str);
};

#endif // AI_CLIENT_H
