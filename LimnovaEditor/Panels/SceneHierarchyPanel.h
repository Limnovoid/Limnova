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
    private:
        Scene* m_Scene;
        Entity m_Selected;
    };

}
