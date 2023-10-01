#include "EditorLayer.h"

#include "Resources/NativeScripts/CameraScripts.h"

#include <Scene/SceneSerializer.h>
#include <Utils/PlatformUtils.h>
#include <imguizmo/ImGuizmo.h>

#define LV_EDITOR_RES_DIR "C:\\Programming\\source\\Limnova\\LimnovaEditor\\Resources"


namespace Limnova
{
    extern const std::filesystem::path s_AssetDirectoryPath;


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

        m_ActiveScene = CreateRef<OrbitalScene>();

        auto commandLineArgs = Application::Get().GetCommandLineArgs();
        if (commandLineArgs.Count > 2)
        {
            std::string sceneFilePath = commandLineArgs[2];
            if (!SceneSerializer::Deserialize(m_ActiveScene.get(), sceneFilePath)) {
                LV_CORE_ERROR("Could not load default scene!");
            }
        }

    #ifdef LV_DEBUG
        for (int i = 0; i < kUpdateDurationPlotSpan; i++) m_PhysicsUpdateDurations[i] = 0.f;
    #endif

    #ifdef EXCLUDE_SETUP
        auto camera = m_ActiveScene->CreateEntity("Camera");
        {
            auto& cc = camera.AddComponent<CameraComponent>();
            cc.SetPerspectiveFov(Radiansf(80.f));
            camera.AddComponent<NativeScriptComponent>().Bind<OrbitalCameraScript>();
        }

        m_ActiveScene->SetRootScaling(10.0);
        auto root = m_ActiveScene->GetRoot();
        {
            auto& crc = root.AddComponent<BillboardCircleRendererComponent>();
            crc.Color = { 1.f, 1.f, 0.9f, 1.f };
            crc.Fade = 0.f;
            crc.Thickness = 1.f;
            auto& orbital = root.GetComponent<OrbitalComponent>();
            orbital.LocalScale = { 0.05f, 0.05f, 0.f };
            orbital.SetMass(1.0 / 6.6743e-11);
        }

        auto orbital0 = m_ActiveScene->CreateEntity("Orbital 0");
        {
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

        auto orbital1 = m_ActiveScene->CreateEntity("Orbital 1");
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

        auto playerShip = m_ActiveScene->CreateEntity("Player Ship");
        {
            playerShip.Parent(orbital0);
            m_ActiveScene->SetViewPrimary(orbital0);

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
            oc.SetVelocity({ 0.f, 0.f, 0.21f });
            oc.UIColor = { 0.9f, 0.9f, 0.9f };
        }
    #endif

#else
        m_ActiveScene = CreateRef<Scene>();

        auto commandLineArgs = Application::Get().GetCommandLineArgs();
        if (commandLineArgs.Count > 1)
        {
            std::string sceneFilePath = commandLineArgs[1];
            if (!SceneSerializer::Deserialize(m_ActiveScene.get(), sceneFilePath)) {
                LV_CORE_ERROR("Could not load default scene!");
            }
        }

    #ifdef EXCLUDE_SETUP
        Entity camera0 = m_ActiveScene->CreateEntity("Camera 0");
        {
            camera0.AddComponent<CameraComponent>();
            auto& transform = camera0.GetComponent<TransformComponent>();
            transform.Set({ 1.f }, { 0.f, 0.f, 2.f });
        }

        Entity camera1 = m_ActiveScene->CreateEntity("Camera 1");
        {
            camera1.AddComponent<CameraComponent>();
            auto& transform = camera1.GetComponent<TransformComponent>();
            transform.Set({ 1.f }, { 0.f, 0.f, 3.f });
        }

        m_ActiveScene->SetActiveCamera(camera0);

        {
            auto& script = camera0.AddComponent<NativeScriptComponent>();
            script.Bind<PlanarCameraScript>();
        }
        {
            auto& script = camera1.AddComponent<NativeScriptComponent>();
            script.Bind<PlanarCameraScript>();
        }

        // Renderables
        Entity square = m_ActiveScene->CreateEntity("Default Square");
        {
            auto& src = square.AddComponent<SpriteRendererComponent>(Vector4{ 0.2f, 1.f, 0.3f, 1.f });
            src.Color.w = 0.6f;
        }

        Entity subSquare = m_ActiveScene->CreateEntity("Sub-Square");
        {
            auto& src = subSquare.AddComponent<SpriteRendererComponent>(Vector4{ 1.f, 0.8f, 0.3f, 1.f });
            auto& transform = subSquare.GetComponent<TransformComponent>();
            transform.Set({ 0.2f }, { 0.5f, 0.5f, 0.2f });
            m_ActiveScene->SetParent(subSquare, square);
        }

        Entity circle = m_ActiveScene->CreateEntity("Circle");
        {
            auto& crc = circle.AddComponent<CircleRendererComponent>();
            crc.Fade = 0.12f;
            auto& transform = circle.GetComponent<TransformComponent>();
            transform.Set({ 0.4f }, {-0.5f,-0.5f, 0.2f });
        }

        Entity ellipse = m_ActiveScene->CreateEntity("Ellipse");
        {
            auto& erc = ellipse.AddComponent<EllipseRendererComponent>();
            auto& transform = ellipse.GetComponent<TransformComponent>();
            transform.Set({ 0.6f, 0.3f, 0.f }, {-0.5f, 0.5f, 0.2f });
        }
    #endif
#endif

        m_EditorScene = m_ActiveScene;

        m_SceneHierarchyPanel.SetContext(m_ActiveScene.get());

        m_IconPlay = Texture2D::Create(LV_EDITOR_RES_DIR"\\Icons\\PlayButton.png");
        m_IconPause = Texture2D::Create(LV_EDITOR_RES_DIR"\\Icons\\PauseButton.png");
        m_IconStop = Texture2D::Create(LV_EDITOR_RES_DIR"\\Icons\\StopButton.png");
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
            switch (m_SceneState)
            {
            case SceneState::Edit:
            case SceneState::Pause:
            {
                m_EditorCamera.OnUpdate(dT);
                m_ActiveScene->OnUpdateEditor(dT);
                break;
            }
            case SceneState::Simulate:
            {
                m_EditorCamera.OnUpdate(dT); /* Simulate uses editor camera so we update it */
                dT = dT * m_SceneDTMultiplier;
                // fallthrough to Play
            }
            case SceneState::Play:
            {
                m_ActiveScene->OnUpdateRuntime(dT);
                break;
            }
            }
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

            switch (m_SceneState)
            {
            case SceneState::Edit:
            case SceneState::Simulate:
            case SceneState::Pause:
            {
                m_ActiveScene->OnRenderEditor(m_EditorCamera);
                break;
            }
            case SceneState::Play:
            {
                m_ActiveScene->OnRenderRuntime();
                break;
            }
            }

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
                : Entity{ (entt::entity)pixelData, m_ActiveScene.get() };

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

        if (Entity activeCamera = m_ActiveScene->GetActiveCamera())
        {
            if (ImGui::BeginCombo("Camera", activeCamera.GetComponent<TagComponent>().Tag.c_str()))
            {
                auto cameraEntities = m_ActiveScene->GetEntitiesByComponents<CameraComponent>();
                for (auto& entity : cameraEntities)
                {
                    if (ImGui::Selectable(entity.GetComponent<TagComponent>().Tag.c_str(), activeCamera == entity))
                    {
                        activeCamera = entity;
                        m_ActiveScene->SetActiveCamera(entity);
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
            double rootScaling = m_ActiveScene->GetRootScaling();
            if (LimnGui::InputScientific("RootScaling", rootScaling)) {
                m_ActiveScene->SetRootScaling(rootScaling);
            }
        }

        ImGui::Checkbox("Show view space boundary", &m_ActiveScene->m_ShowViewSpace);

        ImGui::Checkbox("Show reference axes", &m_ActiveScene->m_ShowReferenceAxes);
        ImGui::BeginDisabled(!m_ActiveScene->m_ShowReferenceAxes);
        if (ImGui::TreeNodeEx("##ReferenceAxes", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::ColorEdit4("Color", m_ActiveScene->m_ReferenceAxisColor.Ptr(), ImGuiColorEditFlags_AlphaBar);
            ImGui::DragFloat("Length", &m_ActiveScene->m_ReferenceAxisLength, 0.01f, 0.01f, 1.f, "%.2f");
            ImGui::DragFloat("Thickness", &m_ActiveScene->m_ReferenceAxisThickness, 0.001f, 0.001f, 0.1f, "%.3f");
            ImGui::DragFloat("Arrow Head Size", &m_ActiveScene->m_ReferenceAxisArrowSize, 0.001f, 0.001f, 0.5f, "%.3f");

            ImGui::TreePop();
        }
        ImGui::EndDisabled();

        if (ImGui::TreeNodeEx("Influence Visuals", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::ColorEdit4("Color", m_ActiveScene->m_LocalSpaceColor.Ptr(), ImGuiColorEditFlags_AlphaBar);
            ImGui::DragFloat("Thickness", &m_ActiveScene->m_LocalSpaceThickness, 0.001f, 0.001f, 1.f, "%.3f");
            ImGui::DragFloat("Fade", &m_ActiveScene->m_LocalSpaceFade, 0.001f, 0.001f, 1.f, "%.3f");

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Orbit Visuals", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat("Thickness", &m_ActiveScene->m_OrbitThickness, 0.001f, 0.001f, 1.f, "%.3f");
            ImGui::DragFloat("Alpha", &m_ActiveScene->m_OrbitAlpha, 0.001f, 0.f, 1.f, "%.3f");
            ImGui::DragFloat("Plot Point Radius", &m_ActiveScene->m_OrbitPointRadius, 0.001f, 0.001f, 0.1f, "%.3f");

            if (ImGui::TreeNodeEx("Perifocal Frame", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat("Thickness", &m_ActiveScene->m_PerifocalAxisThickness, 0.001f, 0.001f, 0.1f, "%.3f");
                ImGui::DragFloat("Arrow Head Size", &m_ActiveScene->m_PerifocalAxisArrowSize, 0.001f, 0.001f, 0.5f, "%.3f");
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


#if defined(LV_DEBUG) && defined(LV_EDITOR_USE_ORBITAL) && defined(TEMP_EXCLUDE)
        ImGui::Begin("OrbitalPhysics Statistics");
        auto& orbitalStats = m_ActiveScene->GetPhysicsStats();
        {
            bool update = m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulate;

            static float updateDurationMax = 0.f;
            if (update) m_PhysicsUpdateDurations[m_PhysicsUpdateDurationsOffset] = (float)orbitalStats.UpdateTime.count();
            updateDurationMax = std::max(updateDurationMax, m_PhysicsUpdateDurations[m_PhysicsUpdateDurationsOffset]);
            if (ImGui::TreeNode("OnUpdate() duration")) {
                ImGui::PlotLines("##OnUpdateDuration", m_PhysicsUpdateDurations.data(), kUpdateDurationPlotSpan, m_PhysicsUpdateDurationsOffset, 0, 0.f, updateDurationMax, ImVec2{ImGui::GetContentRegionAvail().x - 20, 60});
                ImGui::Text("Max: %f", updateDurationMax);

                static constexpr int kRollingAverageSpan = 60;
                float rollingAverage = m_PhysicsUpdateDurations[m_PhysicsUpdateDurationsOffset];
                for (int i = 1; i < kRollingAverageSpan; i++) {
                    rollingAverage += m_PhysicsUpdateDurations[Wrapi(m_PhysicsUpdateDurationsOffset - i, 0, kUpdateDurationPlotSpan)];
                }
                rollingAverage /= (float)kRollingAverageSpan;
                ImGui::Text("Rolling avg.: %f (across %d samples)", rollingAverage, kRollingAverageSpan);

                ImGui::TreePop();
            }
            if (update) m_PhysicsUpdateDurationsOffset = Wrapi(++m_PhysicsUpdateDurationsOffset, kUpdateDurationPlotSpan);
        }

        auto& objStats = orbitalStats.ObjStats;

        { // Object update counts
            m_ResizeInit(m_ObjectUpdates, objStats.size(), 0.f);
            bool expanded = ImGui::TreeNode("Object update counts");
            for (size_t object = 1; object < objStats.size(); object++) {
                m_ObjectUpdates[object][m_ObjectUpdatesOffset] = (float)objStats[object].NumObjectUpdates;
                if (expanded) {
                    std::ostringstream title; title << object;
                    ImGui::PlotLines(title.str().c_str(), m_ObjectUpdates[object].data(), kObjPlotSpan, m_ObjectUpdatesOffset, 0, 0.f, 20.f, ImVec2{ ImGui::GetContentRegionAvail().x - 20, 60 });
                }
            }
            m_ObjectUpdatesOffset = Wrapi(++m_ObjectUpdatesOffset, kObjPlotSpan);
            if (expanded) ImGui::TreePop();
        }

        { // Object orbit durations
            m_ResizeInit(m_DurationErrors, objStats.size(), 0.f);
            m_DurationErrorsOffsets.resize(objStats.size(), kObjPlotSpan - 1); /* initialize offsets to max so that we can increment before assignment of new value */

            bool expanded = ImGui::TreeNode("Orbit duration errors");
            for (size_t object = 1; object < objStats.size(); object++)
            {
                Entity entity = m_ActiveScene->GetEntity(m_ActiveScene->GetPhysicsObjectUser(object));
                auto& elems = entity.GetComponent<OrbitalComponent>().GetElements();
                int offset = m_DurationErrorsOffsets[object];
                float error = (float)objStats[object].LastOrbitDurationError;
                bool errorUpdated = error != m_DurationErrors[object][offset];
                if (errorUpdated) {
                    offset = Wrapi(offset + 1, kObjPlotSpan);
                    m_DurationErrors[object][offset] = error;
                    m_DurationErrorsOffsets[object] = offset;
                }
                if (expanded) {
                    std::ostringstream title; title << (uint32_t)entity;
                    ImGui::PlotLines(title.str().c_str(), m_DurationErrors[object].data(), kObjPlotSpan, offset, 0, -1.f, 1.f, ImVec2{ImGui::GetContentRegionAvail().x - 20, 60});
                }
            }
            if (expanded) ImGui::TreePop();
        }

        ImGui::End();
#endif


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
        //Application::Get().GetImGuiLayer()->SetBlockEvents(!m_ViewportFocused && !m_ViewportHovered);

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        Vector2 viewportSize{ ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y };
        if (viewportSize != m_ViewportSize && viewportSize.x > 0 && viewportSize.y > 0)
        {
            m_ViewportSize = viewportSize;
            float aspect = viewportSize.x / viewportSize.y;

            m_Framebuffer->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
            m_EditorCamera.SetAspect(aspect);
            m_ActiveScene->OnWindowChangeAspect(aspect);
        }
        uint32_t viewportRendererId = m_Framebuffer->GetColorAttachmentRendererId();
        ImGui::Image((void*)viewportRendererId, viewportPanelSize, { 0, 1 }, { 1, 0 });

        // Scene drag & drop
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSERT_BROWSER_ITEM")) {
                const wchar_t* path = (const wchar_t*)payload->Data;
                OpenScene(s_AssetDirectoryPath / path);
            }
            ImGui::EndDragDropTarget();
        }

        // Gizmos
        if (m_ActiveGizmo > -1)
        {
            Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
            Entity activeCameraEntity = m_ActiveScene->GetActiveCamera();
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

        // Panels
        m_SceneHierarchyPanel.OnImGuiRender();
        m_AssetBrowserPanel.OnImGuiRender();

        UI_Toolbar();

        ImGui::End(); // DockSpace

        ImGui::ShowDemoWindow();
    }


    void EditorLayer::UI_Toolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.f, 2.f });
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2{ 0.f, 0.f });

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.f, 0.f, 0.f, 0.f });
        auto& colorButtonHovered = ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered];
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ colorButtonHovered.x, colorButtonHovered.y, colorButtonHovered.z, 0.5f });
        auto& colorButtonActive = ImGui::GetStyle().Colors[ImGuiCol_ButtonActive];
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ colorButtonActive.x, colorButtonActive.y, colorButtonActive.z, 0.5f });

        ImGui::Begin("##toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove);

        // Play button
        float mid = ImGui::GetWindowContentRegionMax().x * 0.5f;
        float size = ImGui::GetWindowHeight() - 8.f;
        float pad = size * 0.1f;
        const Ref<Texture2D>& playButtonIcon = (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Pause) ? m_IconPlay : m_IconPause;
        ImGui::SetCursorPosX(mid - size - pad);
        if (ImGui::ImageButton("##playButton", (ImTextureID)playButtonIcon->GetRendererId(), ImVec2{ size, size }))
        {
            switch (m_SceneState)
            {
            case SceneState::Edit:
                //OnScenePlay();
                OnSceneSimulate();
                break;
            case SceneState::Simulate:
                m_SceneState = SceneState::Pause;
                break;
            case SceneState::Pause:
                m_SceneState = SceneState::Simulate;
                break;
            }
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(mid + pad);
        if (ImGui::ImageButton("##stopButton", (ImTextureID)m_IconStop->GetRendererId(), ImVec2{ size, size }))
        {
            switch (m_SceneState)
            {
            case SceneState::Edit:
                break;
            case SceneState::Simulate:
            case SceneState::Play:
            case SceneState::Pause:
                OnSceneStop();
                break;
            }
        }

        ImGui::SameLine();
        ImGui::SetCursorPosX(2.f * mid - 250.f);
        static const LimnGui::InputConfig<float> config {
            1.f,    // ResetValue
            0.1f,   // Speed
            1.f,    // FastSpeed 
            0.1f,   // Min
            1000.f, // Max
            3,      // Precision
            false,  // Scientific
            false,  // ReadOnly
            0,      // WidgetId
            80,     // LabelWidth
            120,     // WidgetWidth
            "Delta-time multiplier: multiplied with frame dT before being passed to Scene::OnUpdate.\n"
            "Effectively a time dilation tool for controlling the apparent timescale of the game scene."
        };
        LimnGui::SliderFloat("dT mult.", m_SceneDTMultiplier, config, true);
        /*LimnGui::HelpMarker("Delta-time multiplier: multiplied with frame dT before being passed to Scene::OnUpdate.\n"
            "Effectively a time dilation tool for controlling the apparent timescale of the game scene.");*/

        ImGui::End(); // toolbar

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
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

        m_ActiveScene->OnEvent(e);

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
            if (ctrl && !shift) {
                SaveScene();
            }
            break;

            // Scene
        case LV_KEY_D:
            if (ctrl) {
                OnDuplicateEntity();
            }

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
        if (m_SceneState != SceneState::Edit) {
            OnSceneStop();
        }

#ifdef LV_EDITOR_USE_ORBITAL
        m_EditorScene = CreateRef<OrbitalScene>();
#else
        m_EditorScene = CreateRef<Scene>();
#endif
        m_EditorScene->OnWindowChangeAspect(m_ViewportSize.x / m_ViewportSize.y);
        m_SceneHierarchyPanel.SetContext(m_EditorScene.get());
        m_ActiveScene = m_EditorScene;
        m_EditorScenePath.clear();
    }


    void EditorLayer::OpenScene()
    {
        std::string filepath = FileDialogs::OpenFile("Limnova Scene (*.limn)\0*.limn\0");
        if (!filepath.empty()) {
            OpenScene(filepath);
        }
    }


    void EditorLayer::OpenScene(std::filesystem::path filepath)
    {
        NewScene();
        SceneSerializer::Deserialize(m_EditorScene.get(), filepath.string());
        m_EditorScenePath = filepath;
    }


    void EditorLayer::SaveScene()
    {
        if (!m_EditorScenePath.empty()) {
            SceneSerializer::Serialize(m_EditorScene.get(), m_EditorScenePath.string());
        }
        else {
            SaveSceneAs();
        }
    }


    void EditorLayer::SaveSceneAs()
    {
        std::string filepath = FileDialogs::SaveFile("Limnova Scene (*.limn)\0*.limn\0");

        if (!filepath.empty()) {
            SceneSerializer::Serialize(m_EditorScene.get(), filepath);
            m_EditorScenePath = filepath;
        }
    }


    void EditorLayer::OnScenePlay()
    {
        m_SceneState = SceneState::Play;

#ifdef LV_EDITOR_USE_ORBITAL
        m_ActiveScene = OrbitalScene::Copy(m_EditorScene);
#else
        m_ActiveScene = Scene::Copy(m_EditorScene);
#endif
        m_ActiveScene->OnStartRuntime();

        m_SceneHierarchyPanel.SetContext(m_ActiveScene.get());
    }


    void EditorLayer::OnSceneSimulate()
    {
        m_SceneState = SceneState::Simulate;

#ifdef LV_EDITOR_USE_ORBITAL
        m_ActiveScene = OrbitalScene::Copy(m_EditorScene);

    #if defined(LV_DEBUG) && defined(TEMP_EXCLUDE)
        size_t numObjs = m_ActiveScene->GetPhysicsStats().ObjStats.size();
        m_ObjectUpdates.clear();
        m_ResizeInit(m_ObjectUpdates, numObjs, 0.f);
        m_DurationErrors.clear();
        m_ResizeInit(m_DurationErrors, numObjs, 0.f);
        m_DurationErrorsOffsets.clear();
        m_DurationErrorsOffsets.resize(numObjs, 0);
    #endif
#else
        m_ActiveScene = Scene::Copy(m_EditorScene);
#endif
        m_ActiveScene->OnStartRuntime();

        m_SceneHierarchyPanel.SetContext(m_ActiveScene.get());
    }


    void EditorLayer::OnSceneStop()
    {
        m_SceneState = SceneState::Edit;

        m_ActiveScene->OnStopRuntime();

        m_ActiveScene = m_EditorScene;

        m_SceneHierarchyPanel.SetContext(m_EditorScene.get());
        m_EditorScene->PhysicsUseContext();
    }


    void EditorLayer::OnDuplicateEntity()
    {
        if (m_SceneState != SceneState::Edit) return;

        Entity selected = m_SceneHierarchyPanel.GetSelectedEntity();
        if (selected) {
            m_EditorScene->DuplicateEntity(selected);
        }
    }

}
