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

        virtual void OnUpdate(Timestep dT);
        virtual void OnEvent(Event& e);
    protected:
        entt::registry m_Registry;
        entt::entity m_ActiveCamera;

        friend class Entity;
    private:
        void OnHierarchyComponentDestruction(entt::registry&, entt::entity);
        void OnCameraComponentConstruction(entt::registry&, entt::entity);
        void OnCameraComponentDestruction(entt::registry&, entt::entity);

        bool OnWindowResize(WindowResizeEvent& e);
    };

}
