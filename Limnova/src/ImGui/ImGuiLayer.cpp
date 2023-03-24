#include "ImGuiLayer.h"

#include "imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "Core/Application.h"

// TEMPORARY for ImGui
#include <GLFW/glfw3.h>
#include <glad/glad.h>


namespace Limnova
{

    ImGuiLayer::ImGuiLayer()
        : Layer("ImGuiLayer")
    {
    }

    ImGuiLayer::~ImGuiLayer()
    {
    }


    void ImGuiLayer::OnAttach()
    {
        LV_PROFILE_FUNCTION();

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        //io.ConfigViewportsNoAutoMerge = true;
        //io.ConfigViewportsNoTaskBarIcon = true;

        /*** NOTE : AddFontFromFileTTF() calls must correspond to FontIndex enums ***/
        io.FontDefault = io.Fonts->AddFontFromFileTTF(LV_ASSET_DIR"/fonts/NunitoSans/NunitoSans-Regular.ttf", 16.f);
        io.Fonts->AddFontFromFileTTF(LV_ASSET_DIR"/fonts/NunitoSans/NunitoSans-Bold.ttf", 16.f);

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.f;
            style.Colors[ImGuiCol_WindowBg].w = 1.f;
        }

        SetDarkTheme();

        GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());

        // Setup Platform/Renderer bindings
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 410");
    }


    void ImGuiLayer::OnDetach()
    {
        LV_PROFILE_FUNCTION();

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }


    void ImGuiLayer::Begin()
    {
        LV_PROFILE_FUNCTION();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }


    void ImGuiLayer::End()
    {
        LV_PROFILE_FUNCTION();

        ImGuiIO& io = ImGui::GetIO();
        Window& win = Application::Get().GetWindow();
        io.DisplaySize = ImVec2(win.GetWidth(), win.GetHeight());

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
    }


    void ImGuiLayer::OnImGuiRender()
    {
        LV_PROFILE_FUNCTION();
    }


    void ImGuiLayer::OnEvent(Event& e)
    {
        LV_PROFILE_FUNCTION();

        if (m_BlockEvents)
        {
            ImGuiIO& io = ImGui::GetIO();
            e.Handled |= e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
            e.Handled |= e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
        }
    }


    void ImGuiLayer::SetDarkTheme()
    {
        auto& colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_WindowBg] = ImVec4{ 0.08f, 0.08f, 0.08f, 1.f };

        // Headers
        colors[ImGuiCol_Header]         = ImVec4{ 0.2f, 0.2f, 0.2f, 1.f };
        colors[ImGuiCol_HeaderHovered]  = ImVec4{ 0.3f, 0.3f, 0.3f, 1.f };
        colors[ImGuiCol_HeaderActive]   = ImVec4{ 0.15f, 0.15f, 0.15f, 1.f };

        // Buttons
        colors[ImGuiCol_Button]         = ImVec4{ 0.2f, 0.2f, 0.2f, 1.f };
        colors[ImGuiCol_ButtonHovered]  = ImVec4{ 0.3f, 0.3f, 0.3f, 1.f };
        colors[ImGuiCol_ButtonActive]   = ImVec4{ 0.15f, 0.15f, 0.15f, 1.f };

        // Frame BG
        colors[ImGuiCol_FrameBg]        = ImVec4{ 0.15f, 0.2f, 0.25f, 1.f };
        colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.25f, 0.3f, 0.35f, 1.f };
        colors[ImGuiCol_FrameBgActive]  = ImVec4{ 0.1f, 0.15f, 0.2f, 1.f };

        // Tabs
        colors[ImGuiCol_Tab]                = ImVec4{ 0.15f, 0.15f, 0.15f, 1.f };
        colors[ImGuiCol_TabHovered]         = ImVec4{ 0.38f, 0.38f, 0.38f, 1.f };
        colors[ImGuiCol_TabActive]          = ImVec4{ 0.28f, 0.28f, 0.28f, 1.f };
        colors[ImGuiCol_TabUnfocused]       = ImVec4{ 0.15f, 0.15f, 0.15f, 1.f };
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.2f, 0.2f, 1.f };

        // Title
        colors[ImGuiCol_TitleBg]            = ImVec4{ 0.15f, 0.15f, 0.15f, 1.f };
        colors[ImGuiCol_TitleBgActive]      = ImVec4{ 0.15f, 0.15f, 0.15f, 1.f };
        colors[ImGuiCol_TitleBgCollapsed]   = ImVec4{ 0.95f, 0.15f, 0.95f, 1.f };
    }

}
