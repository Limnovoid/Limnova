#pragma once

#include <entt.hpp>
#include <Core/Timestep.h>
#include <Events/Event.h>
#include <Events/ApplicationEvent.h>


namespace Limnova
{

    class Entity;

    class Scene
    {
    public:
        Scene();
        ~Scene() = default;

        Entity CreateEntity(const std::string& name = std::string());

        template<typename First, typename... Rest>
        std::vector<Entity> GetEntitiesByComponents()
        {
            std::vector<Entity> entities;
            auto view = m_Registry.view<First, Rest...>();
            for (auto entity : view)
            {
                entities.emplace_back(entity, this);
            }
            return entities;
        }

        void SetActiveCamera(Entity cameraEntity);
        Entity GetActiveCamera();
        void OnWindowChangeAspect(float aspect);

        Entity GetRoot();
        void SetParent(Entity entity, Entity newParent);
        Entity GetParent(Entity entity);
        std::vector<Entity> GetChildren(Entity entity);

        virtual void OnUpdate(Timestep dT);
        virtual void OnRender();
        virtual void OnEvent(Event& e);
    protected:
        entt::registry m_Registry;
        entt::entity m_ActiveCamera;

        entt::entity m_Root; // Scene hierarchy

        friend class Entity;
        friend class SceneHierarchyPanel;
    private:
        /* Events and signals */
        void OnHierarchyComponentConstruction(entt::registry&, entt::entity);
        void OnHierarchyComponentDestruction(entt::registry&, entt::entity);
        void OnCameraComponentConstruction(entt::registry&, entt::entity);
        void OnCameraComponentDestruction(entt::registry&, entt::entity);

        bool OnWindowResize(WindowResizeEvent& e);

        /* Helpers */
        void HierarchyConnect(Entity entity, Entity parent);
        void HierarchyDisconnect(Entity entity);
    };

}
