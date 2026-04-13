#include "../include/admin_view.h"
#include "../include/color_codes.h"
#include <iostream>
#include <limits>

using namespace Color;

AdminView::AdminView() : db_manager(nullptr), admin_obj(nullptr) {}

AdminView::~AdminView() {}

void AdminView::start()
{
    std::cout << "🔑 正在作为管理员进入并建立数据库连接..." << std::endl;

    // 使用管理员专用账号进行连接
    db_manager = std::make_unique<DatabaseManager>("localhost", "oj_admin", "090800", "OJ");

    if (db_manager->get_connection())
    {
        admin_obj = std::unique_ptr<Admin>(new Admin(db_manager.get()));
        std::cout << "✅ 成功以管理员身份连接数据库。" << std::endl;

        bool running = true;
        while (running)
        {
            show_menu();
            int choice;
            std::cout << "\n请输入管理员操作选项 (0-3): ";
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
                handle_list_problems();
                break;
            case 2:
                handle_show_problem();
                break;
            case 3:
                handle_add_problem();
                break;
            case 0:
                std::cout << "🔙 退出管理员模式，正在断开连接..." << std::endl;
                running = false;
                break;
            default:
                std::cout << "⚠️ 无效选项，请重新选择。" << std::endl;
            }
        }

        admin_obj.reset();
        db_manager.reset();
    }
    else
    {
        std::cerr << "❌ 数据库连接失败，请检查管理员账号配置。" << std::endl;
        db_manager.reset();
    }
}

void AdminView::show_menu()
{
    std::cout << "\n"
              << GREEN << "----------------------------------------" << RESET << std::endl;
    std::cout << "       👑 管理员操作面板" << std::endl;
    std::cout << GREEN << "----------------------------------------" << RESET << std::endl;
    std::cout << " 1. 查看所有题目列表" << std::endl;
    std::cout << " 2. 查看题目详细信息 (通过 ID)" << std::endl;
    std::cout << " 3. 发布新题目 (手动 SQL)" << std::endl;
    std::cout << " 0. 返回登录菜单" << std::endl;
    std::cout << GREEN << "----------------------------------------" << RESET << std::endl;
}

void AdminView::handle_list_problems()
{
    admin_obj->list_all_problems();
}

void AdminView::handle_show_problem()
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
    admin_obj->show_problem_detail(id);
}

void AdminView::handle_add_problem()
{
    std::string sql;
    std::cout << "--- 管理员发布新题目 (输入 SQL) ---" << std::endl;
    std::cout << "SQL> ";
    std::getline(std::cin, sql);
    if (sql.empty())
    {
        std::cout << "⚠️ SQL 语句不能为空！" << std::endl;
    }
    else if (!admin_obj->add_problem(sql))
    {
        std::cout << "❌ 输入错误" << std::endl;
    }
    else
    {
        std::cout << "✅ 题目发布成功！" << std::endl;
    }
}

void AdminView::clear_input()
{
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
