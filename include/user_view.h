#ifndef USER_VIEW_H
#define USER_VIEW_H

#include "view_manager.h"
#include "db_manager.h"
#include "user.h"
#include "ai_client.h"
#include <memory>

/**
 * @brief ImGui 用户界面
 */
class UserView
{
public:
    UserView();
    ~UserView();

    void draw_login(AppState &st);
    void draw_register(AppState &st);
    void draw_menu(AppState &st);
    void draw_problem_list(AppState &st);
    void draw_problem_detail(AppState &st);
    void draw_submit_result(AppState &st);
    void draw_submissions_list(AppState &st);
    void draw_submission_detail(AppState &st);
    void draw_change_password(AppState &st);
    void draw_ai_assistant(AppState &st);

    bool is_logged_in() const { return user_ && user_->is_logged_in(); }
    std::string get_account() const { return user_ ? user_->get_current_account() : ""; }
    int get_user_id() const { return user_ ? user_->get_current_user_id() : -1; }

private:
    std::unique_ptr<DatabaseManager> db_;
    std::unique_ptr<User> user_;
    std::unique_ptr<AIClient> ai_;

    void init_db();
    void load_problems(AppState &st);
    void load_submissions(AppState &st);
    void load_problem_detail(AppState &st, int id);
    static std::string read_file(const std::string &path);
};

#endif // USER_VIEW_H
