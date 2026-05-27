#include "../include/view_manager.h"
#include "../include/user_view.h"
#include "../include/admin_view.h"
#include <imgui.h>

using namespace std;

float UI_SCALE = 1.0f;

static void apply_app_style()
{
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(14, 12);
    style.FramePadding = ImVec2(10, 7);
    style.CellPadding = ImVec2(9, 7);
    style.ItemSpacing = ImVec2(10, 9);
    style.ItemInnerSpacing = ImVec2(8, 6);
    style.ScrollbarSize = 16.0f;
    style.WindowRounding = 0.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 5.0f;
    style.PopupRounding = 5.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 5.0f;
    style.TabRounding = 5.0f;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

    ImVec4 *c = style.Colors;
    c[ImGuiCol_Text] = ImVec4(0.91f, 0.92f, 0.90f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.58f, 0.60f, 0.58f, 1.00f);
    c[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.09f, 0.09f, 1.00f);
    c[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.12f, 0.12f, 1.00f);
    c[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.12f, 0.12f, 0.98f);
    c[ImGuiCol_Border] = ImVec4(0.25f, 0.28f, 0.27f, 1.00f);
    c[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.17f, 0.17f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.25f, 0.24f, 1.00f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.32f, 0.29f, 1.00f);
    c[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.09f, 0.09f, 1.00f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.12f, 0.12f, 1.00f);
    c[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.12f, 0.12f, 1.00f);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.09f, 0.09f, 1.00f);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.33f, 0.32f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.45f, 0.43f, 1.00f);
    c[ImGuiCol_Button] = ImVec4(0.13f, 0.36f, 0.32f, 1.00f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.17f, 0.49f, 0.43f, 1.00f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.10f, 0.28f, 0.25f, 1.00f);
    c[ImGuiCol_Header] = ImVec4(0.16f, 0.34f, 0.31f, 1.00f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.47f, 0.41f, 1.00f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.12f, 0.29f, 0.26f, 1.00f);
    c[ImGuiCol_Separator] = ImVec4(0.25f, 0.28f, 0.27f, 1.00f);
    c[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.42f, 0.38f, 0.65f);
    c[ImGuiCol_ResizeGripHovered] = ImVec4(0.35f, 0.58f, 0.51f, 0.85f);
    c[ImGuiCol_TableHeaderBg] = ImVec4(0.13f, 0.15f, 0.15f, 1.00f);
    c[ImGuiCol_TableBorderStrong] = ImVec4(0.30f, 0.34f, 0.33f, 1.00f);
    c[ImGuiCol_TableBorderLight] = ImVec4(0.21f, 0.24f, 0.23f, 1.00f);
    c[ImGuiCol_TableRowBg] = ImVec4(0.10f, 0.11f, 0.11f, 1.00f);
    c[ImGuiCol_TableRowBgAlt] = ImVec4(0.13f, 0.14f, 0.14f, 1.00f);
    c[ImGuiCol_TextSelectedBg] = ImVec4(0.18f, 0.45f, 0.39f, 0.55f);
}

static void centered_text(const char *text)
{
    float text_w = ImGui::CalcTextSize(text).x;
    float avail_w = ImGui::GetContentRegionAvail().x;
    if (avail_w > text_w)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail_w - text_w) * 0.5f);
    ImGui::TextUnformatted(text);
}

// ============================================================
// ViewManager::Impl
// ============================================================

class ViewManager::Impl
{
public:
    AppState st;
    unique_ptr<UserView> user_view;
    unique_ptr<AdminView> admin_view;

    Impl()
    {
        apply_app_style();
        user_view = make_unique<UserView>();
        admin_view = make_unique<AdminView>();
    }

    void draw_main_menu();
    void render();
};

// ============================================================
// 主菜单
// ============================================================

void ViewManager::Impl::draw_main_menu()
{
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float panel_w = avail.x * 0.58f;
    if (panel_w < 760.0f)
        panel_w = 760.0f;
    if (panel_w > 1120.0f)
        panel_w = 1120.0f;
    if (panel_w > avail.x - 32.0f)
        panel_w = avail.x - 32.0f;

    float panel_h = 420.0f;
    if (panel_h > avail.y - 32.0f)
        panel_h = avail.y - 32.0f;

    float offset_x = (avail.x - panel_w) * 0.5f;
    float offset_y = (avail.y - panel_h) * 0.42f;
    if (offset_x > 0)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset_x);
    if (offset_y > 0)
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset_y);

    ImGui::BeginChild("main_menu_panel", ImVec2(panel_w, panel_h), false);

    centered_text("OJ 在线判题系统");
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    centered_text("桌面版在线评测与编程训练平台");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();
    centered_text(st.status);
    ImGui::Spacing();

    if (ImGui::BeginTable("role_entry", 2, ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("user", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("admin", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::BeginChild("user_entry", ImVec2(0, 150.0f), true);
        ImGui::Text("普通用户");
        ImGui::Dummy(ImVec2(0, 18.0f));
        if (ImGui::Button("进入用户端", ImVec2(-1, 52.0f)))
        {
            st.state = GUIState::USER_LOGIN;
            st.account[0] = st.password[0] = '\0';
            st.set_status("用户登录 / 注册");
        }
        ImGui::EndChild();

        ImGui::TableNextColumn();
        ImGui::BeginChild("admin_entry", ImVec2(0, 150.0f), true);
        ImGui::Text("管理员");
        ImGui::Dummy(ImVec2(0, 18.0f));
        if (ImGui::Button("进入管理端", ImVec2(-1, 52.0f)))
        {
            st.state = GUIState::ADMIN_LOGIN;
            st.account[0] = st.password[0] = '\0';
            st.set_status("管理员登录");
        }
        ImGui::EndChild();

        ImGui::EndTable();
    }
    ImGui::Spacing();
    float exit_w = 260.0f;
    ImGui::SetCursorPosX((panel_w - exit_w) * 0.5f);
    if (ImGui::Button("退出系统", ImVec2(exit_w, 48.0f)))
    {
        st.set_status("退出中...");
    }

    ImGui::EndChild();
}

// ============================================================
// 渲染主循环
// ============================================================

void ViewManager::Impl::render()
{
    ImVec2 display_size = ImGui::GetIO().DisplaySize;
    UI_SCALE = display_size.y / 600.0f; // 以 600px 高度为基准
    if (UI_SCALE < 0.8f)
        UI_SCALE = 0.8f;
    if (UI_SCALE > 3.0f)
        UI_SCALE = 3.0f;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(display_size);

    ImGui::Begin("OJ 在线判题系统", nullptr,
                 ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove);

    switch (st.state)
    {
    case GUIState::MAIN_MENU:
        draw_main_menu();
        break;
    case GUIState::USER_LOGIN:
        user_view->draw_login(st);
        break;
    case GUIState::USER_REGISTER:
        user_view->draw_register(st);
        break;
    case GUIState::USER_MENU:
        user_view->draw_menu(st);
        break;
    case GUIState::PROBLEM_LIST:
        user_view->draw_problem_list(st);
        break;
    case GUIState::PROBLEM_DETAIL:
        user_view->draw_problem_detail(st);
        break;
    case GUIState::SUBMIT_RESULT:
        user_view->draw_submit_result(st);
        break;
    case GUIState::SUBMISSIONS_LIST:
        user_view->draw_submissions_list(st);
        break;
    case GUIState::SUBMISSION_DETAIL:
        user_view->draw_submission_detail(st);
        break;
    case GUIState::CHANGE_PASSWORD:
        user_view->draw_change_password(st);
        break;
    case GUIState::AI_ASSISTANT:
        user_view->draw_ai_assistant(st);
        break;
    case GUIState::ADMIN_LOGIN:
        admin_view->draw_login(st);
        break;
    case GUIState::ADMIN_MENU:
        admin_view->draw_menu(st);
        break;
    case GUIState::ADMIN_PROBLEM_LIST:
        admin_view->draw_problem_list(st);
        break;
    case GUIState::ADMIN_PROBLEM_DETAIL:
        admin_view->draw_problem_detail(st);
        break;
    case GUIState::ADMIN_ADD_PROBLEM:
        admin_view->draw_add_problem(st);
        break;
    }

    ImGui::End();
}

// ============================================================
// ViewManager 公共接口
// ============================================================

ViewManager::ViewManager() : impl_(new Impl()) {}

ViewManager::~ViewManager()
{
    delete impl_;
}

void ViewManager::render()
{
    impl_->render();
}
