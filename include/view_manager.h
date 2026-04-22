#ifndef VIEW_MANAGER_H
#define VIEW_MANAGER_H

#include "admin_view.h"
#include "user_view.h"
#include <memory>
using namespace std;

// 命令行界面主控制器
class ViewManager
{
public:
    ViewManager();
    ~ViewManager();

    // 启动登录菜单
    void start_login_menu();

private:
    unique_ptr<AdminView> admin_view;
    unique_ptr<UserView> user_view;

    // 清屏
    void clear_screen();

    // 显示主菜单
    void show_main_menu();

    // 清空输入缓冲区
    void clear_input();
};

#endif // VIEW_MANAGER_H
