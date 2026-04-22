#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include "db_manager.h"
#include <memory>
#include <string>
using namespace std;

/**
 * @brief 应用上下文
 *
 * 统一管理数据库连接和全局配置。
 * 视图层不再直接创建数据库连接，而是通过 AppContext 获取已配置好的连接。
 */
class AppContext
{
public:
    /**
     * @brief 创建管理员数据库连接（全权限）
     * @return 数据库管理器实例
     */
    static unique_ptr<DatabaseManager> createAdminDB();

    /**
     * @brief 创建普通用户数据库连接（受限权限）
     * @return 数据库管理器实例
     */
    static unique_ptr<DatabaseManager> createUserDB();

private:
    AppContext() = delete;
};

#endif // APP_CONTEXT_H
