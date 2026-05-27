#include "../include/admin_view.h"
#include <imgui.h>
#include <cstdlib>
#include <string>
#include <cstdio>

using namespace std;

// ============================================================
// 辅助
// ============================================================

static bool labeled_input(const char *label, const char *id, char *buf, size_t size,
                          ImGuiInputTextFlags flags = 0, float width = -1.0f)
{
    ImGui::Text("%s", label);
    if (width > 0)
        ImGui::SetNextItemWidth(width);
    else
        ImGui::SetNextItemWidth(-1);
    return ImGui::InputText(id, buf, size, flags);
}

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
    CenteredContent(430, 230);

    ImGui::Text("管理员登录");
    ImGui::TextDisabled("管理员可以维护题目和查看题库");
    ImGui::Separator();
    ImGui::TextWrapped("%s", st.status);
    ImGui::Spacing();

    labeled_input("账号", "##admin_login_account", st.account, sizeof(st.account), 0, CenteredTargetW());
    labeled_input("密码", "##admin_login_password", st.password, sizeof(st.password),
                  ImGuiInputTextFlags_Password, CenteredTargetW());

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

    EndCenteredContent(430);
}

// ============================================================
// 管理员菜单
// ============================================================

void AdminView::draw_menu(AppState &st)
{
    ImGui::Text("管理员工作台");
    ImGui::SameLine();
    ImGui::TextDisabled("题库维护入口");
    ImGui::Separator();

    if (ImGui::BeginTable("admin_dashboard", 2, ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("nav", ImGuiTableColumnFlags_WidthFixed, 260 * UI_SCALE);
        ImGui::TableSetupColumn("content", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::BeginChild("admin_nav", ImVec2(0, 0), true);
        ImGui::TextDisabled("导航");
        ImGui::Separator();
        if (ImGui::Button("查看所有题目", ImVec2(-1, 48 * UI_SCALE)))
        {
            init_db();
            st.problems = db_->query(
                "SELECT id, title, category, time_limit, memory_limit FROM problems ORDER BY id");
            st.state = GUIState::ADMIN_PROBLEM_LIST;
        }
        if (ImGui::Button("查看题目详情", ImVec2(-1, 48 * UI_SCALE)))
        {
            st.problem_id = 0;
            st.cur_problem.clear();
            st.state = GUIState::ADMIN_PROBLEM_DETAIL;
        }
        if (ImGui::Button("添加题目", ImVec2(-1, 48 * UI_SCALE)))
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
        ImGui::Dummy(ImVec2(0, 12 * UI_SCALE));
        if (ImGui::Button("退出管理", ImVec2(-1, 42 * UI_SCALE)))
        {
            admin_.reset();
            db_.reset();
            st.state = GUIState::MAIN_MENU;
            st.set_status("欢迎来到 OJ 在线判题系统");
        }
        ImGui::EndChild();

        ImGui::TableNextColumn();
        ImGui::BeginChild("admin_home", ImVec2(0, 0), true);
        ImGui::Text("当前状态");
        ImGui::Separator();
        ImGui::TextWrapped("%s", st.status);
        ImGui::Spacing();
        ImGui::TextDisabled("维护说明");
        ImGui::BulletText("查看所有题目用于快速浏览题库。");
        ImGui::BulletText("查看题目详情用于按 ID 检查题面、限制和测试数据路径。");
        ImGui::BulletText("添加题目前请确认 data/<题号>/ 下测试点已准备好。");
        ImGui::EndChild();

        ImGui::EndTable();
    }
}

// ============================================================
// 管理员题目列表 / 详情
// ============================================================

void AdminView::draw_problem_list(AppState &st)
{
    ImGui::Text("题目管理");
    ImGui::Separator();

    if (ImGui::Button("返回", SZ(80, 28)))
    {
        st.state = GUIState::ADMIN_MENU;
        return;
    }
    ImGui::SameLine();
    if (ImGui::Button("刷新", SZ(80, 28)))
    {
        init_db();
        st.problems = db_->query(
            "SELECT id, title, category, time_limit, memory_limit FROM problems ORDER BY id");
    }
    ImGui::SameLine();
    if (ImGui::Button("添加题目", SZ(100, 28)))
    {
        st.problem_title[0] = '\0';
        st.problem_category[0] = '\0';
        st.problem_test_path[0] = '\0';
        st.problem_description[0] = '\0';
        st.problem_time_limit = 1000;
        st.problem_memory_limit = 128;
        st.state = GUIState::ADMIN_ADD_PROBLEM;
        st.set_status("填写题目信息");
        return;
    }

    ImGui::TextDisabled("共 %d 题，点击题目标题查看详情",
                        static_cast<int>(st.problems.size()));
    ImGui::Spacing();

    if (!ImGui::BeginTable("admin_problems", 5,
                           ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                               ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
        return;

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 56 * UI_SCALE);
    ImGui::TableSetupColumn("标题", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("知识点", ImGuiTableColumnFlags_WidthFixed, 140 * UI_SCALE);
    ImGui::TableSetupColumn("时间", ImGuiTableColumnFlags_WidthFixed, 90 * UI_SCALE);
    ImGui::TableSetupColumn("内存", ImGuiTableColumnFlags_WidthFixed, 90 * UI_SCALE);
    ImGui::TableHeadersRow();

    for (const auto &p : st.problems)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%s", p.at("id").c_str());
        ImGui::TableNextColumn();
        string selectable = p.at("title") + "##admin_problem_" + p.at("id");
        if (ImGui::Selectable(selectable.c_str(), false, ImGuiSelectableFlags_SpanAllColumns))
        {
            init_db();
            auto rows = db_->query(
                "SELECT id, title, description, time_limit, memory_limit, category, test_data_path "
                "FROM problems WHERE id = " +
                p.at("id"));
            if (!rows.empty())
                st.cur_problem = rows[0];
            st.state = GUIState::ADMIN_PROBLEM_DETAIL;
        }
        ImGui::TableNextColumn();
        ImGui::Text("%s", p.at("category").c_str());
        ImGui::TableNextColumn();
        ImGui::Text("%sms", p.at("time_limit").c_str());
        ImGui::TableNextColumn();
        ImGui::Text("%sMB", p.at("memory_limit").c_str());
    }
    ImGui::EndTable();
}

void AdminView::draw_problem_detail(AppState &st)
{
    ImGui::Text("题目详情");
    ImGui::Separator();

    if (ImGui::Button("返回", SZ(80, 28)))
    {
        st.state = GUIState::ADMIN_MENU;
        return;
    }
    ImGui::SameLine();
    if (ImGui::Button("返回列表", SZ(100, 28)))
    {
        init_db();
        st.problems = db_->query(
            "SELECT id, title, category, time_limit, memory_limit FROM problems ORDER BY id");
        st.state = GUIState::ADMIN_PROBLEM_LIST;
        return;
    }

    ImGui::Spacing();
    if (st.cur_problem.empty())
    {
        ImGui::Text("输入题目 ID 查询:");
        ImGui::SetNextItemWidth(180 * UI_SCALE);
        ImGui::InputInt("##admin_problem_id", &st.problem_id);
        ImGui::SameLine();
        if (ImGui::Button("查询", SZ(80, 28)) && st.problem_id > 0)
        {
            init_db();
            auto rows = db_->query(
                "SELECT id, title, description, time_limit, memory_limit, category, test_data_path "
                "FROM problems WHERE id = " +
                to_string(st.problem_id));
            if (!rows.empty())
            {
                st.cur_problem = rows[0];
                st.set_status("题目已加载");
            }
            else
            {
                st.set_status("未找到该题目");
            }
        }
        return;
    }

    float h = ImGui::GetContentRegionAvail().y;
    if (h < 360 * UI_SCALE)
        h = 360 * UI_SCALE;

    ImGui::BeginChild("admin_problem_detail", ImVec2(0, h), true);
    ImGui::TextColored(ImVec4(0.35f, 0.70f, 1.0f, 1.0f), "#%s  %s",
                       st.cur_problem["id"].c_str(),
                       st.cur_problem["title"].c_str());
    ImGui::Separator();
    ImGui::Text("知识点: %s", st.cur_problem["category"].c_str());
    ImGui::Text("时间限制: %s ms", st.cur_problem["time_limit"].c_str());
    ImGui::Text("内存限制: %s MB", st.cur_problem["memory_limit"].c_str());
    ImGui::Text("测试数据路径: %s", st.cur_problem["test_data_path"].c_str());
    ImGui::Spacing();
    ImGui::TextDisabled("题目描述");
    ImGui::Separator();
    ImGui::TextWrapped("%s", st.cur_problem["description"].c_str());
    ImGui::EndChild();
}

// ============================================================
// 添加题目
// ============================================================

void AdminView::draw_add_problem(AppState &st)
{
    ImGui::Text("添加题目");
    ImGui::Separator();
    ImGui::Text("%s", st.status);
    ImGui::Spacing();

    float form_h = ImGui::GetContentRegionAvail().y - 54 * UI_SCALE;
    if (form_h < 380 * UI_SCALE)
        form_h = 380 * UI_SCALE;

    ImGui::BeginChild("add_problem_form", ImVec2(0, form_h), true);

    if (ImGui::BeginTable("add_problem_layout", 2,
                          ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("基础信息", ImGuiTableColumnFlags_WidthStretch, 0.40f);
        ImGui::TableSetupColumn("题目描述", ImGuiTableColumnFlags_WidthStretch, 0.60f);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::TextDisabled("基础信息");
        ImGui::Separator();
        ImGui::Text("标题");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##problem_title", st.problem_title, sizeof(st.problem_title));
        ImGui::Spacing();

        ImGui::Text("知识点");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##problem_category", st.problem_category, sizeof(st.problem_category));
        ImGui::Spacing();

        ImGui::Text("时间限制(ms)");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputInt("##problem_time_limit", &st.problem_time_limit);
        ImGui::Spacing();

        ImGui::Text("内存限制(MB)");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputInt("##problem_memory_limit", &st.problem_memory_limit);
        ImGui::Spacing();

        ImGui::Text("测试数据路径");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##problem_test_path", st.problem_test_path, sizeof(st.problem_test_path));
        ImGui::Spacing();
        ImGui::TextWrapped("测试数据目录示例: data/9，目录内包含 1.in/1.out 等测试点。");

        ImGui::TableNextColumn();
        ImGui::TextDisabled("题目描述");
        ImGui::Separator();
        ImGui::InputTextMultiline("##problem_description", st.problem_description,
                                  sizeof(st.problem_description),
                                  ImVec2(-1, form_h - 54 * UI_SCALE));
        ImGui::EndTable();
    }

    ImGui::EndChild();

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
}
