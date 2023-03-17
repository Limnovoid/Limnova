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

        EntityNode(m_Scene->GetRoot());

        ImGui::End();
    }


    void SceneHierarchyPanel::EntityNode(Entity entity)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (entity == m_Selected) flags |= ImGuiTreeNodeFlags_Selected;

        auto children = m_Scene->GetChildren(entity);
        if (children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;

        auto& tag = entity.GetComponent<TagComponent>();
        bool expanded = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.Tag.c_str());

        if (ImGui::IsItemClicked()) m_Selected = entity;

        if (expanded)
        {
            for (auto child : children)
            {
                EntityNode(child);
            }
            ImGui::TreePop();
        }
    }

}
