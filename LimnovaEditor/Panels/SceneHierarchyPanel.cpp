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
    }


    void SceneHierarchyPanel::OnImGuiRender()
    {
        ImGui::Begin("Scene Hierarchy");

        EntityNode(m_Scene->GetRoot(), true);

        if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
            m_Selected = Entity::Null;
        }

        ImGui::End(); // Scene Hierarchy

        ImGui::Begin("Inspector");

        if (m_Selected) {
            Inspector(m_Selected);
        }

        ImGui::End(); // Inspector
    }


    void SceneHierarchyPanel::EntityNode(Entity entity, bool forceExpanded)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (forceExpanded) flags |= ImGuiTreeNodeFlags_DefaultOpen;
        if (entity == m_Selected) flags |= ImGuiTreeNodeFlags_Selected;

        auto& tag = entity.GetComponent<TagComponent>();

        auto children = m_Scene->GetChildren(entity);
        if (children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;

        bool expanded = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.Tag.c_str());
        if (ImGui::IsItemClicked()) {
            m_Selected = entity;
        }

        if (entity.HasComponent<OrbitalComponent>()) {
            ImGui::SameLine();
            if (ImGui::Button("View")) {
                ((OrbitalScene*)m_Scene)->SetViewPrimary(entity);
            }
        }

        if (expanded)
        {
            for (auto child : children)
            {
                EntityNode(child);
            }
            ImGui::TreePop();
        }
    }


    void SceneHierarchyPanel::Inspector(Entity entity)
    {
        // Tag
        if (entity.HasComponent<TagComponent>())
        {
            auto& tag = entity.GetComponent<TagComponent>();

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy_s(buffer, sizeof(buffer), tag.Tag.c_str());
            if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
            {
                tag = std::string(buffer);
            }

            ImGui::Separator();
        }

        bool isOrbital = entity.HasComponent<OrbitalComponent>();
        bool isOrbitalViewPrimary = isOrbital ? entity == ((OrbitalScene*)m_Scene)->GetViewPrimary() : false;
        bool isOrbitalViewSecondary = isOrbital ? m_Scene->GetParent(entity) == ((OrbitalScene*)m_Scene)->GetViewPrimary() : false;

        // Transform
        ComponentInspector<TransformComponent>(entity, "Transform", [&]()
        {
            auto& transform = entity.GetComponent<TransformComponent>();

            // Position
            ImGui::BeginDisabled(isOrbital && !isOrbitalViewSecondary);
            if (LimnGui::DragVec3("Position", transform.Position, 0.01f, 0.f))
            {
                transform.NeedCompute = true;

                if (entity.HasComponent<OrbitalComponent>()) {
                    entity.GetComponent<OrbitalComponent>().SetPosition(transform.Position);
                }
            }
            ImGui::EndDisabled();

            // Rotation
            ImGui::BeginDisabled(isOrbital && !(isOrbitalViewPrimary || isOrbitalViewSecondary));
            Vector3 eulerAngles = DegreesVec3(transform.GetEulerAngles());
            if (LimnGui::DragVec3("Rotation", eulerAngles, 1.f, 0.f))
            {
                transform.SetEulerAngles(RadiansVec3(eulerAngles));
            }
            ImGui::EndDisabled();

            // Scale
            ImGui::BeginDisabled(isOrbital && !isOrbitalViewPrimary);
            if (LimnGui::DragVec3("Scale", transform.Scale, 0.01f, 1.f))
            {
                transform.NeedCompute = true;

                if (entity.HasComponent<OrbitalComponent>()) {
                    entity.GetComponent<OrbitalComponent>().LocalScale = transform.Scale;
                }
            }
            ImGui::EndDisabled();
        });

        ComponentInspector<CameraComponent>(entity, "Camera", [&]()
        {
            auto& camera = entity.GetComponent<CameraComponent>();

            ImGui::BeginDisabled(m_Scene->GetActiveCamera() == entity);
            if (ImGui::Button("Set Active"))
            {
                m_Scene->SetActiveCamera(entity);
            }
            ImGui::EndDisabled();

            bool isOrthographic = camera.GetOrthographic();
            if (ImGui::Checkbox("Orthographic", &isOrthographic)) {
                camera.SetOrthographic(isOrthographic);
            }

            if (isOrthographic)
            {
                // Orthographic options
                float height = camera.GetOrthographicHeight();
                if (ImGui::DragFloat("Height", &height, 0.01f, 0.001f)) {
                    camera.SetOrthographicHeight(height);
                }

                auto [orthoNear, orthoFar] = camera.GetOrthographicClip();
                if (ImGui::DragFloat("Near", &orthoNear, 0.01f, 0.f, orthoFar)) {
                    camera.SetOrthographicClip(orthoNear, orthoFar);
                }
                if (ImGui::DragFloat("Far", &orthoFar, 0.01f, orthoNear)) {
                    camera.SetOrthographicClip(orthoNear, orthoFar);
                }
            }
            else
            {
                // Perspective options
                float fov = Degreesf(camera.GetPerspectiveFov());
                if (ImGui::DragFloat("FOV", &fov, 1.f, 1.f, 179.f, "%.1f")) {
                    camera.SetPerspectiveFov(Radiansf(fov));
                }

                auto [perspNear, perspFar] = camera.GetPerspectiveClip();
                if (ImGui::DragFloat("Near", &perspNear, 0.01f, 0.001f, perspFar)) {
                    camera.SetPerspectiveClip(perspNear, perspFar);
                }
                if (ImGui::DragFloat("Far", &perspFar, 1.f, perspNear)) {
                    camera.SetPerspectiveClip(perspNear, perspFar);
                }
            }
        });

        // Orbital
        ComponentInspector<OrbitalComponent>(entity, "Orbital", [&]()
        {
            auto& orbital = entity.GetComponent<OrbitalComponent>();

            switch (orbital.GetValidity())
            {
            case Physics::Validity::Valid:          ImGui::Text(                                "Validity: Valid");             break;
            case Physics::Validity::InvalidParent:  ImGui::TextColored({1.f, 0.f, 0.f, 0.8f},   "Validity: Invalid Parent!");   break;
            case Physics::Validity::InvalidMass:    ImGui::TextColored({1.f, 0.f, 0.f, 0.8f},   "Validity: Invalid Mass!");     break;
            case Physics::Validity::InvalidPosition:ImGui::TextColored({1.f, 0.f, 0.f, 0.8f},   "Validity: Invalid Position!"); break;
            }

            ImGui::BeginDisabled(isOrbital && !(isOrbitalViewPrimary || isOrbitalViewSecondary));

            // Local scale
            ImGui::DragFloat3("Local Scale", orbital.LocalScale.Ptr(), 0.1f);

            // Local space radius
            float localSpaceRadius = orbital.GetLocalSpaceRadius();
            if (ImGui::DragFloat("Local Space Radius", &localSpaceRadius, 0.01f)) {
                orbital.SetLocalSpaceRadius(localSpaceRadius);
            }

            // Mass
            double mass = orbital.GetMass();
            auto [c, e] = ToScientific<double, float, int>(mass);
            double step = FromScientific<double, double, int>(1.0, e - 4);
            double stepFast = FromScientific<double, double, int>(1.0, e);
            if (ImGui::InputDouble("Mass", &mass, step, stepFast, "%.4e", ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsScientific))
            {
                orbital.SetMass(mass);
            }

            ImGui::EndDisabled();

            ImGui::BeginDisabled(isOrbital && !isOrbitalViewSecondary);

            // Position
            Vector3 position = orbital.GetPosition();
            if (ImGui::DragFloat3("Position", position.Ptr(), 0.01f))
            {
                orbital.SetPosition(position);
            }

            // Velocity
            Vector3 velocity = orbital.GetVelocity();
            if (ImGui::DragFloat3("Velocity", velocity.Ptr(), 0.01f))
            {
                orbital.SetVelocity(velocity);
            }

            ImGui::EndDisabled(); // (isOrbital && !isOrbitalViewSecondary)
        });

        // Sprite Renderer
        ComponentInspector<SpriteRendererComponent>(entity, "Sprite Renderer", [&]() {
            auto& spriterenderer = entity.GetComponent<SpriteRendererComponent>();
            ImGui::ColorEdit4("Color", spriterenderer.Color.Ptr());
        });
    }


    bool LimnGui::DragVec3(const std::string& label, Vector3& values, float speed, float resetValue, float columnWidth)
    {
        bool valueChanged = false;

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.f;
        ImVec2 ButtonSize = { lineHeight + 3.f, lineHeight };

        ImGui::PushStyleColor(ImGuiCol_Button,          ImVec4{ 1.f, 0.2f, 0.3f, 0.7f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,   ImVec4{ 1.f, 0.2f, 0.3f, 1.f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,    ImVec4{ 1.f, 0.2f, 0.3f, 0.7f });
        if (ImGui::Button("X", ButtonSize)) {
            values.x = resetValue;
            valueChanged = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        if (ImGui::DragFloat("##X", &values.x, speed)) { valueChanged = true; }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 1.f, 0.3f, 0.7f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 1.f, 0.3f, 1.f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 1.f, 0.3f, 0.7f });
        if (ImGui::Button("Y", ButtonSize)) {
            values.y = resetValue;
            valueChanged = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        if (ImGui::DragFloat("##Y", &values.y, speed)) { valueChanged = true; }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.3f, 1.f, 0.7f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.3f, 1.f, 1.f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.3f, 1.f, 0.7f });
        if (ImGui::Button("Z", ButtonSize)) {
            values.z = resetValue;
            valueChanged = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        if (ImGui::DragFloat("##Z", &values.z, speed)) { valueChanged = true; }
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();

        return valueChanged;
    }

}
