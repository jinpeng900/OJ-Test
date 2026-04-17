#include "../include/admin.h"
#include <iostream>
#include <iomanip>
#include <nlohmann/json.hpp>

using namespace std;

using json = nlohmann::json;

Admin::Admin(DatabaseManager* db) : db_manager(db) {}

bool Admin::add_problem(const string& sql) {
    if (!db_manager) return false;
    return db_manager->run_sql(sql);
}

void Admin::list_all_problems() {
    if (!db_manager) return;
    
    auto results = db_manager->query("SELECT id, title, time_limit, memory_limit FROM OJ.problems");
    
    if (results.empty()) {
        cout << "📭 题目列表为空。" << endl;
        return;
    }

    cout << "\n" << left 
              << setw(6)  << "ID" 
              << setw(25) << "标题" 
              << setw(15) << "时间限制(ms)" 
              << setw(15) << "内存限制(MB)" << endl;
    cout << string(65, '-') << endl;

    for (const auto& row : results) {
        cout << left 
                  << setw(6)  << row.at("id")
                  << setw(25) << row.at("title")
                  << setw(15) << row.at("time_limit")
                  << setw(15) << row.at("memory_limit") << endl;
    }
}

void Admin::show_problem_detail(int id) {
    if (!db_manager) return;
    
    string sql = "SELECT * FROM OJ.problems WHERE id = " + to_string(id);
    auto results = db_manager->query(sql);

    if (results.empty()) {
        cout << "❌ 未找到 ID 为 " << id << " 的题目。" << endl;
        return;
    }

    // 将结果转换为 JSON 并美化输出
    json j = results[0];
    cout << "\n[题目详情 (JSON)]" << endl;
    cout << j.dump(4) << endl;
}
