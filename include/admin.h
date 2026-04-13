#ifndef ADMIN_H
#define ADMIN_H

#include "db_manager.h"
#include <string>

/**
 * @brief 管理员类，负责处理管理员特有的业务逻辑
 */
class Admin {
public:
    /**
     * @brief 构造函数：关联一个数据库管理器
     */
    Admin(DatabaseManager* db);

    /**
     * @brief 发布新题目 (执行 SQL)
     * @param sql 管理员输入的 SQL 语句
     * @return 执行结果
     */
    bool add_problem(const std::string& sql);

    /**
     * @brief 查看所有题目
     */
    void list_all_problems();

    /**
     * @brief 查看单个题目详情
     * @param id 题目 ID
     */
    void show_problem_detail(int id);

private:
    DatabaseManager* db_manager;
};

#endif // ADMIN_H
