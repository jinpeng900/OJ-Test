#ifndef USER_VIEW_H
#define USER_VIEW_H

#include "db_manager.h"
#include "user.h"
#include <memory>

/**
 * @brief 用户界面类
 */
class UserView
{
public:
    UserView();
    ~UserView();

    /**
     * @brief 启动用户模式
     */
    void start();

private:
    std::unique_ptr<DatabaseManager> db_manager;
    std::unique_ptr<User> user_obj;

    /**
     * @brief 清屏
     */
    void clear_screen();

    /**
     * @brief 显示游客菜单（未登录）
     */
    void show_guest_menu();

    /**
     * @brief 显示用户菜单（已登录）
     */
    void show_user_menu();

    /**
     * @brief 处理登录
     */
    void handle_login();

    /**
     * @brief 处理注册
     */
    void handle_register();

    /**
     * @brief 处理查看题目列表
     */
    void handle_list_problems();

    /**
     * @brief 处理查看题目详情
     */
    void handle_view_problem();

    /**
     * @brief 处理提交代码
     */
    void handle_submit_code();

    /**
     * @brief 处理查看我的提交
     */
    void handle_view_submissions();

    /**
     * @brief 处理修改密码
     */
    void handle_change_password();

    /**
     * @brief 清空输入缓冲区
     */
    void clear_input();
};

#endif // USER_VIEW_H
