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

UserView::UserView() : db_manager(nullptr), user_obj(nullptr), ai_client(nullptr) {}

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
    cout << "🔑 正在进入用户模式并建立数据库连接..." << endl;

    // 使用普通用户账号连接（受限权限）
    db_manager = make_unique<DatabaseManager>("localhost", "oj_user", "user123", "OJ");

    if (db_manager->get_connection())
    {
        user_obj = unique_ptr<User>(new User(db_manager.get()));
        ai_client = make_unique<AIClient>();
        cout << "✅ 成功连接数据库。" << endl;

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
                cout << "⚠️ 无效输入，请输入数字！" << endl;
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
                    cout << "🔙 返回登录菜单..." << endl;
                    running = false;
                    break;
                default:
                    cout << "⚠️ 无效选项。" << endl;
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
        cerr << "❌ 数据库连接失败。" << endl;
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
        cout << "🔙 返回..." << endl;
        return;
    }

    cout << "密码: ";
    getline(cin, password);

    if (password == "0")
    {
        cout << "🔙 返回..." << endl;
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
        cout << "🔙 返回..." << endl;
        return;
    }

    cout << "请输入密码: ";
    getline(cin, password);

    if (password == "0")
    {
        cout << "🔙 返回..." << endl;
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
        cout << "⚠️ ID 必须是数字！" << endl;
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
            cout << "⚠️ 无效输入！" << endl;
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
            cout << "⚠️ 无效选项。" << endl;
        }
    }
}

void UserView::handle_submit_code_with_id(int problem_id)
{
    cout << "请输入 C++ 代码 (输入 END 结束):" << endl;
    string code, line;
    while (getline(cin, line))
    {
        if (line == "END")
            break;
        code += line + "\n";
    }

    user_obj->submit_code(problem_id, code, "C++");
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

    // 查询题目详情
    string problem_info = "题目ID: " + to_string(problem_id);
    string sql = "SELECT title, description, time_limit, memory_limit FROM problems WHERE id = " + to_string(problem_id);
    auto results = db_manager->query(sql);
    if (!results.empty())
    {
        problem_info = "【题目信息】\n";
        problem_info += "题号: " + results[0]["id"] + "\n";
        problem_info += "标题: " + results[0]["title"] + "\n";
        problem_info += "描述: " + results[0]["description"] + "\n";
        problem_info += "时间限制: " + results[0]["time_limit"] + " ms\n";
        problem_info += "内存限制: " + results[0]["memory_limit"] + " MB";
    }

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

        // 调用 AI，传入代码和题目信息
        string response = ai_client->ask(question, code, problem_info);

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
        cout << "🔙 返回..." << endl;
        return;
    }

    cout << "请输入新密码: ";
    getline(cin, new_pwd);

    if (new_pwd == "0")
    {
        cout << "🔙 返回..." << endl;
        return;
    }

    user_obj->change_my_password(old_pwd, new_pwd);
}

void UserView::clear_input()
{
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}
