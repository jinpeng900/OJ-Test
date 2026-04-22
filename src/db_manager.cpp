#include "../include/db_manager.h"
#include <iostream>
#include <vector>

using namespace std;

// --- DatabaseManager 类实现 ---

DatabaseManager::DatabaseManager(const string &host, const string &user, const string &password, const string &dbname)
    : conn(connect_db(host, user, password, dbname))
{
}

DatabaseManager::~DatabaseManager()
{
    if (conn)
    {
        mysql_close(conn);
    }
}

bool DatabaseManager::run_sql(const string &sql)
{
    if (!conn)
        return false;
    // c_str() 将 C++ string 转换为 C 风格字符串（const char*）
    // mysql 函数需要 C 风格字符串作为输入
    // mysql_query 用来执行 SQL 语句，返回 0 表示成功，非 0 表示失败
    if (mysql_query(conn, sql.c_str()))
    {
        cerr << "执行失败: " << mysql_error(conn) << endl;
        return false;
    }

    // res 用于存储查询结果，如果 SQL 语句是 SELECT 之类的查询语句，mysql_store_result 会返回一个结果集对象
    MYSQL_RES *res = mysql_store_result(conn);
    if (res)
    {
        mysql_free_result(res);
    }

    return true;
}

string DatabaseManager::escape_string(const string &s) const
{
    if (!conn)
        return s;
    vector<char> buf(s.length() * 2 + 1);
    mysql_real_escape_string(conn, buf.data(), s.c_str(), s.length());
    return string(buf.data());
}

vector<map<string, string>> DatabaseManager::query(const string &sql)
{
    vector<map<string, string>> results;
    if (!conn)
        return results;

    if (mysql_query(conn, sql.c_str()))
    {
        cerr << "查询失败: " << mysql_error(conn) << endl;
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
            map<string, string> row_map;
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

// --- 原始连接函数实现 ---

MYSQL *connect_db(const string &host, const string &user, const string &password, const string &dbname)
{
    MYSQL *conn = mysql_init(nullptr);
    if (conn == nullptr)
    {
        cerr << "mysql_init 失败" << endl;
        return nullptr;
    }

    const char *db = dbname.empty() ? nullptr : dbname.c_str();
    if (mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(), db, 3306, nullptr, 0) == nullptr)
    {
        cerr << "连接失败: " << mysql_error(conn) << endl;
        mysql_close(conn);
        return nullptr;
    }

    return conn;
}
