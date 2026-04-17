#include "../include/view_manager.h"

using namespace std;

int main() {
    // 初始化可视化界面管理器
    ViewManager view;

    // 启动登录菜单 (在菜单中选择角色后才会建立数据库连接)
    view.start_login_menu();

    return 0;
}
