#include "view_manager.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <cstdio>

int main()
{
    // ---- 初始化 GLFW ----
    if (!glfwInit())
    {
        fprintf(stderr, "GLFW 初始化失败\n");
        return -1;
    }

    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // ---- 获取屏幕分辨率并创建窗口（占屏幕 85%）----
    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int win_w = static_cast<int>(mode->width * 0.85);
    int win_h = static_cast<int>(mode->height * 0.85);

    GLFWwindow *window = glfwCreateWindow(win_w, win_h, "OJ 在线判题系统", nullptr, nullptr);
    if (!window)
    {
        fprintf(stderr, "创建窗口失败\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // ---- 初始化 ImGui ----
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    // 加载中文字体（基础字号大一点，配合较小的 FontGlobalScale，提高清晰度）
    float font_size = 20.0f;
    ImFontConfig font_cfg;
    font_cfg.OversampleH = 2;
    font_cfg.OversampleV = 2;
    font_cfg.PixelSnapH = false;
    ImFont *font = io.Fonts->AddFontFromFileTTF(
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        font_size, &font_cfg, io.Fonts->GetGlyphRangesChineseFull());
    if (!font)
    {
        font = io.Fonts->AddFontFromFileTTF(
            "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
            font_size, &font_cfg, io.Fonts->GetGlyphRangesChineseFull());
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // ---- 创建视图 ----
    ViewManager view;

    // ---- 主循环 ----
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // 动态缩放
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        ImGuiIO &io = ImGui::GetIO();
        io.FontGlobalScale = (display_h / 750.0f);
        if (io.FontGlobalScale < 0.8f)
            io.FontGlobalScale = 0.8f;
        if (io.FontGlobalScale > 3.0f)
            io.FontGlobalScale = 3.0f;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        view.render();

        ImGui::Render();
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.08f, 0.09f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // ---- 清理 ----
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
