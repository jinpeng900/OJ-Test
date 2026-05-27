#include "../include/user_view.h"
#include <imgui.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <cstdlib>

using namespace std;

// ============================================================
// 辅助
// ============================================================

string UserView::read_file(const string &path)
{
    ifstream f(path);
    if (!f.is_open())
        return "";
    stringstream buf;
    buf << f.rdbuf();
    return buf.str();
}

static bool write_file(const string &path, const string &content)
{
    ofstream f(path);
    if (!f.is_open())
        return false;
    f << content;
    return f.good();
}

static string db_host()
{
    const char *host = getenv("OJ_DB_HOST");
    return (host && host[0]) ? host : "127.0.0.1";
}

static ImVec4 status_color(const string &status)
{
    if (status == "AC")
        return ImVec4(0.20f, 0.85f, 0.35f, 1.0f);
    if (status == "TLE" || status == "MLE")
        return ImVec4(1.0f, 0.65f, 0.15f, 1.0f);
    if (status == "CE")
        return ImVec4(0.95f, 0.35f, 0.95f, 1.0f);
    if (status == "WA" || status == "RE" || status == "SE")
        return ImVec4(1.0f, 0.25f, 0.25f, 1.0f);
    return ImVec4(0.80f, 0.80f, 0.80f, 1.0f);
}

static string judge_status_text(JudgeResult result)
{
    switch (result)
    {
    case JudgeResult::ACCEPTED:
        return "Accepted (AC)";
    case JudgeResult::WRONG_ANSWER:
        return "Wrong Answer (WA)";
    case JudgeResult::TIME_LIMIT_EXCEEDED:
        return "Time Limit Exceeded (TLE)";
    case JudgeResult::MEMORY_LIMIT_EXCEEDED:
        return "Memory Limit Exceeded (MLE)";
    case JudgeResult::RUNTIME_ERROR:
        return "Runtime Error (RE)";
    case JudgeResult::COMPILE_ERROR:
        return "Compile Error (CE)";
    case JudgeResult::SYSTEM_ERROR:
        return "System Error (SE)";
    default:
        return "Unknown";
    }
}

static string judge_status_code(JudgeResult result)
{
    switch (result)
    {
    case JudgeResult::ACCEPTED:
        return "AC";
    case JudgeResult::WRONG_ANSWER:
        return "WA";
    case JudgeResult::TIME_LIMIT_EXCEEDED:
        return "TLE";
    case JudgeResult::MEMORY_LIMIT_EXCEEDED:
        return "MLE";
    case JudgeResult::RUNTIME_ERROR:
        return "RE";
    case JudgeResult::COMPILE_ERROR:
        return "CE";
    case JudgeResult::SYSTEM_ERROR:
        return "SE";
    default:
        return "??";
    }
}

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

void UserView::init_db()
{
    if (!db_)
    {
        db_ = make_unique<DatabaseManager>(db_host(), "oj_user", "user123", "OJ");
        user_ = make_unique<User>(db_.get());
        ai_ = make_unique<AIClient>();
    }
}

void UserView::load_problems(AppState &st)
{
    init_db();
    st.problems = db_->query(
        "SELECT id, title, category, time_limit, memory_limit FROM problems ORDER BY id");

    // 加载当前用户已解决的题目 ID
    st.solved_problems.clear();
    if (user_ && user_->is_logged_in())
    {
        auto solved = db_->query(
            "SELECT DISTINCT problem_id FROM submissions WHERE user_id = " +
            std::to_string(user_->get_current_user_id()) + " AND status = 'AC'");
        for (const auto &row : solved)
        {
            st.solved_problems.insert(stoi(row.at("problem_id")));
        }
    }
}

void UserView::load_submissions(AppState &st)
{
    init_db();
    if (user_ && user_->is_logged_in())
    {
        st.submissions = db_->query(
            "SELECT s.id, s.problem_id, p.title, s.status, s.submit_time AS created_at, s.code "
            "FROM submissions s JOIN problems p ON s.problem_id = p.id "
            "WHERE s.user_id = " +
            to_string(user_->get_current_user_id()) +
            " ORDER BY s.id DESC LIMIT 50");
    }
}

void UserView::load_problem_detail(AppState &st, int id)
{
    init_db();
    auto rows = db_->query(
        "SELECT id, title, description, time_limit, memory_limit, category, test_data_path "
        "FROM problems WHERE id = " +
        to_string(id));
    if (!rows.empty())
        st.cur_problem = rows[0];
}

// ============================================================
// 构造 / 析构
// ============================================================

UserView::UserView() = default;
UserView::~UserView() = default;

// ============================================================
// 登录 / 注册
// ============================================================

void UserView::draw_login(AppState &st)
{
    CenteredContent(430, 250);

    ImGui::Text("用户登录");
    ImGui::TextDisabled("登录后可以浏览题目、提交代码和查看记录");
    ImGui::Separator();
    ImGui::TextWrapped("%s", st.status);
    ImGui::Spacing();

    labeled_input("账号", "##user_login_account", st.account, sizeof(st.account), 0, CenteredTargetW());
    labeled_input("密码", "##user_login_password", st.password, sizeof(st.password),
                  ImGuiInputTextFlags_Password, CenteredTargetW());

    float btn_w = 122 * UI_SCALE;
    float total_w = btn_w * 3 + ImGui::GetStyle().ItemSpacing.x * 2;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (CenteredTargetW() - total_w) * 0.5f);
    if (ImGui::Button("登录", SZ(122, 40)))
    {
        init_db();
        if (user_->login(st.account, st.password))
        {
            st.set_status("欢迎，" + string(st.account));
            st.state = GUIState::USER_MENU;
        }
        else
        {
            st.set_status("登录失败，请检查账号密码");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("注册", SZ(122, 40)))
    {
        st.state = GUIState::USER_REGISTER;
        st.set_status("注册新账号");
    }
    ImGui::SameLine();
    if (ImGui::Button("返回", SZ(122, 40)))
    {
        st.state = GUIState::MAIN_MENU;
        st.set_status("欢迎来到 OJ 在线判题系统");
    }

    EndCenteredContent(430);
}

void UserView::draw_register(AppState &st)
{
    CenteredContent(430, 220);

    ImGui::Text("注册新账号");
    ImGui::TextDisabled("账号创建后会自动生成独立代码工作区");
    ImGui::Separator();
    ImGui::TextWrapped("%s", st.status);
    ImGui::Spacing();

    labeled_input("账号", "##user_register_account", st.account, sizeof(st.account), 0, CenteredTargetW());
    labeled_input("密码", "##user_register_password", st.password, sizeof(st.password),
                  ImGuiInputTextFlags_Password, CenteredTargetW());

    float btn_w = 120 * UI_SCALE;
    float total_w = btn_w * 2 + ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (CenteredTargetW() - total_w) * 0.5f);
    if (ImGui::Button("确认注册", SZ(120, 40)))
    {
        init_db();
        if (user_->register_user(st.account, st.password))
        {
            st.set_status("注册成功，请登录");
            st.state = GUIState::USER_LOGIN;
        }
        else
        {
            st.set_status("注册失败：账号已存在");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("返回", SZ(120, 40)))
    {
        st.state = GUIState::USER_LOGIN;
    }

    EndCenteredContent(430);
}

// ============================================================
// 用户菜单
// ============================================================

void UserView::draw_menu(AppState &st)
{
    ImGui::Text("用户工作台");
    if (user_ && user_->is_logged_in())
    {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", user_->get_current_account().c_str());
    }
    ImGui::Separator();

    if (ImGui::BeginTable("user_dashboard", 2, ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("nav", ImGuiTableColumnFlags_WidthFixed, 260 * UI_SCALE);
        ImGui::TableSetupColumn("content", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::BeginChild("user_nav", ImVec2(0, 0), true);
        ImGui::TextDisabled("导航");
        ImGui::Separator();
        if (ImGui::Button("题目列表", ImVec2(-1, 48 * UI_SCALE)))
        {
            load_problems(st);
            st.state = GUIState::PROBLEM_LIST;
        }
        if (ImGui::Button("我的提交", ImVec2(-1, 48 * UI_SCALE)))
        {
            load_submissions(st);
            st.state = GUIState::SUBMISSIONS_LIST;
        }
        if (ImGui::Button("修改密码", ImVec2(-1, 48 * UI_SCALE)))
        {
            st.old_pwd[0] = st.new_pwd[0] = '\0';
            st.state = GUIState::CHANGE_PASSWORD;
        }
        ImGui::Dummy(ImVec2(0, 12 * UI_SCALE));
        if (ImGui::Button("退出登录", ImVec2(-1, 42 * UI_SCALE)))
        {
            user_.reset();
            db_.reset();
            ai_.reset();
            st.state = GUIState::MAIN_MENU;
            st.set_status("欢迎来到 OJ 在线判题系统");
        }
        ImGui::EndChild();

        ImGui::TableNextColumn();
        ImGui::BeginChild("user_home", ImVec2(0, 0), true);
        ImGui::Text("当前状态");
        ImGui::Separator();
        ImGui::TextWrapped("%s", st.status);
        ImGui::Spacing();
        ImGui::TextDisabled("常用流程");
        ImGui::BulletText("进入题目列表，选择题目后在右侧代码区编写并提交。");
        ImGui::BulletText("提交完成后可在我的提交中查看状态、原因和代码。");
        ImGui::BulletText("评测失败后可携带题目和代码上下文询问 AI 助手。");
        ImGui::EndChild();

        ImGui::EndTable();
    }
}

// ============================================================
// 题目列表
// ============================================================

void UserView::draw_problem_list(AppState &st)
{
    ImGui::Text("题目列表");
    ImGui::Separator();

    if (ImGui::Button("返回", SZ(80, 25)))
    {
        st.state = GUIState::USER_MENU;
        return;
    }
    ImGui::SameLine();

    ImGui::TextDisabled("共 %d 题，已完成 %d 题",
                        static_cast<int>(st.problems.size()),
                        static_cast<int>(st.solved_problems.size()));
    ImGui::Spacing();

    ImGui::SetNextItemWidth(280 * UI_SCALE);
    ImGui::InputText("##search", st.search_buf, sizeof(st.search_buf),
                     ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    ImGui::TextDisabled("搜索标题");
    ImGui::SameLine();
    if (ImGui::Button("刷新", SZ(70, 25)))
    {
        load_problems(st);
    }
    ImGui::SameLine();
    if (ImGui::Button("AI 助手", SZ(80, 25)))
    {
        st.ai_list_mode = true;
        st.chat_history.clear();
        st.question[0] = '\0';
        st.state = GUIState::AI_ASSISTANT;
    }

    ImGui::Spacing();
    if (!ImGui::BeginTable("problems", 6,
                           ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                               ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
        return;

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("状态", ImGuiTableColumnFlags_WidthFixed, 72 * UI_SCALE);
    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 48 * UI_SCALE);
    ImGui::TableSetupColumn("标题", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("知识点", ImGuiTableColumnFlags_WidthFixed, 120 * UI_SCALE);
    ImGui::TableSetupColumn("时间", ImGuiTableColumnFlags_WidthFixed, 80 * UI_SCALE);
    ImGui::TableSetupColumn("内存", ImGuiTableColumnFlags_WidthFixed, 80 * UI_SCALE);
    ImGui::TableHeadersRow();

    string search_key = st.search_buf;
    for (char &c : search_key)
        c = tolower(c);

    for (const auto &p : st.problems)
    {
        // 按题目名称过滤
        if (!search_key.empty())
        {
            string title_lower = p.at("title");
            for (char &c : title_lower)
                c = tolower(c);
            if (title_lower.find(search_key) == string::npos)
                continue;
        }

        int pid = stoi(p.at("id"));
        bool is_solved = st.solved_problems.count(pid) > 0;

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (is_solved)
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            float size = 10.0f * UI_SCALE;
            float pad_y = (ImGui::GetTextLineHeight() - size) * 0.5f;
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(pos.x, pos.y + pad_y),
                ImVec2(pos.x + size, pos.y + pad_y + size),
                IM_COL32(60, 210, 90, 255),
                2.0f * UI_SCALE);
            ImGui::Dummy(ImVec2(size + 4.0f * UI_SCALE, ImGui::GetTextLineHeight()));
            ImGui::SameLine();
            ImGui::TextColored(status_color("AC"), "完成");
        }
        else
        {
            ImGui::TextDisabled("未做");
        }

        ImGui::TableNextColumn();
        ImGui::Text("%s", p.at("id").c_str());
        ImGui::TableNextColumn();
        string selectable_id = p.at("title") + "##problem_" + p.at("id");
        if (ImGui::Selectable(selectable_id.c_str(), false,
                              ImGuiSelectableFlags_SpanAllColumns))
        {
            st.problem_id = pid;
            st.code_buf[0] = '\0';
            load_problem_detail(st, st.problem_id);
            st.state = GUIState::PROBLEM_DETAIL;
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

// ============================================================
// 题目详情
// ============================================================

void UserView::draw_problem_detail(AppState &st)
{
    ImGui::Text("题目详情");
    ImGui::Separator();

    if (ImGui::Button("返回", SZ(80, 25)))
    {
        st.state = GUIState::USER_MENU;
        return;
    }

    if (!st.cur_problem.empty())
    {
        bool can_submit = user_ && user_->is_logged_in();
        string workspace;
        if (can_submit)
        {
            workspace = "workspace/" + to_string(user_->get_current_user_id()) +
                        "/solution.cpp";
            if (st.code_buf[0] == '\0')
            {
                string code = read_file(workspace);
                snprintf(st.code_buf, sizeof(st.code_buf), "%s", code.c_str());
            }
        }

        float detail_h = ImGui::GetContentRegionAvail().y - 8.0f * UI_SCALE;
        if (detail_h < 360.0f * UI_SCALE)
            detail_h = 360.0f * UI_SCALE;

        if (ImGui::BeginTable("problem_detail_layout", 2,
                              ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("题面", ImGuiTableColumnFlags_WidthStretch, 0.42f);
            ImGui::TableSetupColumn("代码", ImGuiTableColumnFlags_WidthStretch, 0.58f);
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::BeginChild("problem_info_panel", ImVec2(0, detail_h), true);
            ImGui::TextColored(ImVec4(0.35f, 0.70f, 1.0f, 1.0f), "#%s  %s",
                               st.cur_problem["id"].c_str(),
                               st.cur_problem["title"].c_str());
            ImGui::Separator();
            ImGui::Text("知识点: %s", st.cur_problem["category"].c_str());
            ImGui::Text("时间限制: %s ms", st.cur_problem["time_limit"].c_str());
            ImGui::Text("内存限制: %s MB", st.cur_problem["memory_limit"].c_str());
            ImGui::Spacing();
            ImGui::TextDisabled("题目描述");
            ImGui::Separator();
            ImGui::TextWrapped("%s", st.cur_problem["description"].c_str());
            if (st.cur_problem.count("test_data_path"))
            {
                ImGui::Spacing();
                ImGui::TextDisabled("测试数据");
                ImGui::Separator();
                ImGui::TextWrapped("%s", st.cur_problem["test_data_path"].c_str());
            }
            ImGui::EndChild();

            ImGui::TableNextColumn();
            ImGui::BeginChild("code_panel", ImVec2(0, detail_h), true);
            ImGui::Text("代码编辑（C++17）");
            ImGui::SameLine();
            if (!can_submit)
                ImGui::TextDisabled("只读预览");
            ImGui::Separator();

            ImGui::InputTextMultiline("##code", st.code_buf, sizeof(st.code_buf),
                                      ImVec2(-1, detail_h - 96 * UI_SCALE),
                                      ImGuiInputTextFlags_AllowTabInput);

            if (can_submit && ImGui::Button("保存代码", SZ(110, 34)))
            {
                if (write_file(workspace, st.code_buf))
                    st.set_status("代码已保存到 " + workspace);
                else
                    st.set_status("保存失败：" + workspace);
            }
            if (can_submit)
            {
                ImGui::SameLine();
                if (ImGui::Button("提交代码", SZ(110, 34)))
                {
                    string code = read_file(workspace);
                    if (code.empty())
                    {
                        st.set_status(workspace + " 为空，请先编写代码");
                    }
                    else
                    {
                        int pid = stoi(st.cur_problem["id"]);
                        user_->submit_code(pid, code, "C++");
                        st.last_report = user_->getLastReport();
                        st.state = GUIState::SUBMIT_RESULT;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("AI 助手", SZ(110, 34)))
                {
                    st.ai_list_mode = false;
                    st.chat_history.clear();
                    st.question[0] = '\0';
                    st.state = GUIState::AI_ASSISTANT;
                }
            }
            ImGui::EndChild();
            ImGui::EndTable();
        }
    }
    else
    {
        ImGui::Text("请输入题目 ID:");
        ImGui::InputInt("", &st.problem_id);
        if (ImGui::Button("查询", SZ(80, 25)) && st.problem_id > 0)
        {
            load_problem_detail(st, st.problem_id);
            st.code_buf[0] = '\0';
        }
    }
}

// ============================================================
// 提交结果
// ============================================================

void UserView::draw_submit_result(AppState &st)
{
    ImGui::Text("评测结果");
    ImGui::Separator();

    if (ImGui::Button("返回", SZ(80, 25)))
    {
        st.state = GUIState::PROBLEM_DETAIL;
        return;
    }

    const auto &r = st.last_report;
    string status_code = judge_status_code(r.result);
    string rt = judge_status_text(r.result);

    ImGui::BeginChild("result_summary", ImVec2(0, 112 * UI_SCALE), true);
    ImGui::Text("状态");
    ImGui::SameLine();
    ImGui::TextColored(status_color(status_code), "%s", rt.c_str());
    ImGui::Spacing();
    ImGui::Text("通过测试点: %d / %d", r.passed_test_cases, r.total_test_cases);
    ImGui::SameLine();
    ImGui::Text("    时间: %d ms", r.time_used_ms);
    ImGui::SameLine();
    ImGui::Text("    内存: %d MB", r.memory_used_mb);
    if (!r.error_message.empty())
        ImGui::TextWrapped("信息: %s", r.error_message.c_str());
    ImGui::EndChild();
    ImGui::Spacing();

    if (r.result != JudgeResult::COMPILE_ERROR &&
        r.result != JudgeResult::SYSTEM_ERROR)
    {
        ImGui::TextDisabled("测试点详情");
        if (ImGui::BeginTable("details", 4,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 40 * UI_SCALE);
            ImGui::TableSetupColumn("结果", ImGuiTableColumnFlags_WidthFixed, 80 * UI_SCALE);
            ImGui::TableSetupColumn("时间", ImGuiTableColumnFlags_WidthFixed, 80 * UI_SCALE);
            ImGui::TableSetupColumn("内存", ImGuiTableColumnFlags_WidthFixed, 80 * UI_SCALE);
            ImGui::TableHeadersRow();
            for (const auto &d : r.details)
            {
                string tag = judge_status_code(d.result);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%d", d.case_id);
                ImGui::TableNextColumn();
                ImGui::TextColored(status_color(tag), "%s", tag.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%dms", d.time_ms);
                ImGui::TableNextColumn();
                ImGui::Text("%dMB", d.memory_mb);
            }
            ImGui::EndTable();
        }
    }
}

// ============================================================
// 提交记录
// ============================================================

void UserView::draw_submissions_list(AppState &st)
{
    ImGui::Text("我的提交记录");
    ImGui::Separator();

    if (ImGui::Button("返回", SZ(80, 25)))
    {
        st.state = GUIState::USER_MENU;
        return;
    }

    ImGui::TextDisabled("点击一条提交记录查看详情");

    if (!ImGui::BeginTable("submissions", 5,
                           ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                               ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
        return;

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 56 * UI_SCALE);
    ImGui::TableSetupColumn("题目ID", ImGuiTableColumnFlags_WidthFixed, 72 * UI_SCALE);
    ImGui::TableSetupColumn("题目", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("状态", ImGuiTableColumnFlags_WidthFixed, 72 * UI_SCALE);
    ImGui::TableSetupColumn("时间", ImGuiTableColumnFlags_WidthFixed, 170 * UI_SCALE);
    ImGui::TableHeadersRow();

    bool open_detail = false;
    for (const auto &s : st.submissions)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::Selectable(s.at("id").c_str(), false, ImGuiSelectableFlags_SpanAllColumns))
        {
            st.cur_submission = s;
            open_detail = true;
        }
        ImGui::TableNextColumn();
        ImGui::Text("%s", s.at("problem_id").c_str());
        ImGui::TableNextColumn();
        ImGui::Text("%s", s.at("title").c_str());
        ImGui::TableNextColumn();
        const string &status_str = s.at("status");
        ImGui::TextColored(status_color(status_str), "%s", status_str.c_str());
        ImGui::TableNextColumn();
        ImGui::Text("%s", s.at("created_at").c_str());
    }
    ImGui::EndTable();

    if (open_detail)
        st.state = GUIState::SUBMISSION_DETAIL;
}

static string submission_reason(const string &status)
{
    if (status == "AC")
        return "通过：程序输出与所有测试点期望输出一致。";
    if (status == "WA")
        return "答案错误：程序正常运行，但至少一个测试点的输出与标准答案不一致。";
    if (status == "TLE")
        return "运行超时：程序在某个测试点超过题目时间限制。";
    if (status == "MLE")
        return "内存超限：程序运行时内存使用超过题目限制。";
    if (status == "RE")
        return "运行时错误：程序运行过程中异常退出，常见原因包括数组越界、除零、非法内存访问等。";
    if (status == "CE")
        return "编译错误：代码未能通过 g++ 编译，请检查语法、头文件和类型错误。";
    if (status == "SE")
        return "系统错误：评测环境、测试数据或沙箱容器发生异常。";
    return "等待评测或未知状态。";
}

void UserView::draw_submission_detail(AppState &st)
{
    ImGui::Text("提交详情");
    ImGui::Separator();

    if (ImGui::Button("返回", SZ(80, 25)))
    {
        st.state = GUIState::SUBMISSIONS_LIST;
        return;
    }

    if (st.cur_submission.empty())
    {
        ImGui::Text("未选择提交记录");
        return;
    }

    const string status = st.cur_submission["status"];
    ImGui::Spacing();

    ImGui::BeginChild("submission_meta", ImVec2(0, 132 * UI_SCALE), true);
    if (ImGui::BeginTable("submission_meta_table", 4, ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("k1", ImGuiTableColumnFlags_WidthFixed, 80 * UI_SCALE);
        ImGui::TableSetupColumn("v1", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("k2", ImGuiTableColumnFlags_WidthFixed, 80 * UI_SCALE);
        ImGui::TableSetupColumn("v2", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextDisabled("提交 ID");
        ImGui::TableNextColumn();
        ImGui::Text("%s", st.cur_submission["id"].c_str());
        ImGui::TableNextColumn();
        ImGui::TextDisabled("状态");
        ImGui::TableNextColumn();
        ImGui::TextColored(status_color(status), "%s", status.c_str());

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextDisabled("题目");
        ImGui::TableNextColumn();
        ImGui::TextWrapped("#%s %s",
                           st.cur_submission["problem_id"].c_str(),
                           st.cur_submission["title"].c_str());
        ImGui::TableNextColumn();
        ImGui::TextDisabled("提交时间");
        ImGui::TableNextColumn();
        ImGui::TextWrapped("%s", st.cur_submission["created_at"].c_str());
        ImGui::EndTable();
    }
    ImGui::Spacing();
    ImGui::TextDisabled("错误原因 / 状态说明");
    ImGui::TextWrapped("%s", submission_reason(status).c_str());
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Text("提交代码");
    ImGui::BeginChild("submission_code", ImVec2(0, 0), true,
                      ImGuiWindowFlags_HorizontalScrollbar);
    const string &code = st.cur_submission["code"];
    ImGui::TextUnformatted(code.c_str());
    ImGui::EndChild();
}

// ============================================================
// 修改密码
// ============================================================

void UserView::draw_change_password(AppState &st)
{
    CenteredContent(400, 200);

    ImGui::Text("修改密码");
    ImGui::Separator();
    ImGui::Text("%s", st.status);

    labeled_input("旧密码", "##old_password", st.old_pwd, sizeof(st.old_pwd),
                  ImGuiInputTextFlags_Password, CenteredTargetW());
    labeled_input("新密码", "##new_password", st.new_pwd, sizeof(st.new_pwd),
                  ImGuiInputTextFlags_Password, CenteredTargetW());

    float btn_w = 120 * UI_SCALE;
    float total_w = btn_w * 2 + ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (CenteredTargetW() - total_w) * 0.5f);
    if (ImGui::Button("确认修改", SZ(120, 40)))
    {
        init_db();
        user_->change_my_password(st.old_pwd, st.new_pwd);
        st.set_status("密码修改完成");
    }
    ImGui::SameLine();
    if (ImGui::Button("返回", SZ(120, 40)))
    {
        st.state = GUIState::USER_MENU;
    }

    EndCenteredContent(400);
}

// ============================================================
// AI 助手
// ============================================================

void UserView::draw_ai_assistant(AppState &st)
{
    ImGui::Text("AI 助手");
    ImGui::Separator();

    if (ImGui::Button("返回", SZ(80, 25)))
    {
        if (st.ai_list_mode)
            st.state = GUIState::PROBLEM_LIST;
        else
            st.state = GUIState::PROBLEM_DETAIL;
        return;
    }

    if (!ai_ || !ai_->isAvailable())
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "AI 服务不可用");
        return;
    }

    // 每个用户使用独立的 session_id，避免记忆污染
    ai_->setSessionId("user_" + to_string(user_->get_current_user_id()));

    ImGui::BeginChild("chat", ImVec2(0, 200 * UI_SCALE), true);
    for (const auto &msg : st.chat_history)
    {
        if (msg.first == "user")
            ImGui::TextColored(ImVec4(0.3f, 0.7f, 1, 1), "你: %s", msg.second.c_str());
        else
            ImGui::TextColored(ImVec4(0, 1, 0.3f, 1), "AI: %s", msg.second.c_str());
    }
    ImGui::EndChild();

    ImGui::InputText("问题", st.question, sizeof(st.question));
    ImGui::SameLine();
    if (ImGui::Button("发送", SZ(80, 25)) && st.question[0] != '\0')
    {
        string q = st.question;
        st.chat_history.push_back({"user", q});

        // 判断用户意图：是否询问推荐题目/图论等（与当前代码无关）
        bool ask_recommend = false;
        string lower_q = q;
        for (char &c : lower_q)
            c = tolower(c);
        if (lower_q.find("推荐") != string::npos ||
            lower_q.find("题目") != string::npos ||
            lower_q.find("图论") != string::npos ||
            lower_q.find("有什么题") != string::npos ||
            lower_q.find("想做") != string::npos)
        {
            ask_recommend = true;
        }

        string code, info;
        if (!ask_recommend && !st.cur_problem.empty())
        {
            // 只有非推荐类问题才传递当前题目和代码上下文
            string workspace = "workspace/" + to_string(user_->get_current_user_id()) +
                               "/solution.cpp";
            code = read_file(workspace);

            info = "【题目信息】\n";
            info += "题号: " + st.cur_problem["id"] + "\n";
            info += "标题: " + st.cur_problem["title"] + "\n";
            info += "描述: " + st.cur_problem["description"] + "\n";
            info += "时间限制: " + st.cur_problem["time_limit"] + " ms\n";
            info += "内存限制: " + st.cur_problem["memory_limit"] + " MB";

            string err = user_->getLastErrorContext();
            if (!err.empty())
                info += "\n\n" + err;
        }

        string resp;
        if (st.ai_list_mode)
        {
            // 题库模式：传递整个题库，不传递代码
            string ctx = "【题库列表】\n";
            auto all = db_->query(
                "SELECT id, title, category, description FROM problems ORDER BY id");
            for (const auto &p : all)
            {
                ctx += "题号:" + p.at("id") + " | 标题:" + p.at("title") +
                       " | 类别:" + p.at("category") + " | 描述:" +
                       p.at("description").substr(0, 80) + "\n";
            }
            resp = ai_->ask(q, "", ctx);
        }
        else
        {
            resp = ai_->ask(q, code, info);
        }

        if (resp.find("[NEED_PROBLEMS]") != string::npos)
        {
            string ctx = info;
            if (!ctx.empty())
                ctx += "\n\n";
            ctx += "【题库列表】\n";
            auto all = db_->query(
                "SELECT id, title, category, description FROM problems ORDER BY id");
            for (const auto &p : all)
            {
                ctx += "题号:" + p.at("id") + " | 标题:" + p.at("title") +
                       " | 类别:" + p.at("category") + " | 描述:" +
                       p.at("description").substr(0, 80) + "\n";
            }
            resp = ai_->ask(q, "", ctx);
            // 如果 AI 仍然返回占位符，给默认回复
            if (resp.find("[NEED_PROBLEMS]") != string::npos)
            {
                resp = "抱歉，我暂时无法获取题库信息。你可以直接在题目列表中浏览可用题目。";
            }
        }

        st.chat_history.push_back({"ai", resp});
        st.question[0] = '\0';
    }
}
