#include "../include/admin_view.h"
#include <imgui.h>
#include <cstdlib>
#include <string>
#include <cstdio>

using namespace std;

// ============================================================
// 辅助
// ============================================================

void AdminView::init_db()
{
    if (!db_)
    {
        const char *host = getenv("OJ_DB_HOST");
        db_ = make_unique<DatabaseManager>((host && host[0]) ? host : "127.0.0.1", "oj_admin", "090800", "OJ");
        admin_ = make_unique<Admin>(db_.get());
    }
}

// ============================================================
// 构造 / 析构
// ============================================================

AdminView::AdminView() = default;
AdminView::~AdminView() = default;

// ============================================================
// 管理员登录
// ============================================================

void AdminView::draw_login(AppState &st)
{
    CenteredContent(400, 200);

    ImGui::Text("管理员登录");
    ImGui::Separator();
    ImGui::Text("%s", st.status);

    ImGui::PushItemWidth(CenteredTargetW());
    ImGui::InputText("账号", st.account, sizeof(st.account));
    ImGui::InputText("密码", st.password, sizeof(st.password),
                     ImGuiInputTextFlags_Password);
    ImGui::PopItemWidth();

    float btn_w = 120 * UI_SCALE;
    float total_w = btn_w * 2 + ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (CenteredTargetW() - total_w) * 0.5f);
    if (ImGui::Button("登录", SZ(120, 40)))
    {
        if (string(st.account) == "admin" && string(st.password) == "admin123")
        {
            st.set_status("管理员登录成功");
            st.state = GUIState::ADMIN_MENU;
        }
        else
        {
            st.set_status("管理员账号或密码错误");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("返回", SZ(120, 40)))
    {
        st.state = GUIState::MAIN_MENU;
        st.set_status("欢迎来到 OJ 在线判题系统");
    }

    EndCenteredContent(400);
}

// ============================================================
// 管理员菜单
// ============================================================

void AdminView::draw_menu(AppState &st)
{
    CenteredContent(400, 260);

    ImGui::Text("管理员模式");
    ImGui::Separator();
    ImGui::Text("%s", st.status);

    if (ImGui::Button("查看所有题目", CW(50)))
    {
        init_db();
        st.problems = db_->query(
            "SELECT id, title, category, time_limit, memory_limit FROM problems ORDER BY id");
        st.state = GUIState::PROBLEM_LIST;
    }
    if (ImGui::Button("查看题目详情", CW(50)))
    {
        st.problem_id = 0;
        st.state = GUIState::PROBLEM_DETAIL;
    }
    if (ImGui::Button("添加题目", CW(50)))
    {
        st.problem_title[0] = '\0';
        st.problem_category[0] = '\0';
        st.problem_test_path[0] = '\0';
        st.problem_description[0] = '\0';
        st.problem_time_limit = 1000;
        st.problem_memory_limit = 128;
        st.state = GUIState::ADMIN_ADD_PROBLEM;
        st.set_status("填写题目信息");
    }
    if (ImGui::Button("返回", CW(50)))
    {
        admin_.reset();
        db_.reset();
        st.state = GUIState::MAIN_MENU;
        st.set_status("欢迎来到 OJ 在线判题系统");
    }

    EndCenteredContent(400);
}

// ============================================================
// 添加题目
// ============================================================

void AdminView::draw_add_problem(AppState &st)
{
    CenteredContent(760, 520);

    ImGui::Text("添加题目");
    ImGui::Separator();
    ImGui::Text("%s", st.status);

    ImGui::PushItemWidth(CenteredTargetW());
    ImGui::InputText("标题", st.problem_title, sizeof(st.problem_title));
    ImGui::InputText("知识点", st.problem_category, sizeof(st.problem_category));
    ImGui::InputInt("时间限制(ms)", &st.problem_time_limit);
    ImGui::InputInt("内存限制(MB)", &st.problem_memory_limit);
    ImGui::InputText("测试数据路径", st.problem_test_path, sizeof(st.problem_test_path));
    ImGui::InputTextMultiline("题目描述", st.problem_description, sizeof(st.problem_description),
                              ImVec2(0, 220 * UI_SCALE));
    ImGui::PopItemWidth();

    float btn_w = 120 * UI_SCALE;
    float total_w = btn_w * 2 + ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (CenteredTargetW() - total_w) * 0.5f);
    if (ImGui::Button("添加", SZ(120, 40)))
    {
        init_db();
        string title = st.problem_title;
        string desc = st.problem_description;
        string category = st.problem_category;
        string test_path = st.problem_test_path;

        if (title.empty() || desc.empty() || test_path.empty())
        {
            st.set_status("标题、题目描述和测试数据路径不能为空");
        }
        else if (st.problem_time_limit <= 0 || st.problem_memory_limit <= 0)
        {
            st.set_status("时间限制和内存限制必须大于 0");
        }
        else
        {
            string sql = "INSERT INTO problems "
                         "(title, description, time_limit, memory_limit, test_data_path, category) VALUES ('" +
                         db_->escape_string(title) + "', '" +
                         db_->escape_string(desc) + "', " +
                         to_string(st.problem_time_limit) + ", " +
                         to_string(st.problem_memory_limit) + ", '" +
                         db_->escape_string(test_path) + "', '" +
                         db_->escape_string(category) + "')";

            if (admin_->add_problem(sql))
            {
                snprintf(st.problem_title, sizeof(st.problem_title), "%s", "");
                snprintf(st.problem_category, sizeof(st.problem_category), "%s", "");
                snprintf(st.problem_test_path, sizeof(st.problem_test_path), "%s", "");
                snprintf(st.problem_description, sizeof(st.problem_description), "%s", "");
                st.problem_time_limit = 1000;
                st.problem_memory_limit = 128;
                st.set_status("题目添加成功");
            }
            else
            {
                st.set_status("添加失败，请检查数据库或测试数据路径");
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("返回", SZ(120, 40)))
    {
        st.state = GUIState::ADMIN_MENU;
    }

    EndCenteredContent(500);
}
