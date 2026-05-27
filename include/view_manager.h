#ifndef VIEW_MANAGER_H
#define VIEW_MANAGER_H

#include <imgui.h>
#include "judge_core.h"
#include <vector>
#include <map>
#include <set>
#include <string>

// 全局 UI 缩放因子（由 ViewManager 每帧更新）
extern float UI_SCALE;

// 辅助：按缩放因子生成 ImVec2
inline ImVec2 SZ(float w, float h) { return ImVec2(w * UI_SCALE, h * UI_SCALE); }

// ============================================================
// 居中布局辅助
// ============================================================

/**
 * @brief 开始水平垂直居中内容块
 * @param content_width  内容区域目标宽度（未缩放前的逻辑像素）
 * @param content_height 内容区域估计高度（未缩放前的逻辑像素），用于垂直居中
 */
inline float &CenteredTargetW()
{
    static float w = 0;
    return w;
}
inline float &CenteredIndentVal()
{
    static float v = 0;
    return v;
}

inline void CenteredContent(float content_width, float content_height)
{
    float avail_h = ImGui::GetContentRegionAvail().y;
    float offset_y = (avail_h - content_height * UI_SCALE) * 0.45f;
    if (offset_y > 0)
        ImGui::Dummy(ImVec2(0, offset_y));

    float avail_w = ImGui::GetContentRegionAvail().x;
    float target_w = content_width * UI_SCALE;
    float max_w = avail_w * 0.7f;
    if (max_w > 980.0f)
        max_w = 980.0f;
    if (target_w > max_w)
        target_w = max_w;
    if (target_w < 240 * UI_SCALE)
        target_w = 240 * UI_SCALE;
    CenteredTargetW() = target_w;

    float offset_x = (avail_w - target_w) * 0.5f;
    CenteredIndentVal() = offset_x > 0 ? offset_x : 0;
    if (offset_x > 0)
        ImGui::Indent(offset_x);
}

inline void EndCenteredContent(float content_width)
{
    ImGui::Unindent(CenteredIndentVal());
}

// 占满居中面板宽度的按钮尺寸
inline ImVec2 CW(float h)
{
    float w = CenteredTargetW();
    if (w <= 0)
        w = ImGui::GetContentRegionAvail().x;
    return ImVec2(w, h * UI_SCALE);
}

// ============================================================
// GUI 状态枚举
// ============================================================

enum class GUIState
{
    MAIN_MENU,
    USER_LOGIN,
    USER_REGISTER,
    USER_MENU,
    PROBLEM_LIST,
    PROBLEM_DETAIL,
    SUBMIT_RESULT,
    SUBMISSIONS_LIST,
    SUBMISSION_DETAIL,
    CHANGE_PASSWORD,
    AI_ASSISTANT,
    ADMIN_LOGIN,
    ADMIN_MENU,
    ADMIN_PROBLEM_LIST,
    ADMIN_PROBLEM_DETAIL,
    ADMIN_ADD_PROBLEM,
};

// ============================================================
// 共享应用状态
// ============================================================

struct AppState
{
    GUIState state = GUIState::MAIN_MENU;
    char status[256] = "欢迎来到 OJ 在线判题系统";

    char account[64] = "";
    char password[64] = "";
    char old_pwd[64] = "";
    char new_pwd[64] = "";
    char question[512] = "";
    char sql_buf[2048] = "";
    char code_buf[65536] = "";
    char problem_title[256] = "";
    char problem_category[64] = "";
    char problem_test_path[256] = "";
    char problem_description[4096] = "";
    int problem_time_limit = 1000;
    int problem_memory_limit = 128;
    int problem_id = 0;

    std::vector<std::map<std::string, std::string>> problems;
    std::vector<std::map<std::string, std::string>> submissions;
    std::map<std::string, std::string> cur_problem;
    std::map<std::string, std::string> cur_submission;
    JudgeReport last_report{};
    std::string ai_response;
    std::vector<std::pair<std::string, std::string>> chat_history;
    std::set<int> solved_problems;
    bool ai_list_mode = false;
    char search_buf[128] = "";

    void set_status(const std::string &s)
    {
        snprintf(status, sizeof(status), "%s", s.c_str());
    }
};

// ============================================================
// 主控制器
// ============================================================

class ViewManager
{
public:
    ViewManager();
    ~ViewManager();

    /**
     * @brief 每帧调用，渲染完整 GUI
     */
    void render();

private:
    class Impl;
    Impl *impl_;
};

#endif // VIEW_MANAGER_H
