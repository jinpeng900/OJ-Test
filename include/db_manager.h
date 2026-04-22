#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <map>
using namespace std;

// 数据库管理类，封装连接与 SQL 执行
class DatabaseManager
{
public:
    // 构造函数：初始化并连接数据库
    DatabaseManager(const string &host, const string &user, const string &password, const string &dbname = "");

    // 析构函数：释放资源
    ~DatabaseManager();

    // 获取连接句柄
    MYSQL *get_connection() const { return conn; }

    /**
     * @brief 执行 SQL 语句并打印结果
     * @param sql 要执行的 SQL 语句
     * @return 成功返回 true，失败返回 false
     */
    bool run_sql(const string &sql);

    /**
     * @brief 执行查询语句并返回结果集
     * @param sql 查询语句
     * @return 结果集，每行是一个 map (列名 -> 值)
     */
    vector<map<string, string>> query(const string &sql);

    /**
     * @brief 对字符串进行 MySQL 转义（防 SQL 注入）
     * @param s 原始字符串
     * @return 转义后的字符串
     */
    string escape_string(const string &s) const;

private:
    MYSQL *conn;
};

MYSQL *connect_db(const string &host, const string &user, const string &password, const string &dbname = "");

#endif // DB_MANAGER_H
