#include "EditorLayer.h"

#include "NativeScripts/CameraScripts.h"

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

        FramebufferSpecification fbspec;
        fbspec.Width = 1600;
        fbspec.Height = 900;
        m_Framebuffer = Framebuffer::Create(fbspec);

        // Camera controller
        //Application& app = Application::Get();
        //m_CameraController = CreateRef<PerspectivePlanarCameraController>(
        //    Vector3(0.f, 0.f, 2.f), Vector3(0.f, 0.f, -1.f),
        //    (float)app.GetWindow().GetWidth() / (float)app.GetWindow().GetHeight(),
        //    0.1f, 100.f, glm::radians(60.f)
        //    );
        //m_CameraController->SetControlled(true);


#ifdef LV_EDITOR_USE_ORBITAL
        m_Scene = CreateRef<OrbitalScene>();
        auto camera = m_Scene->CreateEntity("Camera");
        {
            camera.AddComponent<CameraComponent>();
            camera.AddComponent<NativeScriptComponent>().Bind<OrbitalCameraScript>();
        }

        m_Scene->SetRootScaling(10.0);
        auto root = m_Scene->GetRoot();
        {
            root.AddComponent<SpriteRendererComponent>(Vector4{ 1.f, 1.f, 0.9f, 1.f });
            auto& orbital = root.GetComponent<OrbitalComponent>();
            orbital.LocalScale = { 0.05f, 0.05f, 0.f };
            orbital.SetMass(1.0 / 6.6743e-11);
        }

        auto orbital0 = m_Scene->CreateEntity("Orbital 0");
        {
            orbital0.AddComponent<SpriteRendererComponent>(Vector4{ 1.f, 0.3f, 0.2f, 1.f });
            auto& transform = orbital0.GetComponent<TransformComponent>();
            transform.SetPosition({ 0.9f, 0.f, 0.f });
            transform.SetScale({ 0.1f, 0.1f, 0.f });
            orbital0.AddComponent<OrbitalComponent>().SetMass(1.0);
        }

        auto orbital1 = m_Scene->CreateEntity("Orbital 1");
        {
            orbital1.AddComponent<SpriteRendererComponent>(Vector4{ 0.3f, 0.2f, 1.f, 1.f });
            auto& transform = orbital1.GetComponent<TransformComponent>();
            transform.SetPosition({ 0.f, 0.5f, 0.f });
            transform.SetScale({ 0.1f, 0.1f, 0.f });
            orbital1.AddComponent<OrbitalComponent>().SetMass(1.0);
        }
#else
        m_Scene = CreateRef<Scene>();

        Entity camera0 = m_Scene->CreateEntity("Camera 0");
        {
            camera0.AddComponent<CameraComponent>();
            auto& transform = camera0.GetComponent<TransformComponent>();
            transform.Set({ 1.f }, { 0.f, 0.f, 2.f });
        }

        Entity camera1 = m_Scene->CreateEntity("Camera 1");
        {
            camera1.AddComponent<CameraComponent>();
            auto& transform = camera1.GetComponent<TransformComponent>();
            transform.Set({ 1.f }, { 0.f, 0.f, 3.f });
        }

        m_Scene->SetActiveCamera(camera0);

        {
            auto& script = camera0.AddComponent<NativeScriptComponent>();
            script.Bind<PlanarCameraScript>();
        }
        {
            auto& script = camera1.AddComponent<NativeScriptComponent>();
            script.Bind<PlanarCameraScript>();
        }

        // Renderables
        Entity square = m_Scene->CreateEntity("Default Square");
        {
            square.AddComponent<SpriteRendererComponent>(Vector4{ 0.2f, 1.f, 0.3f, 1.f });
        }

        Entity subSquare = m_Scene->CreateEntity("Sub-Square");
        {
            subSquare.AddComponent<SpriteRendererComponent>(Vector4{ 1.f, 0.8f, 0.3f, 1.f });
            auto& transform = subSquare.GetComponent<TransformComponent>();
            transform.Set({ 0.2f }, { 0.5f, 0.5f, 0.2f });
            m_Scene->SetParent(subSquare, square);
        }

        Entity circle = m_Scene->CreateEntity("Circle");
        {
            auto& crc = circle.AddComponent<CircleRendererComponent>();
            auto& transform = circle.GetComponent<TransformComponent>();
            transform.Set({ 0.4f }, {-0.5f,-0.5f, 0.2f });
        }

        Entity ellipse = m_Scene->CreateEntity("Ellipse");
        {
            auto& erc = ellipse.AddComponent<EllipseRendererComponent>();
            erc.Fade = 0.f;
            auto& transform = ellipse.GetComponent<TransformComponent>();
            //transform.Set({ 0.0f, 0.3f, 0.f }, {-0.5f, 0.5f, 0.2f });
        }
#endif

        m_SceneHierarchyPanel.SetContext(m_Scene.get());
    }


    void EditorLayer::OnDetach()
    {
        LV_PROFILE_FUNCTION();
    }


    void EditorLayer::OnUpdate(Timestep dT)
    {
        LV_PROFILE_FUNCTION();

        // Update
        {
            LV_PROFILE_SCOPE("m_CameraController->OnUpdate - EditorLayer::OnUpdate");

#ifdef LV_EDITOR_USE_ORBITAL
            /* Do orbital stuff */
#else
            /* Do non-orbital stuff */
#endif
            //m_CameraController->OnUpdate(dT);
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

            m_Scene->OnUpdate(dT);
            m_Scene->OnRender();

            m_Framebuffer->Unbind();
        }
    }


    void EditorLayer::OnImGuiRender()
    {
        // From imgui_demo.cpp /////////////////

        static bool dockspaceOpen = true;
        static bool opt_fullscreen = true;
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
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // Submit the DockSpace
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float winMinSize = style.WindowMinSize.x;
        style.WindowMinSize.x = 370.f;
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
        style.WindowMinSize.x = winMinSize;


        /*** Menu bar ***/

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


        /*** Scene ***/

        ImGui::Begin("Scene Properties");

        Entity activeCamera = m_Scene->GetActiveCamera();
        if (ImGui::BeginCombo("Camera", activeCamera.GetComponent<TagComponent>().Tag.c_str()))
        {
            auto cameraEntities = m_Scene->GetEntitiesByComponents<CameraComponent>();
            for (auto& entity : cameraEntities)
            {
                if (ImGui::Selectable(entity.GetComponent<TagComponent>().Tag.c_str(), activeCamera == entity))
                {
                    activeCamera = entity;
                    m_Scene->SetActiveCamera(entity);
                }
            }
            ImGui::EndCombo();
        }

#ifdef LV_EDITOR_USE_ORBITAL
        {
            double rootScaling = m_Scene->GetRootScaling();
            if (LimnGui::InputScientific("RootScaling", rootScaling)) {
                m_Scene->SetRootScaling(rootScaling);
            }
        }

        if (ImGui::TreeNodeEx("Influence Visuals", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::ColorEdit4("Color", m_Scene->m_InfluenceColor.Ptr(), ImGuiColorEditFlags_AlphaBar);
            ImGui::DragFloat("Thickness", &m_Scene->m_InfluenceThickness, 0.001f, 0.001f, 1.f, "%.3f");
            ImGui::DragFloat("Fade", &m_Scene->m_InfluenceFade, 0.001f, 0.001f, 1.f, "%.3f");

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Orbit Visuals", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat("Thickness", &m_Scene->m_OrbitThickness, 0.001f, 0.001f, 1.f, "%.3f");
            ImGui::DragFloat("Thickness Factor", &m_Scene->m_OrbitThicknessFactor, 0.001f, 0.001f, 1.f, "%.3f");

            ImGui::TreePop();
        }
#endif

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
        //m_CameraController->SetControlled(m_ViewportFocused && m_ViewportHovered);

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        Vector2 viewportSize{ ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y };
        if (viewportSize != m_ViewportSize && viewportSize.x > 0 && viewportSize.y > 0)
        {
            m_ViewportSize = viewportSize;
            float aspect = viewportSize.x / viewportSize.y;

            m_Framebuffer->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
            //m_CameraController->SetAspect(aspect);
            m_Scene->OnWindowChangeAspect(aspect);
        }
        uint32_t viewportRendererId = m_Framebuffer->GetColorAttachmentRendererId();
        ImGui::Image((void*)viewportRendererId, viewportPanelSize, { 0, 1 }, { 1, 0 });
        ImGui::End(); // Viewport
        ImGui::PopStyleVar();


        m_SceneHierarchyPanel.OnImGuiRender();


        ImGui::End(); // DockSpace

        ImGui::ShowDemoWindow();
    }


    void EditorLayer::OnEvent(Event& e)
    {
        /* The editor needs to capture WindowResize events before they reach the camera controller, because
         * the camera's aspect ratio should be determined by the ImGui panel which displays the viewport
         * and not by the application window (which displays the entire editor).
         */
        //if (e.GetEventType() != EventType::WindowResize)
        //{
        //    m_CameraController->OnEvent(e);
        //}

        m_Scene->OnEvent(e);
    }

}
