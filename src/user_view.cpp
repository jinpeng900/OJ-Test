#include "../include/user_view.h"
#include "../include/color_codes.h"
#include <iostream>
#include <limits>

using namespace Color;

UserView::UserView() : db_manager(nullptr), user_obj(nullptr) {}

UserView::~UserView() {}

void UserView::clear_screen()
{
    std::cout << "\033[2J\033[H";
}

void UserView::start()
{
    clear_screen();
    std::cout << "🔑 正在进入用户模式并建立数据库连接..." << std::endl;

    // 使用普通用户账号连接（受限权限）
    db_manager = std::make_unique<DatabaseManager>("localhost", "oj_user", "user123", "OJ");

    if (db_manager->get_connection())
    {
        user_obj = std::unique_ptr<User>(new User(db_manager.get()));
        std::cout << "✅ 成功连接数据库。" << std::endl;

        bool running = true;
        while (running)
        {
            if (!user_obj->is_logged_in())
            {
                show_guest_menu();
            }
            else
            {
                show_user_menu();
            }

            int choice;
            std::cout << "\n请输入选项: ";
            if (!(std::cin >> choice))
            {
                clear_input();
                std::cout << "⚠️ 无效输入，请输入数字！" << std::endl;
                continue;
            }
            clear_input();

            if (!user_obj->is_logged_in())
            {
                // 未登录状态
                switch (choice)
                {
                case 1:
                    handle_login();
                    break;
                case 2:
                    handle_register();
                    break;
                case 0:
                    std::cout << "🔙 返回登录菜单..." << std::endl;
                    running = false;
                    break;
                default:
                    std::cout << "⚠️ 无效选项。" << std::endl;
                }
            }
            else
            {
                // 已登录状态
                switch (choice)
                {
                case 1:
                    handle_list_problems();
                    break;
                case 2:
                    handle_view_problem();
                    break;
                case 3:
                    handle_submit_code();
                    break;
                case 4:
                    handle_view_submissions();
                    break;
                case 5:
                    handle_change_password();
                    break;
                case 0:
                    std::cout << "🔙 退出用户模式..." << std::endl;
                    running = false;
                    break;
                default:
                    std::cout << "⚠️ 无效选项。" << std::endl;
                }
            }
        }

        user_obj.reset();
        db_manager.reset();
    }
    else
    {
        std::cerr << "❌ 数据库连接失败。" << std::endl;
        db_manager.reset();
    }
}

void UserView::show_guest_menu()
{
    std::cout << "\n"
              << GREEN << "----------------------------------------" << RESET << std::endl;
    std::cout << "       👤 用户模式 (未登录)" << std::endl;
    std::cout << GREEN << "----------------------------------------" << RESET << std::endl;
    std::cout << " 1. 登录" << std::endl;
    std::cout << " 2. 注册新账号" << std::endl;
    std::cout << " 0. 返回主菜单" << std::endl;
    std::cout << GREEN << "----------------------------------------" << RESET << std::endl;
}

void UserView::show_user_menu()
{
    std::cout << "\n"
              << GREEN << "----------------------------------------" << RESET << std::endl;
    std::cout << "       👤 用户模式 - " << user_obj->get_current_account() << std::endl;
    std::cout << GREEN << "----------------------------------------" << RESET << std::endl;
    std::cout << " 1. 查看题目列表" << std::endl;
    std::cout << " 2. 查看题目详情" << std::endl;
    std::cout << " 3. 提交代码 (C++)" << std::endl;
    std::cout << " 4. 查看我的提交" << std::endl;
    std::cout << " 5. 修改密码" << std::endl;
    std::cout << " 0. 退出登录" << std::endl;
    std::cout << GREEN << "----------------------------------------" << RESET << std::endl;
}

void UserView::handle_login()
{
    std::string account, password;
    std::cout << "账号: ";
    std::getline(std::cin, account);
    std::cout << "密码: ";
    std::getline(std::cin, password);
    user_obj->login(account, password);
}

void UserView::handle_register()
{
    std::string account, password;
    std::cout << "请输入新账号: ";
    std::getline(std::cin, account);
    std::cout << "请输入密码: ";
    std::getline(std::cin, password);
    user_obj->register_user(account, password);
}

void UserView::handle_list_problems()
{
    user_obj->list_problems();
}

void UserView::handle_view_problem()
{
    int id;
    std::cout << "请输入题目 ID: ";
    if (!(std::cin >> id))
    {
        clear_input();
        std::cout << "⚠️ ID 必须是数字！" << std::endl;
        return;
    }
    clear_input();
    user_obj->view_problem(id);
}

void UserView::handle_submit_code()
{
    int problem_id;
    std::cout << "请输入题目 ID: ";
    if (!(std::cin >> problem_id))
    {
        clear_input();
        std::cout << "⚠️ ID 必须是数字！" << std::endl;
        return;
    }
    clear_input();

    std::cout << "请输入 C++ 代码 (输入 END 结束):" << std::endl;
    std::string code, line;
    while (std::getline(std::cin, line))
    {
        if (line == "END")
            break;
        code += line + "\n";
    }

    user_obj->submit_code(problem_id, code, "C++");
}

void UserView::handle_view_submissions()
{
    user_obj->view_my_submissions();
}

void UserView::handle_change_password()
{
    std::string old_pwd, new_pwd;
    std::cout << "请输入旧密码: ";
    std::getline(std::cin, old_pwd);
    std::cout << "请输入新密码: ";
    std::getline(std::cin, new_pwd);
    user_obj->change_my_password(old_pwd, new_pwd);
}

void UserView::clear_input()
{
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
