#include "../include/admin_view.h"
#include "../include/color_codes.h"
#include <iostream>
#include <limits>

using namespace std;

using namespace Color;

AdminView::AdminView(unique_ptr<DatabaseManager> db)
    : db_manager(move(db)), admin_obj(nullptr) {}

AdminView::~AdminView() {}

void AdminView::clear_screen()
{
    // 使用 ANSI 转义序列清屏并移动光标到左上角
    cout << "\033[2J\033[3J\033[H";
    cout.flush();
}

void AdminView::start()
{
    clear_screen();
    cout << "正在作为管理员进入并建立数据库连接..." << endl;

    // 数据库连接由调用方（ViewManager / AppContext）注入

    if (db_manager->get_connection())
    {
        admin_obj = make_unique<Admin>(db_manager.get()); // 将 DatabaseManager 的裸指针传递给 Admin 对象       get() 返回 unique_ptr 管理的原始指针
        cout << "成功以管理员身份连接数据库。" << endl;

        bool running = true;
        while (running)
        {
            show_menu();
            int choice;
            cout << "\n请输入管理员操作选项 (0-3): ";
            if (!(cin >> choice))
            {
                clear_input();
                cout << "无效输入，请输入数字！" << endl;
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
                cout << "退出管理员模式，正在断开连接..." << endl;
                running = false;
                break;
            default:
                cout << "无效选项，请重新选择。" << endl;
            }
        }

        admin_obj.reset();
        db_manager.reset();
    }
    else
    {
        cerr << "数据库连接失败，请检查管理员账号配置。" << endl;
        db_manager.reset();
    }
}

void AdminView::show_menu()
{
    cout << "\n"
         << GREEN << "----------------------------------------" << RESET << endl;
    cout << "         管理员操作面板" << endl;
    cout << GREEN << "----------------------------------------" << RESET << endl;
    cout << " 1. 查看所有题目列表" << endl;
    cout << " 2. 查看题目详细信息 (通过 ID)" << endl;
    cout << " 3. 发布新题目 (手动 SQL)" << endl;
    cout << " 0. 返回登录菜单" << endl;
    cout << GREEN << "----------------------------------------" << RESET << endl;
}

void AdminView::handle_list_problems()
{
    clear_screen();
    admin_obj->list_all_problems();
}

void AdminView::handle_show_problem()
{
    clear_screen();
    int id;
    cout << "请输入题目 ID: ";
    if (!(cin >> id))
    {
        clear_input();
        cout << "ID 必须是数字！" << endl;
        return;
    }
    clear_input();
    admin_obj->show_problem_detail(id);
}

void AdminView::handle_add_problem()
{
    clear_screen();
    string sql;
    cout << "--- 管理员发布新题目 (输入 SQL) ---" << endl;
    cout << "SQL> ";
    getline(cin, sql);
    if (sql.empty())
    {
        cout << "SQL 语句不能为空！" << endl;
    }
    else if (!admin_obj->add_problem(sql))
    {
        cout << "输入错误" << endl;
    }
    else
    {
        cout << "题目发布成功！" << endl;
    }
}

void AdminView::clear_input()
{
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}
