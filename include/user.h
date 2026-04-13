#ifndef USER_H
#define USER_H

#include "db_manager.h"
#include <string>

/**
 * @brief 用户类，负责处理普通用户的业务逻辑
 */
class User
{
public:
    /**
     * @brief 构造函数：关联一个数据库管理器
     */
    User(DatabaseManager *db);

    /**
     * @brief 用户登录
     * @param account 账号
     * @param password 密码
     * @return 登录成功返回true
     */
    bool login(const std::string &account, const std::string &password);

    /**
     * @brief 用户注册
     * @param account 账号
     * @param password 密码
     * @return 注册成功返回true
     */
    bool register_user(const std::string &account, const std::string &password);

    /**
     * @brief 修改自己的密码
     * @param old_password 旧密码
     * @param new_password 新密码
     * @return 修改成功返回true
     */
    bool change_my_password(const std::string &old_password, const std::string &new_password);

    /**
     * @brief 查看所有题目列表
     */
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
     * @param language 编程语言
     */
    void submit_code(int problem_id, const std::string &code, const std::string &language);

    /**
     * @brief 查看个人提交记录
     */
    void view_my_submissions();

    /**
     * @brief 获取当前登录状态
     */
    bool is_logged_in() const { return logged_in; }

    /**
     * @brief 获取当前用户ID
     */
    int get_current_user_id() const { return current_user_id; }

    /**
     * @brief 获取当前账号
     */
    std::string get_current_account() const { return current_account; }

private:
    DatabaseManager *db_manager;
    int current_user_id = -1;    // 当前登录用户ID
    std::string current_account; // 当前登录账号
    bool logged_in = false;      // 登录状态
};

#endif // USER_H
