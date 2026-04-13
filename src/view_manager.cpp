#include "../include/view_manager.h"
#include "../include/color_codes.h"
#include <iostream>
#include <limits>

using namespace Color;

ViewManager::ViewManager() : admin_view(nullptr), user_view(nullptr) {}

ViewManager::~ViewManager() {}

void ViewManager::clear_screen()
{
    std::cout << "\033[2J\033[H";
}

void ViewManager::show_main_menu()
{
    std::cout << GREEN << "========================================" << RESET << std::endl;
    std::cout << "       🚀 OJ 在线判题系统 - 登录" << std::endl;
    std::cout << GREEN << "========================================" << RESET << std::endl;
    std::cout << " 1. 管理员进入" << std::endl;
    std::cout << " 2. 用户进入" << std::endl;
    std::cout << " 0. 退出系统" << std::endl;
    std::cout << GREEN << "========================================" << RESET << std::endl;
}

void ViewManager::start_login_menu()
{
    bool running = true;
    while (running)
    {
        clear_screen();
        show_main_menu();

        int choice;
        std::cout << "请选择角色 (0-2): ";
        if (!(std::cin >> choice))
        {
            clear_input();
            std::cout << "⚠️ 无效输入，请输入数字！" << std::endl;
            continue;
        }
        clear_input();

        switch (choice)
        {
        case 1:
            admin_view = std::make_unique<AdminView>();
            admin_view->start();
            admin_view.reset();
            break;
        case 2:
            user_view = std::make_unique<UserView>();
            user_view->start();
            user_view.reset();
            break;
        case 0:
            std::cout << "👋 正在退出..." << std::endl;
            running = false;
            break;
        default:
            std::cout << "⚠️ 无效选项，请重新选择。" << std::endl;
        }
    }
}

void ViewManager::clear_input()
{
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
