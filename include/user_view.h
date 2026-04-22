#ifndef USER_VIEW_H
#define USER_VIEW_H

#include "db_manager.h"
#include "user.h"
#include "ai_client.h"
#include <memory>

// 用户界面类
class UserView
{
public:
    explicit UserView(unique_ptr<DatabaseManager> db);      // explicit 防止隐式转换
    ~UserView();

    // 启动用户模式
    void start();

private:
    unique_ptr<DatabaseManager> db_manager;
    unique_ptr<User> user_obj;
    unique_ptr<AIClient> ai_client;

    // 清屏
    void clear_screen();

    // 显示游客菜单（未登录）
    void show_guest_menu();

    // 显示用户菜单（已登录）
    void show_user_menu();

    // 处理登录
    void handle_login();

    // 处理注册
    void handle_register();

    // 处理查看题目列表
    void handle_list_problems();

    // 处理查看题目详情
    void handle_view_problem();

    /**
     * @brief 处理提交代码（读取 workspace/solution.cpp）
     * @param problem_id 题目ID
     */
    void handle_submit_code_with_id(int problem_id);

    /**
     * @brief 处理AI助手
     * @param problem_id 题目ID
     */
    void handle_ai_assistant(int problem_id);

    // 处理查看我的提交
    void handle_view_submissions();

    // 处理修改密码
    void handle_change_password();

    // 清空输入缓冲区
    void clear_input();
};

#endif // USER_VIEW_H
