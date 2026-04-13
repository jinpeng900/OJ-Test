#include "../include/user.h"
#include <iostream>

User::User(DatabaseManager *db) : db_manager(db), current_user_id(-1), logged_in(false) {}

bool User::login(const std::string &account, const std::string &password)
{
    if (!db_manager)
        return false;

    // TODO: 查询数据库验证账号密码
    // 临时实现：直接设置登录状态
    current_account = account;
    current_user_id = 1; // 临时ID
    logged_in = true;

    std::cout << "✅ 登录成功，欢迎 " << account << std::endl;
    return true;
}

bool User::register_user(const std::string &account, const std::string &password)
{
    if (!db_manager)
        return false;

    // TODO: 插入新用户到数据库
    std::cout << "✅ 注册成功: " << account << std::endl;
    return true;
}

bool User::change_my_password(const std::string &old_password, const std::string &new_password)
{
    if (!logged_in)
    {
        std::cout << "❌ 请先登录" << std::endl;
        return false;
    }

    // TODO: 验证旧密码并更新
    std::cout << "✅ 密码修改成功" << std::endl;
    return true;
}

void User::list_problems()
{
    if (!db_manager)
        return;

    // TODO: 实现查看题目列表功能
    std::cout << "[User] 查看题目列表 - 待实现" << std::endl;
}

void User::view_problem(int id)
{
    if (!db_manager)
        return;

    // TODO: 实现查看题目详情功能
    std::cout << "[User] 查看题目详情 (ID: " << id << ") - 待实现" << std::endl;
}

void User::submit_code(int problem_id, const std::string &code, const std::string &language)
{
    if (!logged_in)
    {
        std::cout << "❌ 请先登录" << std::endl;
        return;
    }

    // TODO: 实现代码提交评测功能
    std::cout << "[User] 提交代码 (Problem ID: " << problem_id
              << ", Language: " << language << ") - 待实现" << std::endl;
}

void User::view_my_submissions()
{
    if (!logged_in)
    {
        std::cout << "❌ 请先登录" << std::endl;
        return;
    }

    // TODO: 实现查看提交记录功能
    std::cout << "[User] 查看我的提交记录 - 待实现" << std::endl;
}
