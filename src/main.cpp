#include "../include/db_manager.h"
#include <iostream>

int main() {
    // 调用连接函数 (使用你的 root 和 090800)
    MYSQL* conn = connect_db("localhost", "root", "090800");

    if (conn) {
        // --- 在这里直接写 MySQL 语言并运行 ---
        
        // 示例 1: 查看数据库
        run_sql(conn, "SHOW DATABASES");

        // 示例 2: 创建并切换数据库
        run_sql(conn, "CREATE DATABASE IF NOT EXISTS oj_test");
        run_sql(conn, "USE oj_test");

        // 示例 3: 创建表并插入数据
        run_sql(conn, "CREATE TABLE IF NOT EXISTS test_table (id INT PRIMARY KEY, content TEXT)");
        run_sql(conn, "REPLACE INTO test_table VALUES (1, 'Hello standard CMake structure!')");
        
        // 示例 4: 查询数据
        run_sql(conn, "SELECT * FROM test_table");

        // ------------------------------------

        // 关闭连接
        mysql_close(conn);
        std::cout << "\n已断开连接。" << std::endl;
    }

    return 0;
}
