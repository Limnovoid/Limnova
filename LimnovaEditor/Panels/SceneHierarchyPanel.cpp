#include "SceneHierarchyPanel.h"


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

        if (entity.HasComponent<TransformComponent>())
        {
            auto& transform = entity.GetComponent<TransformComponent>();

            if (ImGui::TreeNodeEx((void*)typeid(TransformComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Transform"))
            {
                // TODO - drag sensitivity proportional to zoom
                if (ImGui::DragFloat3("Position", transform.Position.Ptr(), 0.1f))
                {
                    transform.NeedCompute = true;

                    // TODO - forward to OrbitalPhysics !!!!
                }
                if (ImGui::DragFloat3("Scale", transform.Scale.Ptr(), 0.1f))
                {
                    transform.NeedCompute = true;
                }
                ImGui::TreePop();
            }

            ImGui::Separator();
        }

#ifdef LV_EDITOR_USE_ORBITAL
        if (entity.HasComponent<OrbitalComponent>())
        {
            auto& orbital = entity.GetComponent<OrbitalComponent>();

            switch (orbital.GetValidity())
            {
            case Physics::Validity::Valid:          ImGui::Text(                                "Validity: Valid"); break;
            case Physics::Validity::InvalidParent:  ImGui::TextColored({1.f, 0.f, 0.f, 0.8f},   "Validity: Invalid Parent!"); break;
            case Physics::Validity::InvalidMass:    ImGui::TextColored({1.f, 0.f, 0.f, 0.8f},   "Validity: Invalid Mass!"); break;
            case Physics::Validity::InvalidPosition:ImGui::TextColored({1.f, 0.f, 0.f, 0.8f},   "Validity: Invalid Position!"); break;
            }

            if (ImGui::TreeNodeEx((void*)typeid(OrbitalComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Orbital"))
            {
                // TODO - scientific format (e.g, 3.14e15)
                double mass = orbital.GetMass();
                if (ImGui::InputDouble("Mass", &mass, 1e5, 1e10))
                {
                    orbital.SetMass(mass);
                }

                Vector3 position = orbital.GetPosition();
                if (ImGui::DragFloat3("Position", position.Ptr(), 0.01f))
                {
                    orbital.SetPosition(position);
                }

                Vector3 velocity = orbital.GetVelocity();
                if (ImGui::DragFloat3("Velocity", velocity.Ptr(), 0.01f))
                {
                    orbital.SetVelocity(velocity);
                }

                ImGui::TreePop();
            }

            ImGui::Separator();
        }
#endif
    }

}
