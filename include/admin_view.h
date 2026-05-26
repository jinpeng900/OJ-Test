#ifndef ADMIN_VIEW_H
#define ADMIN_VIEW_H

#include "view_manager.h"
#include "db_manager.h"
#include "admin.h"
#include <memory>

/**
 * @brief ImGui 管理员界面
 */
class AdminView
{
public:
    AdminView();
    ~AdminView();

    void draw_login(AppState &st);
    void draw_menu(AppState &st);
    void draw_add_problem(AppState &st);

private:
    std::unique_ptr<DatabaseManager> db_;
    std::unique_ptr<Admin> admin_;

    void init_db();
};

#endif // ADMIN_VIEW_H
