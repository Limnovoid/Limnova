#include "EditorLayer.h"

#include "NativeScripts/CameraScripts.h"

#include <Scene/SceneSerializer.h>
#include <Utils/PlatformUtils.h>
#include <imguizmo/ImGuizmo.h>

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

        FramebufferSpecification fbSpec;
        fbSpec.Width = 1600;
        fbSpec.Height = 900;
        fbSpec.Attachments = {
            FramebufferTextureFormat::RGBA8,
            FramebufferTextureFormat::RINT,
            FramebufferTextureFormat::Depth
        };
        m_Framebuffer = Framebuffer::Create(fbSpec);

#ifdef LV_EDITOR_USE_ORBITAL

        m_EditorCamera.SetElevation(Radiansf(30.f));

        m_Scene = CreateRef<OrbitalScene>();
        auto camera = m_Scene->CreateEntity("Camera");
        {
            auto& cc = camera.AddComponent<CameraComponent>();
            cc.SetPerspectiveFov(Radiansf(80.f));
            camera.AddComponent<NativeScriptComponent>().Bind<OrbitalCameraScript>();
        }

        m_Scene->SetRootScaling(10.0);
        auto root = m_Scene->GetRoot();
        {
            auto& crc = root.AddComponent<BillboardCircleRendererComponent>();
            crc.Color = { 1.f, 1.f, 0.9f, 1.f };
            crc.Fade = 0.f;
            crc.Thickness = 1.f;
            auto& orbital = root.GetComponent<OrbitalComponent>();
            orbital.LocalScale = { 0.05f, 0.05f, 0.f };
            orbital.SetMass(1.0 / 6.6743e-11);
        }

        auto orbital0 = m_Scene->CreateEntity("Orbital 0");
        {
            //orbital0.AddComponent<SpriteRendererComponent>(Vector4{ 1.f, 0.3f, 0.2f, 1.f });
            auto& crc = orbital0.AddComponent<BillboardCircleRendererComponent>();
            crc.Color = { 1.f, 0.3f, 0.2f, 1.f };
            crc.Fade = 0.f;
            crc.Thickness = 1.f;
            auto& transform = orbital0.GetComponent<TransformComponent>();
            transform.SetPosition({ 0.9f, 0.f, 0.f });
            transform.SetScale({ 0.1f, 0.1f, 0.f });
            auto& oc = orbital0.AddComponent<OrbitalComponent>();
            oc.SetMass(1e5f);
            oc.UIColor = { 1.f, 0.3f, 0.2f };
        }

        auto orbital1 = m_Scene->CreateEntity("Orbital 1");
        {
            auto& crc = orbital1.AddComponent<BillboardCircleRendererComponent>();
            crc.Color = { 0.3f, 0.2f, 1.f, 1.f };
            crc.Fade = 0.f;
            crc.Thickness = 1.f;
            auto& transform = orbital1.GetComponent<TransformComponent>();
            transform.SetPosition({ 0.f, 0.f,-0.5f });
            transform.SetScale({ 0.1f, 0.1f, 0.f });
            auto& oc = orbital1.AddComponent<OrbitalComponent>();
            oc.SetMass(1e5f);
            oc.UIColor = { 0.3f, 0.2f, 1.f };
        }

        auto playerShip = m_Scene->CreateEntity("Player Ship");
        {
            playerShip.Parent(orbital0);

            auto& crc = playerShip.AddComponent<BillboardCircleRendererComponent>();
            crc.Color = { 0.9f, 0.9f, 0.9f, 1.f };
            crc.Fade = 0.f;
            crc.Thickness = 1.f;
            auto& transform = playerShip.GetComponent<TransformComponent>();
            transform.SetPosition({-0.7f, 0.f, 0.f });
            transform.SetScale({ 0.1f, 0.1f, 0.f });
            auto& oc = playerShip.AddComponent<OrbitalComponent>();
            oc.SetMass(1e-11f);
            oc.SetDynamic();
            oc.UIColor = { 0.9f, 0.9f, 0.9f };
        }
#else
        m_Scene = CreateRef<Scene>();

        auto commandLineArgs = Application::Get().GetCommandLineArgs();
        if (commandLineArgs.Count > 1)
        {
            std::string sceneFilePath = commandLineArgs[1];
            if (!SceneSerializer::Deserialize(m_Scene.get(), sceneFilePath)) {
                LV_CORE_ERROR("Could not default load scene!");
            }
        }

    #ifdef EXCLUDE_SETUP
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
            auto& src = square.AddComponent<SpriteRendererComponent>(Vector4{ 0.2f, 1.f, 0.3f, 1.f });
            src.Color.w = 0.6f;
        }

        Entity subSquare = m_Scene->CreateEntity("Sub-Square");
        {
            auto& src = subSquare.AddComponent<SpriteRendererComponent>(Vector4{ 1.f, 0.8f, 0.3f, 1.f });
            auto& transform = subSquare.GetComponent<TransformComponent>();
            transform.Set({ 0.2f }, { 0.5f, 0.5f, 0.2f });
            m_Scene->SetParent(subSquare, square);
        }

        Entity circle = m_Scene->CreateEntity("Circle");
        {
            auto& crc = circle.AddComponent<CircleRendererComponent>();
            crc.Fade = 0.12f;
            auto& transform = circle.GetComponent<TransformComponent>();
            transform.Set({ 0.4f }, {-0.5f,-0.5f, 0.2f });
        }

        Entity ellipse = m_Scene->CreateEntity("Ellipse");
        {
            auto& erc = ellipse.AddComponent<EllipseRendererComponent>();
            auto& transform = ellipse.GetComponent<TransformComponent>();
            transform.Set({ 0.6f, 0.3f, 0.f }, {-0.5f, 0.5f, 0.2f });
        }
    #endif
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
            LV_PROFILE_SCOPE("EditorLayer::OnUpdate");

#ifdef LV_EDITOR_USE_ORBITAL
            /* Do orbital stuff */
#else
            /* Do non-orbital stuff */
#endif

            // Update scene
            //m_Scene->OnUpdateRuntime(dT);
            m_Scene->OnUpdateEditor(dT);

            m_EditorCamera.OnUpdate(dT);
        }

        // Render
        Renderer2D::ResetStatistics();
        {
            LV_PROFILE_SCOPE("Render Prep - EditorLayer::OnUpdate");

            m_Framebuffer->Bind();

            RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.f });
            RenderCommand::Clear();

            m_Framebuffer->ClearAttachment(1, -1); /* must come after RenderCommand::Clear() */
        }

        {
            LV_PROFILE_SCOPE("Render Draw - EditorLayer::OnUpdate");

            //m_Scene->OnRenderRuntime();
            m_Scene->OnRenderEditor(m_EditorCamera);

            // Mouse hovering entities
            int mouseX = (int)(ImGui::GetMousePos().x - m_ViewportBounds[0].x);
            int mouseY = (int)(m_ViewportBounds[1].y - ImGui::GetMousePos().y);

            auto viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
            int pixelData = -1;
            if (mouseX >= 0 && mouseY >= 0 &&
                mouseX < viewportSize.x && mouseY < viewportSize.y)
            {
                pixelData = m_Framebuffer->ReadPixel(mouseX, mouseY, 1);
            }
            m_HoveredEntity = (pixelData == -1) ? Entity::Null
                : Entity{ (entt::entity)pixelData, m_Scene.get() };

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
                if (ImGui::MenuItem("New", "Ctrl+N")) {
                    NewScene();
                }

                if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                    OpenScene();
                }

                if (ImGui::MenuItem("Save As", "Ctrl+Shift+S")) {
                    SaveSceneAs();
                }

                ImGui::Separator();

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

        if (Entity activeCamera = m_Scene->GetActiveCamera())
        {
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
        }

        ImGui::Separator();

        std::string hoveredEntityTag = m_HoveredEntity
            ? m_HoveredEntity.GetComponent<TagComponent>().Tag
            : "None";
        ImGui::Text("Hovered entity: %s", hoveredEntityTag.c_str());

#ifdef LV_EDITOR_USE_ORBITAL

        ImGui::Separator();

        {
            double rootScaling = m_Scene->GetRootScaling();
            if (LimnGui::InputScientific("RootScaling", rootScaling)) {
                m_Scene->SetRootScaling(rootScaling);
            }
        }


        ImGui::Checkbox("Show reference axes", &m_Scene->m_ShowReferenceAxes);
        ImGui::BeginDisabled(!m_Scene->m_ShowReferenceAxes);
        if (ImGui::TreeNodeEx("##ReferenceAxes", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::ColorEdit4("Color", m_Scene->m_ReferenceAxisColor.Ptr(), ImGuiColorEditFlags_AlphaBar);
            ImGui::DragFloat("Length", &m_Scene->m_ReferenceAxisLength, 0.01f, 0.01f, 1.f, "%.2f");
            ImGui::DragFloat("Thickness", &m_Scene->m_ReferenceAxisThickness, 0.001f, 0.001f, 0.1f, "%.3f");
            ImGui::DragFloat("Arrow Head Size", &m_Scene->m_ReferenceAxisArrowSize, 0.001f, 0.001f, 0.5f, "%.3f");

            ImGui::TreePop();
        }
        ImGui::EndDisabled();

        if (ImGui::TreeNodeEx("Influence Visuals", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::ColorEdit4("Color", m_Scene->m_LocalSpaceColor.Ptr(), ImGuiColorEditFlags_AlphaBar);
            ImGui::DragFloat("Thickness", &m_Scene->m_LocalSpaceThickness, 0.001f, 0.001f, 1.f, "%.3f");
            ImGui::DragFloat("Fade", &m_Scene->m_LocalSpaceFade, 0.001f, 0.001f, 1.f, "%.3f");

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Orbit Visuals", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat("Thickness", &m_Scene->m_OrbitThickness, 0.001f, 0.001f, 1.f, "%.3f");
            ImGui::DragFloat("Thickness Factor", &m_Scene->m_OrbitThicknessFactor, 0.001f, 0.001f, 1.f, "%.3f");
            ImGui::DragFloat("Alpha", &m_Scene->m_OrbitAlpha, 0.001f, 0.f, 1.f, "%.3f");
            ImGui::DragFloat("Plot Point Radius", &m_Scene->m_OrbitPointRadius, 0.001f, 0.001f, 0.1f, "%.3f");

            if (ImGui::TreeNodeEx("Perifocal Frame", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat("Thickness", &m_Scene->m_PerifocalAxisThickness, 0.001f, 0.001f, 0.1f, "%.3f");
                ImGui::DragFloat("Arrow Head Size", &m_Scene->m_PerifocalAxisArrowSize, 0.001f, 0.001f, 0.5f, "%.3f");
                ImGui::TreePop();
            }

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


        // Viewport //

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.f, 0.f });
        ImGui::Begin("Viewport");

        // Viewport bounds in screen space
        auto viewportRegionMin = ImGui::GetWindowContentRegionMin();
        auto viewportRegionMax = ImGui::GetWindowContentRegionMax();
        auto viewportOffset = ImGui::GetWindowPos();
        m_ViewportBounds[0] = { viewportRegionMin.x + viewportOffset.x, viewportRegionMin.y + viewportOffset.y };
        m_ViewportBounds[1] = { viewportRegionMax.x + viewportOffset.x, viewportRegionMax.y + viewportOffset.y };

        // Only control the camera if the viewport is focused and hovered
        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();
        m_EditorCamera.SetControl(m_ViewportHovered, m_ViewportFocused, m_SceneHierarchyPanel.GetSelectedEntity());
        Application::Get().GetImGuiLayer()->SetBlockEvents(!m_ViewportFocused && !m_ViewportHovered);

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        Vector2 viewportSize{ ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y };
        if (viewportSize != m_ViewportSize && viewportSize.x > 0 && viewportSize.y > 0)
        {
            m_ViewportSize = viewportSize;
            float aspect = viewportSize.x / viewportSize.y;

            m_Framebuffer->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
            m_EditorCamera.SetAspect(aspect);
            m_Scene->OnWindowChangeAspect(aspect);
        }
        uint32_t viewportRendererId = m_Framebuffer->GetColorAttachmentRendererId();
        ImGui::Image((void*)viewportRendererId, viewportPanelSize, { 0, 1 }, { 1, 0 });

        // Gizmos
        if (m_ActiveGizmo > -1)
        {
            Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
            Entity activeCameraEntity = m_Scene->GetActiveCamera();
            if (selectedEntity && activeCameraEntity)
            {
                ImGuizmo::SetOrthographic(false); // TODO - check if editor camera is orthographic
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y,
                    ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

                Matrix4 view = m_EditorCamera.GetCamera().GetView();
                Matrix4 proj = m_EditorCamera.GetCamera().GetProjection();

                auto& tc = selectedEntity.GetComponent<TransformComponent>();
                Matrix4 transform = tc.GetTransform();

                // Snapping
                bool snap = Input::IsKeyPressed(LV_KEY_LEFT_CONTROL);
                float snapValue = 0.f;
                switch (m_ActiveGizmo) {
                case ImGuizmo::OPERATION::TRANSLATE:    snapValue = m_SnapTranslate; break;
                case ImGuizmo::OPERATION::ROTATE:       snapValue = m_SnapRotate; break;
                case ImGuizmo::OPERATION::SCALE:        snapValue = m_SnapScale; break;
                }
                float snapValues[3] = { snapValue, snapValue, snapValue };

                // Draw gizmo
                ImGuizmo::Manipulate(view.Ptr(), proj.Ptr(),
                    (ImGuizmo::OPERATION)m_ActiveGizmo, ImGuizmo::LOCAL, transform.Ptr(),
                    nullptr, snap ? snapValues : nullptr);

                if (ImGuizmo::IsUsing())
                {
                    Vector3 position, scale;
                    Quaternion orientation;
                    DecomposeTransform(transform, position, orientation, scale);

                    tc.SetPosition(position);
                    tc.SetOrientation(orientation);
                    tc.SetScale(scale);
                }
            }
        }

        ImGui::End(); // Viewport
        ImGui::PopStyleVar();

        // Scene hierarchy
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
        if (e.GetEventType() != EventType::WindowResize)
        {
            m_EditorCamera.OnEvent(e);
        }

        m_Scene->OnEvent(e);

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>(LV_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
        dispatcher.Dispatch<MouseButtonPressedEvent>(LV_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
    }


    bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
    {
        bool ctrl = Input::IsKeyPressed(LV_KEY_LEFT_CONTROL) || Input::IsKeyPressed(LV_KEY_RIGHT_CONTROL);
        bool shift = Input::IsKeyPressed(LV_KEY_LEFT_SHIFT) || Input::IsKeyPressed(LV_KEY_RIGHT_SHIFT);

        // Shortcuts
        switch (e.GetKeyCode())
        {
            // Gizmo
        case LV_KEY_Q:
            m_ActiveGizmo = -1;
            break;
        case LV_KEY_W:
            m_ActiveGizmo = ImGuizmo::OPERATION::TRANSLATE;
            break;
        case LV_KEY_E:
            m_ActiveGizmo = ImGuizmo::OPERATION::ROTATE;
            break;
        case LV_KEY_R:
            m_ActiveGizmo = ImGuizmo::OPERATION::SCALE;
            break;

            // File
        case LV_KEY_N:
            if (ctrl) {
                NewScene();
            }
            break;
        case LV_KEY_O:
            if (ctrl) {
                OpenScene();
            }
            break;
        case LV_KEY_S:
            if (ctrl && shift) {
                SaveSceneAs();
            }
            break;
        }
        return false;
    }


    bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
    {
        if (e.GetMouseButton() == LV_MOUSE_BUTTON_LEFT)
        {
            if (CanMousePick()) {
                m_SceneHierarchyPanel.SetSelectedEntity(m_HoveredEntity);
            }
        }
        return false;
    }


    bool EditorLayer::CanMousePick()
    {
        return m_ViewportHovered && !ImGuizmo::IsOver();
    }


    void EditorLayer::NewScene()
    {
#ifdef LV_EDITOR_USE_ORBITAL
        m_Scene = CreateRef<OrbitalScene>();
#else
        m_Scene = CreateRef<Scene>();
#endif
        m_Scene->OnWindowChangeAspect(m_ViewportSize.x / m_ViewportSize.y);
        m_SceneHierarchyPanel.SetContext(m_Scene.get());
    }


    void EditorLayer::OpenScene()
    {
        std::string filepath = FileDialogs::OpenFile("Limnova Scene (*.limn)\0*.limn\0");

        if (!filepath.empty())
        {
            NewScene();
            SceneSerializer::Deserialize(m_Scene.get(), filepath);
        }
    }


    void EditorLayer::SaveSceneAs()
    {
        std::string filepath = FileDialogs::SaveFile("Limnova Scene (*.limn)\0*.limn\0");
        
        if (!filepath.empty())
        {
            SceneSerializer::Serialize(m_Scene.get(), filepath);
        }
    }

}
