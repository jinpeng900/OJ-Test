#ifndef ADMIN_VIEW_H
#define ADMIN_VIEW_H

#include "db_manager.h"
#include "admin.h"
#include <memory>
using namespace std;

// 管理员界面类
class AdminView
{
public:
    explicit AdminView(unique_ptr<DatabaseManager> db);
    ~AdminView();

    // 启动管理员模式
    void start();

private:
    unique_ptr<DatabaseManager> db_manager;
    unique_ptr<Admin> admin_obj;

    // 清屏
    void clear_screen();

    // 显示管理员菜单
    void show_menu();

    // 处理查看所有题目
    void handle_list_problems();

    // 处理查看题目详情
    void handle_show_problem();

    // 处理添加新题目
    void handle_add_problem();

    // 清空输入缓冲区
    void clear_input();
};

#endif // ADMIN_VIEW_H
