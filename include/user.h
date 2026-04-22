#ifndef USER_H
#define USER_H

#include "db_manager.h"
#include "judge_core.h"
#include <string>
using namespace std;

// 用户类，负责处理普通用户的业务逻辑
class User
{
public:
    // 构造函数：关联一个数据库管理器
    User(DatabaseManager *db);

    /**
     * @brief 用户登录
     * @param account 账号
     * @param password 密码
     * @return 登录成功返回true
     */
    bool login(const string &account, const string &password);

    /**
     * @brief 用户注册
     * @param account 账号
     * @param password 密码
     * @return 注册成功返回true
     */
    bool register_user(const string &account, const string &password);

    /**
     * @brief 修改自己的密码
     * @param old_password 旧密码
     * @param new_password 新密码
     * @return 修改成功返回true
     */
    bool change_my_password(const string &old_password, const string &new_password);

    // 查看所有题目列表
    void list_problems();

    /**
     * @brief 查看单个题目详情
     * @param id 题目 ID
     */
    void view_problem(int id);

    /**
     * @brief 提交代码进行评测
     * @param problem_id 题目 ID
     * @param code 用户提交的代码
     */
    void submit_code(int problem_id, const string &code);

    // 查看个人提交记录
    void view_my_submissions();

    // 获取最近一次评测的错误上下文（供 AI 分析） const 表示该函数不会修改类的成员变量
    string getLastErrorContext() const { return last_error_context_; }

    // 获取当前登录状态
    bool is_logged_in() const { return logged_in; }

    // 获取当前用户ID
    int get_current_user_id() const { return current_user_id; }

    // 获取当前账号
    string get_current_account() const { return current_account; }

private:
    DatabaseManager *db_manager;
    int current_user_id = -1;   // 当前登录用户ID
    string current_account;     // 当前登录账号
    bool logged_in = false;     // 登录状态
    string last_error_context_; // 评测错误上下文（供 AI 使用）
};

#endif // USER_H
