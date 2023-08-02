#include "SceneHierarchyPanel.h"

#include <imgui/imgui_internal.h>


namespace Limnova
{

    SceneHierarchyPanel::SceneHierarchyPanel(Scene* scene)
        : m_Scene(scene)
    {
    }


    void SceneHierarchyPanel::SetContext(Scene* scene)
    {
        m_Scene = scene;
        m_SelectedEntity = Entity::Null;
    }


    void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
    {
        m_SelectedEntity = entity;
    }


    void SceneHierarchyPanel::OnImGuiRender()
    {
        ImGui::Begin("Scene Hierarchy");

        EntityNode(m_Scene->GetRoot(), true);

        if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
            m_SelectedEntity = Entity::Null;
        }

        if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("Create Entity")) {
                m_Scene->CreateEntity("New Entity");
            }
            ImGui::EndPopup();
        }

        ImGui::End(); // Scene Hierarchy

        ImGui::Begin("Inspector");

        if (m_SelectedEntity) {
            Inspector(m_SelectedEntity);
        }

        ImGui::End(); // Inspector
    }


    void SceneHierarchyPanel::EntityNode(Entity entity, bool forceExpanded)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (forceExpanded) flags |= ImGuiTreeNodeFlags_DefaultOpen;
        if (entity == m_SelectedEntity) flags |= ImGuiTreeNodeFlags_Selected;

        auto& tag = entity.GetComponent<TagComponent>();

        auto children = m_Scene->GetChildren(entity);
        if (children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;

        bool expanded = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.Tag.c_str());
        bool deleteEntity = false;
        {
            if (ImGui::IsItemClicked()) {
                m_SelectedEntity = entity;
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                // Handle double-clicks on entity node:

                if (entity.HasComponent<OrbitalComponent>()) {
                    ((OrbitalScene*)m_Scene)->SetTrackingEntity(entity);
                }
            }

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Create Child Entity")) {
                    m_SelectedEntity = m_Scene->CreateEntityAsChild(entity, tag.Tag + " child");
                }

                if (ImGui::MenuItem("Duplicate Entity")) {
                    m_Scene->DuplicateEntity(entity);
                }

                if (ImGui::MenuItem("Delete Entity")) {
                    deleteEntity = true;

                    if (m_SelectedEntity == entity) {
                        m_SelectedEntity = Entity::Null;
                    }
                }
                ImGui::EndPopup();
            }
        }

        std::ostringstream oss; oss << (uint32_t)entity << ", " << (uint64_t)entity.GetUUID();
        LimnGui::HelpMarker(oss.str());

        if (expanded)
        {
            for (auto child : children)
            {
                EntityNode(child);
            }
            ImGui::TreePop();
        }

        if (deleteEntity) {
            m_Scene->DestroyEntity(entity);
        }
    }


    template<typename TComponent>
    static void ComponentInspector(Entity entity, const std::string& name, bool canBeDeleted, std::function<void()> control)
    {
        ImGui::PushID(name.c_str());

        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen
            | ImGuiTreeNodeFlags_AllowItemOverlap
            | ImGuiTreeNodeFlags_Framed
            | ImGuiTreeNodeFlags_FramePadding;

        ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();

        if (entity.HasComponent<TComponent>())
        {
            ImGui::Separator();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4.f, 4.f });
            float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.f;

            bool expanded = ImGui::TreeNodeEx((void*)typeid(TComponent).hash_code(), treeNodeFlags, name.c_str());
            ImGui::PopStyleVar();

            ImGui::SameLine(contentRegionAvail.x - lineHeight * 0.5f);
            if (ImGui::Button("...", ImVec2{ lineHeight, lineHeight })) {
                ImGui::OpenPopup("ComponentOptions");
            }
            bool removeComponent = false;
            if (ImGui::BeginPopup("ComponentOptions"))
            {
                if (canBeDeleted) {
                    if (ImGui::MenuItem("Remove Component")) {
                        removeComponent = true;
                    }
                }
                ImGui::EndPopup();
            }

            if (expanded)
            {
                control(); /* Here lies ye component-specific code */

                ImGui::TreePop();
            }

            if (removeComponent) {
                entity.RemoveComponent<TComponent>();
            }
        }
        ImGui::PopID();
    }


    void SceneHierarchyPanel::Inspector(Entity entity)
    {
        if (entity.HasComponent<TagComponent>())
        {
            auto& tag = entity.GetComponent<TagComponent>();

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy_s(buffer, sizeof(buffer), tag.Tag.c_str());
            if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
            {
                std::string strTag{ buffer };
                if (strTag != "Root") {
                    tag = strTag;
                }
            }
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        if (ImGui::Button("Add Component")) {
            ImGui::OpenPopup("AddComponent");
        }
        if (ImGui::BeginPopup("AddComponent"))
        {
            if (ImGui::MenuItem("Camera"))
            {
                m_SelectedEntity.AddComponent<CameraComponent>();
                ImGui::CloseCurrentPopup();
            }
#ifdef LV_EDITOR_USE_ORBITAL
            if (ImGui::MenuItem("Orbital"))
            {
                m_SelectedEntity.AddComponent<OrbitalComponent>();
                ImGui::CloseCurrentPopup();
            }
#endif
            if (ImGui::MenuItem("Sprite Renderer"))
            {
                m_SelectedEntity.AddComponent<SpriteRendererComponent>();
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Billboard Sprite Renderer"))
            {
                m_SelectedEntity.AddComponent<BillboardSpriteRendererComponent>();
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Circle Renderer"))
            {
                m_SelectedEntity.AddComponent<CircleRendererComponent>();
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Billboard Circle Renderer"))
            {
                m_SelectedEntity.AddComponent<BillboardCircleRendererComponent>();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::PopItemWidth();

        bool isOrbital = entity.HasComponent<OrbitalComponent>();
        bool isOrbitalViewPrimary = isOrbital ? entity == ((OrbitalScene*)m_Scene)->GetViewPrimary() : false;
        bool isOrbitalViewSecondary = isOrbital ? m_Scene->GetParent(entity) == ((OrbitalScene*)m_Scene)->GetViewPrimary() : false;

        ComponentInspector<TransformComponent>(entity, "Transform", false, [&]()
        {
            auto& transform = entity.GetComponent<TransformComponent>();

            // Position
            ImGui::BeginDisabled(isOrbital && !isOrbitalViewSecondary);
            {
                LimnGui::InputConfig<float> config;
                config.Speed = 0.01f;
                config.ResetValue = 0.f;
                config.Precision = 2;
                if (LimnGui::DragVec3("Position", transform.Position, config))
                {
                    transform.NeedCompute = true;

                    if (entity.HasComponent<OrbitalComponent>()) {
                        entity.GetComponent<OrbitalComponent>().Object.SetPosition(transform.Position);
                    }
                }
                ImGui::EndDisabled();
            }

            // Rotation
            ImGui::BeginDisabled(isOrbital && !(isOrbitalViewPrimary || isOrbitalViewSecondary));
            {
                Vector3 eulerAngles = DegreesVec3(transform.GetEulerAngles());
                eulerAngles.x = Wrapf(eulerAngles.x, 0.f, 360.f);
                eulerAngles.y = Wrapf(eulerAngles.y, 0.f, 360.f);
                eulerAngles.z = Wrapf(eulerAngles.z, 0.f, 360.f);
                LimnGui::InputConfig<float> config;
                config.Speed = 1.f;
                config.ResetValue = 0.f;
                config.Precision = 1;
                if (LimnGui::DragVec3("Rotation", eulerAngles, config))
                {
                    transform.SetEulerAngles(RadiansVec3(eulerAngles));
                }
                ImGui::EndDisabled();
            }

            // Scale
            ImGui::BeginDisabled(isOrbital && !isOrbitalViewPrimary);
            {
                LimnGui::InputConfig<float> config;
                config.Speed = 0.01f;
                config.ResetValue = 1.f;
                config.Precision = 2;
                if (LimnGui::DragVec3("Scale", transform.Scale, config))
                {
                    transform.NeedCompute = true;

#ifdef LV_EDITOR_USE_ORBITAL
                    entity.GetComponent<OrbitalHierarchyComponent>().AbsoluteScale =
                        ((OrbitalScene*)m_Scene)->GetLocalSpace(entity).GetLSpace().MetersPerRadius * (Vector3d)transform.Scale;
#endif
                }
                ImGui::EndDisabled();
            }
        });

        ComponentInspector<HierarchyComponent>(entity, "Hierarchy", false, [&]() {
            auto& hierarchy = entity.GetComponent<HierarchyComponent>();

            if (hierarchy.Parent != UUID::Null) {
                ImGui::Text("Parent: %s", m_Scene->GetEntity(hierarchy.Parent).GetComponent<TagComponent>().Tag.c_str());
                std::ostringstream oss; oss << (uint64_t)hierarchy.Parent;
                LimnGui::HelpMarker(oss.str());
            }
            if (hierarchy.NextSibling != UUID::Null) {
                ImGui::Text("Next sib: %s", m_Scene->GetEntity(hierarchy.NextSibling).GetComponent<TagComponent>().Tag.c_str());
                std::ostringstream oss; oss << (uint64_t)hierarchy.NextSibling;
                LimnGui::HelpMarker(oss.str());
            }
            if (hierarchy.PrevSibling != UUID::Null) {
                ImGui::Text("Prev sib: %s", m_Scene->GetEntity(hierarchy.PrevSibling).GetComponent<TagComponent>().Tag.c_str());
                std::ostringstream oss; oss << (uint64_t)hierarchy.PrevSibling;
                LimnGui::HelpMarker(oss.str());
            }
            if (hierarchy.FirstChild != UUID::Null) {
                ImGui::Text("First child: %s", m_Scene->GetEntity(hierarchy.FirstChild).GetComponent<TagComponent>().Tag.c_str());
                std::ostringstream oss; oss << (uint64_t)hierarchy.FirstChild;
                LimnGui::HelpMarker(oss.str());
            }
        });

        ComponentInspector<OrbitalHierarchyComponent>(entity, "Orbital Hierarchy", false, [&]()
        {
            auto& orbitalhc = entity.GetComponent<OrbitalHierarchyComponent>();

            // Relative local space
            ImGui::BeginDisabled(!entity.GetParent().HasComponent<OrbitalComponent>());
            {
                LimnGui::InputConfig<int> config;
                config.Speed = 1;
                config.FastSpeed = 1; /* number of local spaces belonging to parent will almost always be on the order of 1 */
                int value = orbitalhc.LocalSpaceRelativeToParent;
                if (LimnGui::InputInt("Relative Local Space", value, config))
                {
                    orbitalhc.LocalSpaceRelativeToParent = std::clamp<int>(value,
                        -1, entity.GetParent().GetComponent<OrbitalComponent>().LocalSpaces.size() - 1);
                }
            }
            ImGui::EndDisabled();

            // Absolute scale
            {
                LimnGui::InputConfig<double> config;
                config.ReadOnly = true;
                LimnGui::InputVec3d("Absolute Scale", orbitalhc.AbsoluteScale, config);
            }
        });

        ComponentInspector<CameraComponent>(entity, "Camera", true, [&]()
        {
            auto& camera = entity.GetComponent<CameraComponent>();

            ImGui::BeginDisabled(m_Scene->GetActiveCamera() == entity);
            if (ImGui::Button("Set Active"))
            {
                m_Scene->SetActiveCamera(entity);
            }
            ImGui::EndDisabled();

            bool isOrthographic = camera.GetIsOrthographic();
            if (LimnGui::Checkbox("Orthographic", isOrthographic)) {
                camera.SetIsOrthographic(isOrthographic);
            }

            if (isOrthographic)
            {
                // Orthographic options
                {
                    float height = camera.GetOrthographicHeight();
                    LimnGui::InputConfig<float> config;
                    config.Speed = 0.01f;
                    config.Min = 0.001f;
                    if (LimnGui::DragFloat("Height", height, config)) {
                        camera.SetOrthographicHeight(height);
                    }
                }

                auto [orthoNear, orthoFar] = camera.GetOrthographicClip();
                {
                    LimnGui::InputConfig<float> config;
                    config.Speed = 0.01f;
                    config.Max = orthoFar;
                    if (LimnGui::DragFloat("Near", orthoNear, config)) {
                        camera.SetOrthographicClip(orthoNear, orthoFar);
                    }
                }
                {
                    LimnGui::InputConfig<float> config;
                    config.Speed = 0.01f;
                    config.Min = orthoNear;
                    if (LimnGui::DragFloat("Far", orthoFar, config)) {
                        camera.SetOrthographicClip(orthoNear, orthoFar);
                    }
                }
            }
            else
            {
                // Perspective options
                {
                    float fov = Degreesf(camera.GetPerspectiveFov());
                    LimnGui::InputConfig<float> config;
                    config.Speed = 1.f;
                    config.Min = 1.f;
                    config.Max = 179.f;
                    config.Precision = 1;
                    if (LimnGui::DragFloat("FOV", fov, config)) {
                        camera.SetPerspectiveFov(Radiansf(fov));
                    }
                }

                auto [perspNear, perspFar] = camera.GetPerspectiveClip();
                {
                    LimnGui::InputConfig<float> config;
                    config.Speed = 0.01f;
                    config.Min = 0.001f;
                    config.Max = perspFar;
                    if (LimnGui::DragFloat("Near", perspNear, config)) {
                        camera.SetPerspectiveClip(perspNear, perspFar);
                    }
                }
                {
                    LimnGui::InputConfig<float> config;
                    config.Speed = 1.f;
                    config.Min = perspNear;
                    if (LimnGui::DragFloat("Far", perspFar, config)) {
                        camera.SetPerspectiveClip(perspNear, perspFar);
                    }
                }
            }
        });

        ComponentInspector<OrbitalComponent>(entity, "Orbital", true, [&]()
        {
            auto& orbital = entity.GetComponent<OrbitalComponent>();

            ImGui::Text("Object ID: %d", orbital.Object.Id());

            LimnGui::ColorEdit3("UI Color", orbital.UIColor);

            auto& obj = orbital.Object.GetObj();

            switch (obj.Validity)
            {
            case OrbitalPhysics::Validity::Valid:          ImGui::Text(                                "Validity: Valid");             break;
            case OrbitalPhysics::Validity::InvalidParent:  ImGui::TextColored({1.f, 0.f, 0.f, 0.8f},   "Validity: Invalid Parent!");   break;
            case OrbitalPhysics::Validity::InvalidMass:    ImGui::TextColored({1.f, 0.f, 0.f, 0.8f},   "Validity: Invalid Mass!");     break;
            case OrbitalPhysics::Validity::InvalidPosition:ImGui::TextColored({1.f, 0.f, 0.f, 0.8f},   "Validity: Invalid Position!"); break;
            case OrbitalPhysics::Validity::InvalidPath:    ImGui::TextColored({1.f, 0.f, 0.f, 0.8f},   "Validity: Invalid Path!");     break;
            }

            if (entity != m_Scene->GetRoot())
            {
                bool isDynamic = orbital.Object.IsDynamic();
                if (LimnGui::Checkbox("Dynamic", isDynamic)) {
                    orbital.Object.SetDynamic(isDynamic);
                }

                ImGui::Separator();

                ImGui::BeginDisabled(!isOrbitalViewSecondary);
                LimnGui::Checkbox("Show Major/Minor Axes", orbital.ShowMajorMinorAxes, 175.f);
                LimnGui::Checkbox("Show Normal", orbital.ShowNormal, 175.f);
                ImGui::EndDisabled();
            }

            ImGui::Separator();
            ImGui::BeginDisabled(!(isOrbitalViewPrimary || isOrbitalViewSecondary));

            ImGui::Separator();

            // Local spaces
            if (ImGui::TreeNode("Local Spaces"))
            {
                for (size_t l = 0; l < orbital.LocalSpaces.size(); l++)
                {
                    auto lspNode = orbital.LocalSpaces[l];
                    bool isInfluencing = lspNode.Influencing();
                    if (isInfluencing) { ImGui::Text("Influencing"); }
                    else { ImGui::Text("Non-influencing"); }

                    ImGui::BeginDisabled(isInfluencing);

                    float localSpaceRadius = lspNode.GetLSpace().Radius;
                    LimnGui::InputConfig<float> config;
                    config.Speed = 0.0001f;
                    config.Precision = 4;
                    config.Min = 0.01f;
                    config.Max = 2.f;
                    if (LimnGui::DragFloat("Radius", localSpaceRadius, config, 100.f)) {
                        lspNode.SetRadius(localSpaceRadius);
                    }

                    ImGui::EndDisabled();
                }
                ImGui::TreePop();
            }

            // Mass
            double mass = obj.State.Mass;
            if (LimnGui::InputScientific("Mass", mass)) {
                orbital.Object.SetMass(mass);
            }

            ImGui::EndDisabled();

            ImGui::Separator();
            ImGui::BeginDisabled(!isOrbitalViewSecondary);

            // Position
            {
                Vector3 position = obj.State.Position;
                LimnGui::InputConfig<float> config;
                config.Speed = 0.0001f;
                config.Precision = 4;
                config.ResetValue = 0.f;
                if (LimnGui::DragVec3("Position", position, config)) {
                    orbital.Object.SetPosition(position);
                }
            }

            // Velocity
            if (entity != m_Scene->GetRoot())
            {
                Vector3d velocity = obj.State.Velocity;
                LimnGui::InputConfig<double> config;
                config.Speed = 0.0001;
                config.FastSpeed = 0.01;
                config.Precision = 8;
                config.ResetValue = 0.f;
                if (LimnGui::InputVec3d("Velocity", velocity, config)) {
                    orbital.Object.SetVelocity(velocity);
                }

                if (ImGui::Button("Circularize")) {
                    orbital.SetCircular();
                }

                ImGui::SameLine();

                if (ImGui::Button("Reverse")) {
                    orbital.Object.SetVelocity(-velocity);
                }
            }

            ImGui::EndDisabled(); // (isOrbital && !isOrbitalViewSecondary)

            ImGui::Separator();

            // Elements
            if (entity != m_Scene->GetRoot())
            {
                const auto& elems = orbital.Object.GetElements();
                if (ImGui::BeginTable("Elements", 2))
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Grav");
                    LimnGui::HelpMarker("Gravity parameter");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3e", elems.Grav);

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("H");
                    LimnGui::HelpMarker("Orbital specific angular momentum");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3e", elems.H);

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("E");
                    LimnGui::HelpMarker("Eccentricity");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3f", elems.E);

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("T");
                    LimnGui::HelpMarker("Orbital period");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.5e", elems.T);

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("True anomaly");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3f", elems.TrueAnomaly);

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("a");
                    LimnGui::HelpMarker("Semi-major axis");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3f", elems.SemiMajor);

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("b");
                    LimnGui::HelpMarker("Semi-minor axis");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3f", elems.SemiMinor);

                    // TEMP - orientation
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("X");
                    LimnGui::HelpMarker("Perifocal X-axis");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3f, %.3f, %.3f", elems.PerifocalX.x, elems.PerifocalX.y, elems.PerifocalX.z);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Y");
                    LimnGui::HelpMarker("Perifocal Y-axis");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3f, %.3f, %.3f", elems.PerifocalY.x, elems.PerifocalY.y, elems.PerifocalY.z);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Z");
                    LimnGui::HelpMarker("Perifocal Z-axis");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3f, %.3f, %.3f", elems.PerifocalNormal.x, elems.PerifocalNormal.y, elems.PerifocalNormal.z);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("C");
                    LimnGui::HelpMarker("Distance from primary to orbit centre");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3f", elems.C);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("I");
                    LimnGui::HelpMarker("Inclination");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3f", elems.I);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("N");
                    LimnGui::HelpMarker("Direction of ascending node");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3f, %.3f, %.3f", elems.N.x, elems.N.y, elems.N.z);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Omega");
                    LimnGui::HelpMarker("Right ascension of ascending node");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3f", elems.Omega);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("ArgPer");
                    LimnGui::HelpMarker("Argument of periapsis");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3f", elems.ArgPeriapsis);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Ort");
                    LimnGui::HelpMarker("Orientation of perifocal frame");
                    ImGui::TableSetColumnIndex(1);
                    auto ort = elems.PerifocalOrientation.ToEulerAngles();
                    ImGui::Text("%.3f, %.3f, %.3f", ort.x, ort.y, ort.z);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Ort Q");
                    LimnGui::HelpMarker("Quaternion form of orientation");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.3f, %.3f, %.3f, %.3f", elems.PerifocalOrientation.GetX(), elems.PerifocalOrientation.GetY(),
                        elems.PerifocalOrientation.GetZ(), elems.PerifocalOrientation.GetW());

                    ImGui::EndTable();
                }
            }
        });

        ComponentInspector<SpriteRendererComponent>(entity, "Sprite Renderer", true, [&]() {
            auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
            LimnGui::ColorEdit("Color", spriteRenderer.Color);
        });

        ComponentInspector<BillboardSpriteRendererComponent>(entity, "Billboard Sprite Renderer", true, [&]() {
            auto& bsrc = entity.GetComponent<BillboardSpriteRendererComponent>();
            LimnGui::ColorEdit("Color", bsrc.Color);
        });

        ComponentInspector<CircleRendererComponent>(entity, "Circle Renderer", true, [&]() {
            auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();

            LimnGui::ColorEdit("Color", circleRenderer.Color);

            {
                LimnGui::InputConfig<float> config;
                config.Min = 0.f;
                config.Max = 1.f;
                config.Speed = 0.001f;
                LimnGui::DragFloat("Thickness", circleRenderer.Thickness, config);
            }

            {
                LimnGui::InputConfig<float> config;
                config.Min = 0.f;
                config.Max = 1.f;
                config.Speed = 0.001f;
                LimnGui::DragFloat("Fade", circleRenderer.Fade, config);
            }
        });

        ComponentInspector<BillboardCircleRendererComponent>(entity, "Billboard Circle Renderer", true, [&]() {
            auto& circleRenderer = entity.GetComponent<BillboardCircleRendererComponent>();

            LimnGui::ColorEdit("Color", circleRenderer.Color);

            {
                LimnGui::InputConfig<float> config;
                config.Min = 0.f;
                config.Max = 1.f;
                config.Speed = 0.001f;
                LimnGui::DragFloat("Thickness", circleRenderer.Thickness, config);
            }

            {
                LimnGui::InputConfig<float> config;
                config.Min = 0.f;
                config.Max = 1.f;
                config.Speed = 0.001f;
                LimnGui::DragFloat("Fade", circleRenderer.Fade, config);
            }
        });

        ComponentInspector<EllipseRendererComponent>(entity, "Ellipse Renderer", true, [&]() {
            auto& circleRenderer = entity.GetComponent<EllipseRendererComponent>();

            LimnGui::ColorEdit("Color", circleRenderer.Color);

            {
                LimnGui::InputConfig<float> config;
                config.Min = 0.f;
                config.Max = 1.f;
                config.Speed = 0.001f;
                LimnGui::DragFloat("Thickness", circleRenderer.Thickness, config);
            }

            {
                LimnGui::InputConfig<float> config;
                config.Min = 0.f;
                config.Max = 1.f;
                config.Speed = 0.001f;
                LimnGui::DragFloat("Fade", circleRenderer.Fade, config);
            }
        });
    }


    // LimnGui /////////////////////////////////

    void LimnGui::HelpMarker(const std::string& description)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(description.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    bool LimnGui::Checkbox(const std::string& label, bool& value, float columnWidth)
    {
        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        bool valueChanged = ImGui::Checkbox("##V", &value);

        ImGui::Columns(1);
        ImGui::PopID();
        return valueChanged;
    }


    bool LimnGui::InputInt(const std::string& label, int& value, const InputConfig<int>& config, float columnWidth = 100.f)
    {
        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        bool valueChanged = ImGui::InputInt("##V", &value, config.Speed, config.FastSpeed, ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Columns(1);
        ImGui::PopID();
        return valueChanged;
    }


    bool LimnGui::InputScientific(const std::string& label, double& value, float columnWidth)
    {
        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        auto [c, e] = ToScientific<double, float, int>(value);
        double step = FromScientific<double, double, int>(1.0, e - 4);
        double stepFast = FromScientific<double, double, int>(1.0, e - 1);
        bool valueChanged = ImGui::InputDouble("##V", &value, step, stepFast, "%.4e", ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsScientific);

        ImGui::Columns(1);
        ImGui::PopID();
        return valueChanged;
    }

    bool LimnGui::DragFloat(const std::string& label, float& value, const InputConfig<float>& config, float columnWidth)
    {
        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        std::ostringstream formatting;
        formatting << "%." << config.Precision << "f";
        bool valueChanged = ImGui::DragFloat("##V", &value, config.Speed, config.Min, config.Max, formatting.str().c_str(), ImGuiSliderFlags_AlwaysClamp);

        ImGui::Columns(1);
        ImGui::PopID();
        return valueChanged;
    }


    bool LimnGui::DragVec3(const std::string& label, Vector3& values, const InputConfig<float>& config, float columnWidth)
    {
        bool valueChanged = false;

        ImGuiIO& io = ImGui::GetIO();
        auto* boldFont = io.Fonts->Fonts[ImGuiLayer::FontIndex::Bold];

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.f;
        ImVec2 ButtonSize = { lineHeight + 3.f, lineHeight };

        std::string formatStr;
        {
            std::ostringstream formatting;
            formatting << "%." << config.Precision << "f";
            formatStr = formatting.str();
        }

        // X
        ImGui::PushStyleColor(ImGuiCol_Button,          ImVec4{ 1.f, 0.2f, 0.3f, 0.7f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,   ImVec4{ 1.f, 0.2f, 0.3f, 1.f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,    ImVec4{ 1.f, 0.2f, 0.3f, 0.4f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("X", ButtonSize)) {
            values.x = config.ResetValue;
            valueChanged = true;
        }
        ImGui::PopFont();
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
        if (ImGui::DragFloat("##X", &values.x, config.Speed, config.Min, config.Max, formatStr.c_str(), ImGuiSliderFlags_AlwaysClamp)) {
            valueChanged = true;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        // Y
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 1.f, 0.3f, 0.7f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 1.f, 0.3f, 1.f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 1.f, 0.3f, 0.7f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Y", ButtonSize)) {
            values.y = config.ResetValue;
            valueChanged = true;
        }
        ImGui::PopFont();
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
        if (ImGui::DragFloat("##Y", &values.y, config.Speed, config.Min, config.Max, formatStr.c_str(), ImGuiSliderFlags_AlwaysClamp)) {
            valueChanged = true;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        // Z
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.3f, 1.f, 0.7f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.3f, 1.f, 1.f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.3f, 1.f, 0.7f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Z", ButtonSize)) {
            values.z = config.ResetValue;
            valueChanged = true;
        }
        ImGui::PopFont();
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
        if (ImGui::DragFloat("##Z", &values.z, config.Speed, config.Min, config.Max, formatStr.c_str(), ImGuiSliderFlags_AlwaysClamp)) {
            valueChanged = true;
        }
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();
        ImGui::Columns(1);
        ImGui::PopID();
        return valueChanged;
    }


    bool LimnGui::InputVec3d(const std::string& label, Vector3d& values, const InputConfig<double>& config, float columnWidth)
    {
        bool valueChanged = false;

        ImGuiIO& io = ImGui::GetIO();
        auto* boldFont = io.Fonts->Fonts[ImGuiLayer::FontIndex::Bold];

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        //ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.f;
        ImVec2 ButtonSize = { lineHeight + 3.f, lineHeight };

        std::string formatStr;
        {
            std::ostringstream formatting;
            formatting << "%." << config.Precision << "f";
            formatStr = formatting.str();
        }

        ImGuiInputTextFlags flags;
        if (config.ReadOnly) flags |= ImGuiInputTextFlags_ReadOnly;

        // X
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 1.f, 0.2f, 0.3f, 0.7f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 1.f, 0.2f, 0.3f, 1.f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 1.f, 0.2f, 0.3f, 0.4f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("X", ButtonSize)) {
            values.x = config.ResetValue;
            valueChanged = true;
        }
        ImGui::PopFont();
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
        if (ImGui::InputDouble("##X", &values.x, config.Speed, config.FastSpeed, formatStr.c_str(), flags)) {
            valueChanged = true;
        }
        //ImGui::PopItemWidth();
        //ImGui::SameLine();

        // Y
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 1.f, 0.3f, 0.7f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 1.f, 0.3f, 1.f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 1.f, 0.3f, 0.7f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Y", ButtonSize)) {
            values.y = config.ResetValue;
            valueChanged = true;
        }
        ImGui::PopFont();
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
        if (ImGui::InputDouble("##Y", &values.y, config.Speed, config.FastSpeed, formatStr.c_str(), flags)) {
            valueChanged = true;
        }
        //ImGui::PopItemWidth();
        //ImGui::SameLine();

        // Z
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.3f, 1.f, 0.7f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.3f, 1.f, 1.f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.3f, 1.f, 0.7f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Z", ButtonSize)) {
            values.z = config.ResetValue;
            valueChanged = true;
        }
        ImGui::PopFont();
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
        if (ImGui::InputDouble("##Z", &values.z, config.Speed, config.FastSpeed, formatStr.c_str(), flags)) {
            valueChanged = true;
        }
        //ImGui::PopItemWidth();

        ImGui::PopStyleVar();
        ImGui::Columns(1);
        ImGui::PopID();
        return valueChanged;
    }


    bool LimnGui::ColorEdit(const std::string& label, Vector4& values, float columnWidth)
    {
        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        bool valueChanged = ImGui::ColorEdit4("##V", values.Ptr(), ImGuiColorEditFlags_AlphaBar);

        ImGui::Columns(1);
        ImGui::PopID();
        return valueChanged;
    }


    bool LimnGui::ColorEdit3(const std::string& label, Vector3& values, float columnWidth)
    {
        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        bool valueChanged = ImGui::ColorEdit3("##V", values.Ptr());

        ImGui::Columns(1);
        ImGui::PopID();
        return valueChanged;
    }

}
