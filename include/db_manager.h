#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <mysql/mysql.h>
#include <string>

/**
 * @brief 连接 MySQL 数据库
 * @param host 主机名
 * @param user 用户名
 * @param password 密码
 * @param dbname 数据库名
 * @return 成功返回 MYSQL* 指针，失败返回 nullptr
 */
MYSQL* connect_db(const std::string& host, const std::string& user, const std::string& password, const std::string& dbname = "");

/**
 * @brief 执行 SQL 语句并打印结果
 * @param conn 数据库连接句柄
 * @param sql 要执行的 SQL 语句
 */
void run_sql(MYSQL* conn, const std::string& sql);

#endif // DB_MANAGER_H
