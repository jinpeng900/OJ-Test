#include "../include/view_manager.h"
#include "../include/user_view.h"
#include "../include/admin_view.h"
#include <imgui.h>

using namespace std;

float UI_SCALE = 1.0f;

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
    CenteredContent(400, 260);

    ImGui::Text("OJ 在线判题系统");
    ImGui::Separator();
    ImGui::Text("%s", st.status);
    ImGui::Spacing();

    if (ImGui::Button("管理员进入", CW(50)))
    {
        st.state = GUIState::ADMIN_LOGIN;
        st.account[0] = st.password[0] = '\0';
        st.set_status("管理员登录");
    }
    if (ImGui::Button("用户进入", CW(50)))
    {
        st.state = GUIState::USER_LOGIN;
        st.account[0] = st.password[0] = '\0';
        st.set_status("用户登录 / 注册");
    }
    if (ImGui::Button("退出系统", CW(50)))
    {
        st.set_status("退出中...");
    }

    EndCenteredContent(400);
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
