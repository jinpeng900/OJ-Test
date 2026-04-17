#ifndef ADMIN_VIEW_H
#define ADMIN_VIEW_H

#include "db_manager.h"
#include "admin.h"
#include <memory>

/**
 * @brief 管理员界面类
 */
class AdminView
{
public:
    AdminView();
    ~AdminView();

    /**
     * @brief 启动管理员模式
     */
    void start();

private:
    std::unique_ptr<DatabaseManager> db_manager;
    std::unique_ptr<Admin> admin_obj;

    /**
     * @brief 清屏
     */
    void clear_screen();

    /**
     * @brief 显示管理员菜单
     */
    void show_menu();

    /**
     * @brief 处理查看所有题目
     */
    void handle_list_problems();

    /**
     * @brief 处理查看题目详情
     */
    void handle_show_problem();

    /**
     * @brief 处理添加新题目
     */
    void handle_add_problem();

    /**
     * @brief 清空输入缓冲区
     */
    void clear_input();
};

#endif // ADMIN_VIEW_H
