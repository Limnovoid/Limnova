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
        void GetEntitiesByComponents(std::vector<Entity>& entities)
        {
            auto view = m_Registry.view<First, Rest...>();
            for (auto entity : view)
            {
                entities.emplace_back(entity, this);
            }
        }

        void SetActiveCamera(Entity cameraEntity);
        Entity GetActiveCamera();

        void Reparent(Entity entity, Entity newParent);

        virtual void OnUpdate(Timestep dT);
        virtual void OnEvent(Event& e);
    protected:
        entt::registry m_Registry;
        entt::entity m_ActiveCamera;

        entt::entity m_Root; // Scene hierarchy

        friend class Entity;
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