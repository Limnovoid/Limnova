#pragma once

#include <Limnova.h>

namespace Limnova
{

    class SceneHierarchyPanel
    {
    public:
        SceneHierarchyPanel() = default;
        SceneHierarchyPanel(Scene* scene);

        void SetContext(Scene* scene);

        void OnImGuiRender();
    private:
        void EntityNode(Entity entity, bool forceExpanded = false);
        void Inspector(Entity entity);

        template<typename TComponent>
        void ComponentInspector(Entity entity, const std::string& name, std::function<void()> control)
        {
            if (entity.HasComponent<TComponent>())
            {
                if (ImGui::TreeNodeEx((void*)typeid(TComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, name.c_str()))
                {
                    control();

                    ImGui::TreePop();
                }
                ImGui::Separator();
            }
        }
    private:
        Scene* m_Scene;
        Entity m_Selected;
    };

    namespace LimnGui
    {

        bool DragVec3(const std::string& label, Vector3& values, float speed, float resetValue = 0.f, float columnWidth = 100.f);

    }

}
