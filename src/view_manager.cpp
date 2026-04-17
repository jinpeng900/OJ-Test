#include "../include/view_manager.h"
#include "../include/color_codes.h"
#include <iostream>
#include <limits>

using namespace std;

using namespace Color;

ViewManager::ViewManager() : admin_view(nullptr), user_view(nullptr) {}

ViewManager::~ViewManager() {}

void ViewManager::clear_screen()
{
    // 使用 ANSI 转义序列清屏并移动光标到左上角
    cout << "\033[2J\033[3J\033[H";
    cout.flush();
}

void ViewManager::show_main_menu()
{
    cout << GREEN << "========================================" << RESET << endl;
    cout << "       🚀 OJ 在线判题系统 - 登录" << endl;
    cout << GREEN << "========================================" << RESET << endl;
    cout << " 1. 管理员进入" << endl;
    cout << " 2. 用户进入" << endl;
    cout << " 0. 退出系统" << endl;
    cout << GREEN << "========================================" << RESET << endl;
}

void ViewManager::start_login_menu()
{
    bool running = true;
    while (running)
    {
        clear_screen();
        show_main_menu();

        int choice;
        cout << "请选择角色 (0-2): ";
        if (!(cin >> choice))
        {
            clear_input();
            cout << "⚠️ 无效输入，请输入数字！" << endl;
            continue;
        }
        clear_input();

        switch (choice)
        {
        case 1:
            admin_view = make_unique<AdminView>();
            admin_view->start();
            admin_view.reset();
            break;
        case 2:
            user_view = make_unique<UserView>();
            user_view->start();
            user_view.reset();
            break;
        case 0:
            cout << "👋 正在退出..." << endl;
            running = false;
            break;
        default:
            cout << "⚠️ 无效选项，请重新选择。" << endl;
        }
    }
}

void ViewManager::clear_input()
{
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}
