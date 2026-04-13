#ifndef VIEW_MANAGER_H
#define VIEW_MANAGER_H

#include "admin_view.h"
#include "user_view.h"
#include <memory>

/**
 * @brief 命令行界面主控制器
 */
class ViewManager
{
public:
    ViewManager();
    ~ViewManager();

    /**
     * @brief 启动登录菜单
     */
    void start_login_menu();

private:
    std::unique_ptr<AdminView> admin_view;
    std::unique_ptr<UserView> user_view;

    /**
     * @brief 清屏
     */
    void clear_screen();

    /**
     * @brief 显示主菜单
     */
    void show_main_menu();

    /**
     * @brief 清空输入缓冲区
     */
    void clear_input();
};

#endif // VIEW_MANAGER_H
