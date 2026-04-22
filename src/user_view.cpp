#include "../include/user_view.h"
#include "../include/color_codes.h"
#include <iostream>
#include <limits>
#include <fstream>
#include <sstream>

using namespace std;

using namespace Color;

// 辅助函数：读取文件内容
static string read_file(const string &path)
{
    ifstream file(path);
    if (!file.is_open())
    {
        return "";
    }
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

UserView::UserView(unique_ptr<DatabaseManager> db) // move() 将 db 的所有权转移到 UserView 内部
    : db_manager(move(db)), user_obj(nullptr), ai_client(nullptr)
{
}

UserView::~UserView() {}

void UserView::clear_screen()
{
    // 使用 ANSI 转义序列清屏并移动光标到左上角
    cout << "\033[2J\033[3J\033[H";
    cout.flush();
}

void UserView::start()
{
    clear_screen();
    cout << "正在进入用户模式并建立数据库连接..." << endl;

    // 数据库连接由调用方（ViewManager / AppContext）注入

    if (db_manager->get_connection())
    {
        user_obj = make_unique<User>(db_manager.get());
        ai_client = make_unique<AIClient>();
        ai_client->setDatabaseManager(db_manager.get());
        cout << "成功连接数据库。" << endl;

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
            cout << "\n请输入选项: ";
            if (!(cin >> choice))
            {
                clear_input();
                cout << "无效输入，请输入数字！" << endl;
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
                    // 登录成功后清屏
                    if (user_obj->is_logged_in())
                    {
                        clear_screen();
                    }
                    break;
                case 2:
                    handle_register();
                    break;
                case 0:
                    cout << "返回登录菜单..." << endl;
                    running = false;
                    break;
                default:
                    cout << "无效选项。" << endl;
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
                    handle_view_submissions();
                    break;
                case 4:
                    handle_change_password();
                    break;
                case 0:
                    cout << "🔙 退出用户模式..." << endl;
                    running = false;
                    break;
                default:
                    cout << "⚠️ 无效选项。" << endl;
                }
            }
        }

        user_obj.reset();
        db_manager.reset();
    }
    else
    {
        cerr << "数据库连接失败。" << endl;
        db_manager.reset();
    }
}

void UserView::show_guest_menu()
{
    cout << "\n"
         << GREEN << "----------------------------------------" << RESET << endl;
    cout << "       👤 用户模式 (未登录)" << endl;
    cout << GREEN << "----------------------------------------" << RESET << endl;
    cout << " 1. 登录" << endl;
    cout << " 2. 注册新账号" << endl;
    cout << " 0. 返回主菜单" << endl;
    cout << GREEN << "----------------------------------------" << RESET << endl;
}

void UserView::show_user_menu()
{
    cout << "\n"
         << GREEN << "----------------------------------------" << RESET << endl;
    cout << "       👤 用户模式 - " << user_obj->get_current_account() << endl;
    cout << GREEN << "----------------------------------------" << RESET << endl;
    cout << " 1. 查看题目列表" << endl;
    cout << " 2. 查看题目详情" << endl;
    cout << " 3. 查看我的提交" << endl;
    cout << " 4. 修改密码" << endl;
    cout << " 0. 退出登录" << endl;
    cout << GREEN << "----------------------------------------" << RESET << endl;
}

void UserView::handle_login()
{
    cout << "(输入 0 返回上一步)" << endl;

    string account, password;
    cout << "账号: ";
    getline(cin, account);

    // 检查是否返回
    if (account == "0")
    {
        cout << "返回..." << endl;
        return;
    }

    cout << "密码: ";
    getline(cin, password);

    if (password == "0")
    {
        cout << "返回..." << endl;
        return;
    }

    user_obj->login(account, password);
}

void UserView::handle_register()
{
    cout << "(输入 0 返回上一步)" << endl;

    string account, password;
    cout << "请输入新账号: ";
    getline(cin, account);

    // 检查是否返回
    if (account == "0")
    {
        cout << "返回..." << endl;
        return;
    }

    cout << "请输入密码: ";
    getline(cin, password);

    if (password == "0")
    {
        cout << "返回..." << endl;
        return;
    }

    user_obj->register_user(account, password);
}

void UserView::handle_list_problems()
{
    clear_screen();
    user_obj->list_problems();
}

void UserView::handle_view_problem()
{
    int id;
    cout << "请输入题目 ID (0 返回): ";
    if (!(cin >> id))
    {
        clear_input();
        cout << "ID 必须是数字！" << endl;
        return;
    }
    clear_input();

    if (id == 0)
    {
        return;
    }

    // 进入题目详情子菜单
    bool in_problem_menu = true;
    while (in_problem_menu)
    {
        clear_screen();
        user_obj->view_problem(id);

        cout << GREEN << "---------- 操作选项 ----------" << RESET << endl;
        cout << " 1. 提交代码" << endl;
        cout << " 2. AI 助手" << endl;
        cout << " 0. 返回用户模式" << endl;
        cout << GREEN << "------------------------------" << RESET << endl;
        cout << "请输入选项: ";

        int choice;
        if (!(cin >> choice))
        {
            clear_input();
            cout << "无效输入！" << endl;
            continue;
        }
        clear_input();

        switch (choice)
        {
        case 1:
            handle_submit_code_with_id(id);
            break;
        case 2:
            handle_ai_assistant(id);
            break;
        case 0:
            in_problem_menu = false;
            break;
        default:
            cout << "无效选项。" << endl;
        }
    }
}

void UserView::handle_submit_code_with_id(int problem_id)
{
    string code = read_file("workspace/solution.cpp");
    if (code.empty())
    {
        cout << RED << "workspace/solution.cpp 为空或不存在，请先编写代码。" << RESET << endl;
        return;
    }
    cout << "已读取 workspace/solution.cpp（" << code.size() << " 字节）" << endl;
    user_obj->submit_code(problem_id, code);
    cout << "\n按回车键返回...";
    cin.get();
}

void UserView::handle_ai_assistant(int problem_id)
{
    clear_screen();

    // 检查 AI 服务是否可用
    if (!ai_client || !ai_client->isAvailable())
    {
        cout << RED << "AI 服务不可用，请检查配置。" << RESET << endl;
        cout << "按回车键返回...";
        cin.get();
        return;
    }

    // 读取工作区代码
    string code = read_file("workspace/solution.cpp");

    cout << GREEN << "========== AI 助手 ==========" << RESET << endl;
    cout << "题目 ID: " << problem_id << endl;
    cout << "输入问题进行对话，输入 0 或 /quit 返回" << endl;
    cout << GREEN << "=============================" << RESET << endl;

    bool in_ai = true;
    while (in_ai)
    {
        cout << "\n"
             << CYAN << "你> " << RESET;

        string question;
        getline(cin, question);

        // 检查退出条件
        if (question == "0" || question == "/quit")
        {
            in_ai = false;
            continue;
        }

        // 忽略空输入
        if (question.empty())
        {
            continue;
        }

        cout << "正在思考..." << endl;

        // 调用 AI，自动查询题目上下文并处理 NEED_PROBLEMS
        string error_ctx = user_obj->getLastErrorContext();
        string response = ai_client->askWithProblemContext(question, code, problem_id, error_ctx);

        cout << GREEN << "AI> " << RESET << response << endl;
    }
}

void UserView::handle_view_submissions()
{
    clear_screen();
    user_obj->view_my_submissions();
}

void UserView::handle_change_password()
{
    clear_screen();
    cout << "(输入 0 返回上一步)" << endl;

    string old_pwd, new_pwd;
    cout << "请输入旧密码: ";
    getline(cin, old_pwd);

    // 检查是否返回
    if (old_pwd == "0")
    {
        cout << "返回..." << endl;
        return;
    }

    cout << "请输入新密码: ";
    getline(cin, new_pwd);

    if (new_pwd == "0")
    {
        cout << "返回..." << endl;
        return;
    }

    user_obj->change_my_password(old_pwd, new_pwd);
}

void UserView::clear_input()
{
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}
