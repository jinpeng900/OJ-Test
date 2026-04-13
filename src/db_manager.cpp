#include "../include/db_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>

// --- DatabaseManager 类实现 ---

DatabaseManager::DatabaseManager(const std::string &host, const std::string &user, const std::string &password, const std::string &dbname)
{
    conn = connect_db(host, user, password, dbname);
}

DatabaseManager::~DatabaseManager()
{
    if (conn)
    {
        mysql_close(conn);
        std::cout << "\n[DatabaseManager] 已断开连接并释放资源。" << std::endl;
    }
}

bool DatabaseManager::run_sql(const std::string &sql)
{
    return ::run_sql(conn, sql);
}

std::vector<std::map<std::string, std::string>> DatabaseManager::query(const std::string &sql)
{
    std::vector<std::map<std::string, std::string>> results;
    if (!conn)
        return results;

    if (mysql_query(conn, sql.c_str()))
    {
        std::cerr << "❌ 查询失败: " << mysql_error(conn) << std::endl;
        return results;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res)
    {
        int num_fields = mysql_num_fields(res);
        MYSQL_FIELD *fields = mysql_fetch_fields(res);
        MYSQL_ROW row;

        while ((row = mysql_fetch_row(res)))
        {
            std::map<std::string, std::string> row_map;
            for (int i = 0; i < num_fields; i++)
            {
                row_map[fields[i].name] = (row[i] ? row[i] : "NULL");
            }
            results.push_back(row_map);
        }
        mysql_free_result(res);
    }
    return results;
}

void DatabaseManager::execute_sql_file(const std::string &filepath)
{
    if (!conn)
        return;

    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "❌ 无法打开 SQL 文件: " << filepath << std::endl;
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    // 简单地按 ';' 分割 SQL 语句 (注意：不支持包含在引号内的分号)
    std::string statement;
    std::stringstream ss(content);
    while (std::getline(ss, statement, ';'))
    {
        // 去除首尾空白
        size_t first = statement.find_first_not_of(" \t\n\r");
        if (first == std::string::npos)
            continue;
        size_t last = statement.find_last_not_of(" \t\n\r");
        std::string sql = statement.substr(first, (last - first + 1));

        if (!sql.empty())
        {
            if (mysql_query(conn, sql.c_str()))
            {
                std::cerr << "❌ 执行失败 (文件中的语句): " << mysql_error(conn) << "\n[错误语句]: " << sql << std::endl;
            }
            else
            {
                std::cout << "✅ 执行成功 (文件中的语句): " << (sql.length() > 50 ? sql.substr(0, 50) + "..." : sql) << std::endl;
            }
        }
    }
}

// --- 原始全局函数实现 (供内部使用) ---

MYSQL *connect_db(const std::string &host, const std::string &user, const std::string &password, const std::string &dbname)
{
    MYSQL *conn = mysql_init(nullptr);
    if (conn == nullptr)
    {
        std::cerr << "mysql_init 失败" << std::endl;
        return nullptr;
    }

    const char *db = dbname.empty() ? nullptr : dbname.c_str();
    if (mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(), db, 3306, nullptr, 0) == nullptr)
    {
        std::cerr << "❌ 连接失败: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return nullptr;
    }

    std::cout << "🚀 成功连接到 MySQL 数据库: " << (dbname.empty() ? "(未指定)" : dbname) << std::endl;
    return conn;
}

bool run_sql(MYSQL *conn, const std::string &sql)
{
    if (!conn)
        return false;

    std::cout << "\n[执行 SQL]: " << sql << std::endl;

    if (mysql_query(conn, sql.c_str()))
    {
        std::cerr << "❌ 执行失败: " << mysql_error(conn) << std::endl;
        return false;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res)
    {
        int num_fields = mysql_num_fields(res);
        MYSQL_ROW row;
        MYSQL_FIELD *fields = mysql_fetch_fields(res);

        for (int i = 0; i < num_fields; i++)
        {
            std::cout << fields[i].name << "\t";
        }
        std::cout << "\n--------------------------------------------------" << std::endl;

        while ((row = mysql_fetch_row(res)))
        {
            for (int i = 0; i < num_fields; i++)
            {
                std::cout << (row[i] ? row[i] : "NULL") << "\t";
            }
            std::cout << std::endl;
        }
        mysql_free_result(res);
    }
    else
    {
        if (mysql_field_count(conn) == 0)
        {
            std::cout << "✅ 执行成功，受影响行数: " << mysql_affected_rows(conn) << std::endl;
        }
        else
        {
            std::cerr << "❌ 检索结果失败: " << mysql_error(conn) << std::endl;
            return false;
        }
    }
    return true;
}
