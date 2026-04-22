#include "../include/admin.h"
#include "../include/color_codes.h"
#include <iostream>

using namespace std;
using namespace Color;

Admin::Admin(DatabaseManager *db) : db_manager(db) {}

bool Admin::add_problem(const string &sql)
{
    if (!db_manager)
        return false;
    return db_manager->run_sql(sql);
}

void Admin::list_all_problems()
{
    if (!db_manager)
        return;

    auto results = db_manager->query("SELECT id, title, category, time_limit, memory_limit FROM OJ.problems ORDER BY id");

    if (results.empty())
    {
        cout << "暂无题目" << endl;
        return;
    }

    cout << "\n"
         << GREEN << "============================== 题目列表 ================================" << RESET << endl;
    cout << "ID  标题                                知识点" << endl;
    cout << string(70, '-') << endl;

    for (const auto &row : results)
    {
        string category = row.at("category");
        if (category.empty())
            category = "-";

        string title = row.at("title");
        string id = row.at("id");

        int display_width = 0;
        string formatted_title = "";

        for (size_t i = 0; i < title.length();)
        {
            int bytes = 1;
            int width = 1;

            if ((title[i] & 0x80) == 0)
            {
                bytes = 1;
                width = 1;
            }
            else if ((title[i] & 0xE0) == 0xE0)
            {
                bytes = 3;
                width = 2;
            }
            else if ((title[i] & 0xE0) == 0xC0)
            {
                bytes = 2;
                width = 1;
            }
            else
            {
                bytes = 4;
                width = 2;
            }

            if (display_width + width > 33 && i + bytes < title.length())
            {
                formatted_title += "...";
                display_width += 3;
                break;
            }

            formatted_title += title.substr(i, bytes);
            display_width += width;
            i += bytes;
        }

        if (display_width < 36)
        {
            formatted_title += string(36 - display_width, ' ');
        }

        cout << id;

        int id_spaces = 4 - id.length();
        if (id_spaces > 0)
        {
            cout << string(id_spaces, ' ');
        }

        cout << formatted_title << category << endl;
    }

    cout << GREEN << "======================================================================" << RESET << "\n"
         << endl;
}

void Admin::show_problem_detail(int id)
{
    if (!db_manager)
        return;

    string sql = "SELECT * FROM OJ.problems WHERE id = " + to_string(id);
    auto results = db_manager->query(sql);

    if (results.empty())
    {
        cout << "未找到 ID 为 " << id << " 的题目。" << endl;
        return;
    }

    const auto &p = results[0];

    cout << "\n"
         << GREEN << "============================ 题目详情 ============================" << RESET << endl;
    cout << "【题号】     " << p.at("id") << endl;
    cout << "【标题】     " << p.at("title") << endl;
    cout << "【知识点】   " << (p.at("category").empty() ? "-" : p.at("category")) << endl;
    cout << "【时间限制】 " << p.at("time_limit") << " ms" << endl;
    cout << "【内存限制】 " << p.at("memory_limit") << " MB" << endl;
    cout << GREEN << "---------------------------- 题目描述 ----------------------------" << RESET << endl;
    cout << p.at("description") << endl;
    cout << GREEN << "================================================================" << RESET << "\n"
         << endl;
}
