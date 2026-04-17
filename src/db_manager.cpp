#include "../include/db_manager.h"
#include <iostream>

using namespace std;

// --- DatabaseManager 类实现 ---

DatabaseManager::DatabaseManager(const string &host, const string &user, const string &password, const string &dbname)
{
    conn = connect_db(host, user, password, dbname);
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
    return ::run_sql(conn, sql);
}

vector<map<string, string>> DatabaseManager::query(const string &sql)
{
    vector<map<string, string>> results;
    if (!conn)
        return results;

    if (mysql_query(conn, sql.c_str()))
    {
        cerr << "❌ 查询失败: " << mysql_error(conn) << endl;
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

// --- 原始全局函数实现 (供内部使用) ---

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
        cerr << "❌ 连接失败: " << mysql_error(conn) << endl;
        mysql_close(conn);
        return nullptr;
    }

    return conn;
}

bool run_sql(MYSQL *conn, const string &sql)
{
    if (!conn)
        return false;

    if (mysql_query(conn, sql.c_str()))
    {
        cerr << "❌ 执行失败: " << mysql_error(conn) << endl;
        return false;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res)
    {
        mysql_free_result(res);
    }

    return true;
}
