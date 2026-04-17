#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <map>

/**
 * @brief 数据库管理类，封装连接与 SQL 执行
 */
class DatabaseManager
{
public:
    /**
     * @brief 构造函数：初始化并连接数据库
     */
    DatabaseManager(const std::string &host, const std::string &user, const std::string &password, const std::string &dbname = "");

    /**
     * @brief 析构函数：释放资源
     */
    ~DatabaseManager();

    /**
     * @brief 获取连接句柄
     */
    MYSQL *get_connection() const { return conn; }

    /**
     * @brief 执行 SQL 语句并打印结果
     * @param sql 要执行的 SQL 语句
     * @return 成功返回 true，失败返回 false
     */
    bool run_sql(const std::string &sql);

    /**
     * @brief 执行查询语句并返回结果集
     * @param sql 查询语句
     * @return 结果集，每行是一个 map (列名 -> 值)
     */
    std::vector<std::map<std::string, std::string>> query(const std::string &sql);

private:
    MYSQL *conn;
};

// 保留全局函数，供向下兼容或简单使用（可根据需要移除）
MYSQL *connect_db(const std::string &host, const std::string &user, const std::string &password, const std::string &dbname = "");
bool run_sql(MYSQL *conn, const std::string &sql);

#endif // DB_MANAGER_H
