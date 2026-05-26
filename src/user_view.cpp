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
    CenteredContent(400, 220);

    ImGui::Text("用户登录");
    ImGui::Separator();
    ImGui::Text("%s", st.status);

    ImGui::PushItemWidth(CenteredTargetW());
    ImGui::InputText("账号", st.account, sizeof(st.account));
    ImGui::InputText("密码", st.password, sizeof(st.password),
                     ImGuiInputTextFlags_Password);
    ImGui::PopItemWidth();

    float btn_w = 120 * UI_SCALE;
    float total_w = btn_w * 3 + ImGui::GetStyle().ItemSpacing.x * 2;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (CenteredTargetW() - total_w) * 0.5f);
    if (ImGui::Button("登录", SZ(120, 40)))
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
    if (ImGui::Button("注册", SZ(120, 40)))
    {
        st.state = GUIState::USER_REGISTER;
        st.set_status("注册新账号");
    }
    ImGui::SameLine();
    if (ImGui::Button("返回", SZ(120, 40)))
    {
        st.state = GUIState::MAIN_MENU;
        st.set_status("欢迎来到 OJ 在线判题系统");
    }

    EndCenteredContent(400);
}

void UserView::draw_register(AppState &st)
{
    CenteredContent(400, 180);

    ImGui::Text("注册新账号");
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

    EndCenteredContent(400);
}

// ============================================================
// 用户菜单
// ============================================================

void UserView::draw_menu(AppState &st)
{
    CenteredContent(400, 260);

    if (user_ && user_->is_logged_in())
    {
        ImGui::Text("用户模式 - %s", user_->get_current_account().c_str());
    }
    ImGui::Separator();
    ImGui::Text("%s", st.status);

    if (ImGui::Button("查看题目列表", CW(50)))
    {
        load_problems(st);
        st.state = GUIState::PROBLEM_LIST;
    }
    if (ImGui::Button("查看我的提交", CW(50)))
    {
        load_submissions(st);
        st.state = GUIState::SUBMISSIONS_LIST;
    }
    if (ImGui::Button("修改密码", CW(50)))
    {
        st.old_pwd[0] = st.new_pwd[0] = '\0';
        st.state = GUIState::CHANGE_PASSWORD;
    }
    if (ImGui::Button("退出登录", CW(50)))
    {
        user_.reset();
        db_.reset();
        ai_.reset();
        st.state = GUIState::MAIN_MENU;
        st.set_status("欢迎来到 OJ 在线判题系统");
    }

    EndCenteredContent(400);
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

    ImGui::PushItemWidth(200 * UI_SCALE);
    ImGui::InputText("##search", st.search_buf, sizeof(st.search_buf),
                     ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("搜索", SZ(60, 25)))
    {
        // 搜索按钮点击时刷新（由下方过滤逻辑处理）
    }
    ImGui::SameLine();
    if (ImGui::Button("AI 助手", SZ(80, 25)))
    {
        st.ai_list_mode = true;
        st.chat_history.clear();
        st.question[0] = '\0';
        st.state = GUIState::AI_ASSISTANT;
    }

    ImGui::BeginTable("problems", 5,
                      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY);
    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40 * UI_SCALE);
    ImGui::TableSetupColumn("标题", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("知识点", ImGuiTableColumnFlags_WidthFixed, 100 * UI_SCALE);
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
        ImGui::Text("%s", p.at("id").c_str());
        ImGui::TableNextColumn();

        // 已解决题目显示绿色方块，避免缺字体时显示成问号
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
            ImGui::Dummy(ImVec2(size + 6.0f * UI_SCALE, ImGui::GetTextLineHeight()));
            ImGui::SameLine();
        }

        if (ImGui::Selectable(p.at("title").c_str(), false,
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
        ImGui::Text("【题号】 %s", st.cur_problem["id"].c_str());
        ImGui::Text("【标题】 %s", st.cur_problem["title"].c_str());
        ImGui::Text("【知识点】 %s", st.cur_problem["category"].c_str());
        ImGui::Text("【时间限制】 %s ms", st.cur_problem["time_limit"].c_str());
        ImGui::Text("【内存限制】 %s MB", st.cur_problem["memory_limit"].c_str());
        ImGui::Separator();
        ImGui::TextWrapped("%s", st.cur_problem["description"].c_str());
        ImGui::Separator();

        string workspace = "workspace/" + to_string(user_->get_current_user_id()) +
                           "/solution.cpp";
        if (st.code_buf[0] == '\0')
        {
            string code = read_file(workspace);
            snprintf(st.code_buf, sizeof(st.code_buf), "%s", code.c_str());
        }

        ImGui::Text("代码编辑（C++）：");
        ImGui::PushItemWidth(-1);
        ImGui::InputTextMultiline("##code", st.code_buf, sizeof(st.code_buf),
                                  ImVec2(0, 200 * UI_SCALE),
                                  ImGuiInputTextFlags_AllowTabInput);
        ImGui::PopItemWidth();

        float btn_w = 120 * UI_SCALE;
        float total_w = btn_w * 3 + ImGui::GetStyle().ItemSpacing.x * 2;
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - total_w) * 0.5f);

        if (ImGui::Button("保存代码", SZ(120, 35)))
        {
            if (write_file(workspace, st.code_buf))
                st.set_status("代码已保存到 " + workspace);
            else
                st.set_status("保存失败：" + workspace);
        }
        ImGui::SameLine();
        if (ImGui::Button("提交代码", SZ(120, 35)))
        {
            int pid = stoi(st.cur_problem["id"]);
            string workspace = "workspace/" + to_string(user_->get_current_user_id()) +
                               "/solution.cpp";
            string code = read_file(workspace);
            if (code.empty())
            {
                st.set_status(workspace + " 为空，请先编写代码");
            }
            else
            {
                user_->submit_code(pid, code, "C++");
                st.last_report = user_->getLastReport();
                st.state = GUIState::SUBMIT_RESULT;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("AI 助手", SZ(120, 30)))
        {
            st.ai_list_mode = false;
            st.chat_history.clear();
            st.question[0] = '\0';
            st.state = GUIState::AI_ASSISTANT;
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
    string rt;
    switch (r.result)
    {
    case JudgeResult::ACCEPTED:
        rt = "Accepted (AC)";
        break;
    case JudgeResult::WRONG_ANSWER:
        rt = "Wrong Answer (WA)";
        break;
    case JudgeResult::TIME_LIMIT_EXCEEDED:
        rt = "Time Limit Exceeded (TLE)";
        break;
    case JudgeResult::MEMORY_LIMIT_EXCEEDED:
        rt = "Memory Limit Exceeded (MLE)";
        break;
    case JudgeResult::RUNTIME_ERROR:
        rt = "Runtime Error (RE)";
        break;
    case JudgeResult::COMPILE_ERROR:
        rt = "Compile Error (CE)\n" + r.error_message;
        break;
    case JudgeResult::SYSTEM_ERROR:
        rt = "System Error: " + r.error_message;
        break;
    default:
        rt = "Unknown";
        break;
    }

    ImGui::Text("状态: %s", rt.c_str());

    if (r.result != JudgeResult::COMPILE_ERROR &&
        r.result != JudgeResult::SYSTEM_ERROR)
    {
        ImGui::Text("通过: %d / %d", r.passed_test_cases, r.total_test_cases);
        ImGui::Text("时间: %d ms | 内存: %d MB", r.time_used_ms, r.memory_used_mb);

        ImGui::BeginTable("details", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 40 * UI_SCALE);
        ImGui::TableSetupColumn("结果", ImGuiTableColumnFlags_WidthFixed, 80 * UI_SCALE);
        ImGui::TableSetupColumn("时间", ImGuiTableColumnFlags_WidthFixed, 80 * UI_SCALE);
        ImGui::TableSetupColumn("内存", ImGuiTableColumnFlags_WidthFixed, 80 * UI_SCALE);
        ImGui::TableHeadersRow();
        for (const auto &d : r.details)
        {
            string tag;
            switch (d.result)
            {
            case JudgeResult::ACCEPTED:
                tag = "AC";
                break;
            case JudgeResult::WRONG_ANSWER:
                tag = "WA";
                break;
            case JudgeResult::TIME_LIMIT_EXCEEDED:
                tag = "TLE";
                break;
            case JudgeResult::MEMORY_LIMIT_EXCEEDED:
                tag = "MLE";
                break;
            case JudgeResult::RUNTIME_ERROR:
                tag = "RE";
                break;
            default:
                tag = "??";
                break;
            }
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%d", d.case_id);
            ImGui::TableNextColumn();
            ImGui::Text("%s", tag.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%dms", d.time_ms);
            ImGui::TableNextColumn();
            ImGui::Text("%dMB", d.memory_mb);
        }
        ImGui::EndTable();
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

    ImGui::BeginTable("submissions", 5,
                      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY);
    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40);
    ImGui::TableSetupColumn("题目ID", ImGuiTableColumnFlags_WidthFixed, 60);
    ImGui::TableSetupColumn("题目", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("状态", ImGuiTableColumnFlags_WidthFixed, 60);
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
        if (status_str == "AC")
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "AC");
        else if (status_str == "WA")
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "WA");
        else if (status_str == "TLE" || status_str == "MLE")
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "%s", status_str.c_str());
        else if (status_str == "RE")
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "RE");
        else if (status_str == "CE")
            ImGui::TextColored(ImVec4(1, 0, 1, 1), "CE");
        else
            ImGui::Text("%s", status_str.c_str());
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
    ImGui::Text("提交 ID: %s", st.cur_submission["id"].c_str());
    ImGui::Text("题目: #%s %s",
                st.cur_submission["problem_id"].c_str(),
                st.cur_submission["title"].c_str());
    ImGui::Text("提交时间: %s", st.cur_submission["created_at"].c_str());
    ImGui::Text("状态: ");
    ImGui::SameLine();
    if (status == "AC")
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", status.c_str());
    else if (status == "TLE" || status == "MLE")
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "%s", status.c_str());
    else
        ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "%s", status.c_str());

    ImGui::Separator();
    ImGui::Text("错误原因 / 状态说明");
    ImGui::TextWrapped("%s", submission_reason(status).c_str());

    ImGui::Separator();
    ImGui::Text("提交代码");
    ImGui::BeginChild("submission_code", ImVec2(0, 360 * UI_SCALE), true,
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

    ImGui::PushItemWidth(CenteredTargetW());
    ImGui::InputText("旧密码", st.old_pwd, sizeof(st.old_pwd),
                     ImGuiInputTextFlags_Password);
    ImGui::InputText("新密码", st.new_pwd, sizeof(st.new_pwd),
                     ImGuiInputTextFlags_Password);
    ImGui::PopItemWidth();

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
