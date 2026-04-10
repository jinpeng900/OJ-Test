#include "../include/db_manager.h"
#include <iostream>

MYSQL* connect_db(const std::string& host, const std::string& user, const std::string& password, const std::string& dbname) {
    MYSQL* conn = mysql_init(nullptr);
    if (conn == nullptr) {
        std::cerr << "mysql_init 失败" << std::endl;
        return nullptr;
    }

    // 执行连接
    const char* db = dbname.empty() ? nullptr : dbname.c_str();
    if (mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(), db, 3306, nullptr, 0) == nullptr) {
        std::cerr << "❌ 连接失败: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return nullptr;
    }

    std::cout << "🚀 成功连接到 MySQL 数据库: " << (dbname.empty() ? "(未指定)" : dbname) << std::endl;
    return conn;
}

void run_sql(MYSQL* conn, const std::string& sql) {
    if (!conn) return;
    
    std::cout << "\n[执行 SQL]: " << sql << std::endl;
    
    if (mysql_query(conn, sql.c_str())) {
        std::cerr << "❌ 执行失败: " << mysql_error(conn) << std::endl;
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res) {
        int num_fields = mysql_num_fields(res);
        MYSQL_ROW row;
        MYSQL_FIELD* fields = mysql_fetch_fields(res);

        for(int i = 0; i < num_fields; i++) {
            std::cout << fields[i].name << "\t";
        }
        std::cout << "\n--------------------------------------------------" << std::endl;

        while ((row = mysql_fetch_row(res))) {
            for(int i = 0; i < num_fields; i++) {
                std::cout << (row[i] ? row[i] : "NULL") << "\t";
            }
            std::cout << std::endl;
        }
        mysql_free_result(res);
    } else {
        if (mysql_field_count(conn) == 0) {
            std::cout << "✅ 执行成功，受影响行数: " << mysql_affected_rows(conn) << std::endl;
        } else {
            std::cerr << "❌ 检索结果失败: " << mysql_error(conn) << std::endl;
        }
    }
}
