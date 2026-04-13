#include "../include/admin.h"
#include <iostream>
#include <iomanip>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Admin::Admin(DatabaseManager* db) : db_manager(db) {}

bool Admin::add_problem(const std::string& sql) {
    if (!db_manager) return false;
    return db_manager->run_sql(sql);
}

void Admin::list_all_problems() {
    if (!db_manager) return;
    
    auto results = db_manager->query("SELECT id, title, time_limit, memory_limit FROM OJ.problems");
    
    if (results.empty()) {
        std::cout << "📭 题目列表为空。" << std::endl;
        return;
    }

    std::cout << "\n" << std::left 
              << std::setw(6)  << "ID" 
              << std::setw(25) << "标题" 
              << std::setw(15) << "时间限制(ms)" 
              << std::setw(15) << "内存限制(MB)" << std::endl;
    std::cout << std::string(65, '-') << std::endl;

    for (const auto& row : results) {
        std::cout << std::left 
                  << std::setw(6)  << row.at("id")
                  << std::setw(25) << row.at("title")
                  << std::setw(15) << row.at("time_limit")
                  << std::setw(15) << row.at("memory_limit") << std::endl;
    }
}

void Admin::show_problem_detail(int id) {
    if (!db_manager) return;
    
    std::string sql = "SELECT * FROM OJ.problems WHERE id = " + std::to_string(id);
    auto results = db_manager->query(sql);

    if (results.empty()) {
        std::cout << "❌ 未找到 ID 为 " << id << " 的题目。" << std::endl;
        return;
    }

    // 将结果转换为 JSON 并美化输出
    json j = results[0];
    std::cout << "\n[题目详情 (JSON)]" << std::endl;
    std::cout << j.dump(4) << std::endl;
}
