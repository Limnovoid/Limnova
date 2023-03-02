#include "EditorLayer.h"

#define ASSET_DIR "C:\\Programming\\source\\Limnova\\LimnovaEditor\\Assets"


namespace Limnova
{

    EditorLayer::EditorLayer()
        : Layer("EditorLayer")
    {
    }


    void EditorLayer::OnAttach()
    {
        LV_PROFILE_FUNCTION();

        /* No event filtering in ImGuiLayer : we filter input in the EditorLayer itself using
         * the camera controller
         */
        Application::Get().GetImGuiLayer()->SetBlockEvents(false);

        // Camera
        Application& app = Application::Get();
        m_CameraController = CreateRef<PerspectivePlanarCameraController>(
            Vector3(0.f, 0.f, 2.f), Vector3(0.f, 0.f, -1.f),
            (float)app.GetWindow().GetWidth() / (float)app.GetWindow().GetHeight(),
            0.1f, 100.f, glm::radians(60.f)
            );
        m_CameraController->SetControlled(true);

        // Textures
        m_CheckerboardTexture = Texture2D::Create(ASSET_DIR"\\textures\\testtex.png", Texture::WrapMode::MirroredTile);
        m_SpriteSheet = Texture2D::Create(ASSET_DIR"\\textures\\kenney-sheet\\Spritesheet\\RPGpack_sheet_2X.png", Texture::WrapMode::Clamp);
        m_SpriteStairs = SubTexture2D::CreateFromCoords(m_SpriteSheet, { 7, 6 }, { 128, 128 });
        m_SpriteTree = SubTexture2D::CreateFromCoords(m_SpriteSheet, { 2, 1 }, { 128, 128 }, { 1, 2 });

        FramebufferSpecification fbspec;
        fbspec.Width = 1280;
        fbspec.Height = 720;
        m_Framebuffer = Framebuffer::Create(fbspec);
    }


    void EditorLayer::OnDetach()
    {
        LV_PROFILE_FUNCTION();
    }


    void EditorLayer::OnUpdate(Timestep dT)
    {
        LV_PROFILE_FUNCTION();

        static float s_AnimatedRotation;
        constexpr float rotationSpeed = 30.f;

        // Update
        {
            LV_PROFILE_SCOPE("m_CameraController->OnUpdate - EditorLayer::OnUpdate");

            m_CameraController->OnUpdate(dT);

            s_AnimatedRotation = Wrapf(s_AnimatedRotation + dT * rotationSpeed, 0.f, 360.f);
        }

        // Render
        Renderer2D::ResetStatistics();
        {
            LV_PROFILE_SCOPE("Render Prep - EditorLayer::OnUpdate");

            m_Framebuffer->Bind();
            RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.f });
            RenderCommand::Clear();
        }

        {
            LV_PROFILE_SCOPE("Render Draw - EditorLayer::OnUpdate");

            // Scene 1 - test quads
            Renderer2D::BeginScene(m_CameraController->GetCamera());
            Renderer2D::DrawRotatedQuad({ 0.f, 0.f }, { 3.f, 3.f }, glm::radians(m_BackgroundRotation), m_CheckerboardTexture, m_TextureTint, m_TextureScale);
            Renderer2D::DrawRotatedQuad({ 0.f, 0.5f, 1.f }, { 0.5f, 0.5f }, glm::radians(s_AnimatedRotation), m_TextureTint);
            Renderer2D::DrawQuad({ 0.75f, 0.f, 0.5f }, { 1.5f, 1.f }, m_SquareColor);
            Renderer2D::EndScene();

            // Scene 2 - procedural grid
            Renderer2D::BeginScene(m_CameraController->GetCamera());
            for (float y = -1.5f + 0.125f; y < 1.5f; y += 0.125f)
            {
                for (float x = -1.5f + 0.125f; x < 1.5f; x += 0.125f)
                {
                    Vector4 color{ (x + 1.5f) / 3.f, 0.5f, (y + 1.5f) / 3.f, 0.7f };
                    Renderer2D::DrawQuad({ x, y, 0.f }, { 0.11f, 0.11f }, color);
                }
            }
            Renderer2D::EndScene();

            // Scene 3 - sprites
            Renderer2D::BeginScene(m_CameraController->GetCamera());
            Renderer2D::DrawQuad({ 0.f, -1.f, 0.5f }, { 0.25f, 0.25f }, m_SpriteStairs);
            Renderer2D::DrawQuad({ -0.25f, -1.f, 0.5f }, { 0.25f, 0.5f }, m_SpriteTree);
            Renderer2D::EndScene();

            m_Framebuffer->Unbind();
        }
    }


    void EditorLayer::OnImGuiRender()
    {
        // From imgui_demo.cpp /////////////////

        static bool dockspaceOpen = true;
        static bool opt_fullscreen = true;
        static bool opt_padding = false;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }
        else
        {
            dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
        }

        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
        // and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
        // all active windows docked into it will lose their parent and become undocked.
        // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
        // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
        if (!opt_padding)
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
        if (!opt_padding)
            ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // Submit the DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Exit", NULL, false))
                {
                    Application::Get().Close();
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }


        ImGui::Begin("Scene Properties");
        ImGui::ColorEdit4("Square Color", glm::value_ptr(*(glm::vec4*)&m_SquareColor));
        ImGui::ColorEdit4("Texture Tint", glm::value_ptr(*(glm::vec4*)&m_TextureTint));
        ImGui::SliderFloat2("Texture Scale", glm::value_ptr(*(glm::vec2*)&m_TextureScale), 0.1f, 10.f);
        ImGui::SliderFloat("BackgroundRotation", &m_BackgroundRotation, 0.f, 360.f);
        ImGui::End(); // Scene Properties

        ImGui::Begin("Renderer2D Statistics");
        auto& stats = Renderer2D::GetStatistics();
        ImGui::Text("Draw Calls:    %d", stats.DrawCalls);
        ImGui::Text("Quads:         %d", stats.QuadCount);
        ImGui::Text("Vertices:      %d", stats.GetNumVertices());
        ImGui::Text("Indices:       %d", stats.GetNumIndices());
        ImGui::End(); // Renderer2D Statistics

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.f, 0.f });
        ImGui::Begin("Viewport");

        // Only control the camera if the viewport is focused and hovered
        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();
        //Application::Get().GetImGuiLayer()->SetBlockEvents(!m_ViewportFocused || !m_ViewportHovered);
        m_CameraController->SetControlled(m_ViewportFocused && m_ViewportHovered);

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        Vector2 viewportSize{ viewportPanelSize.x, viewportPanelSize.y };
        if (viewportSize != m_ViewportSize && viewportSize.x > 0 && viewportSize.y > 0)
        {
            m_ViewportSize = viewportSize;

            m_Framebuffer->Resize(viewportPanelSize.x, viewportPanelSize.y);
            m_CameraController->SetAspect(m_ViewportSize.x / m_ViewportSize.y);
        }
        uint32_t viewportRendererId = m_Framebuffer->GetColorAttachmentRendererId();
        ImGui::Image((void*)viewportRendererId, viewportPanelSize, { 0, 1 }, { 1, 0 });
        ImGui::End(); // Viewport
        ImGui::PopStyleVar();

        ImGui::End(); // DockSpace
    }


    void EditorLayer::OnEvent(Event& e)
    {
        /* The editor needs to capture WindowResize events before they reach the camera controller, as
         * the camera's aspect ratio should be determined by the ImGui panel which displays the viewport
         * and not by the application window (which displays the entire editor).
         */
        if (e.GetEventType() != EventType::WindowResize)
        {
            m_CameraController->OnEvent(e);
        }
    }

}
