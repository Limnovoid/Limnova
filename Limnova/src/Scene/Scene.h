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
        ~Scene();

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

        void OnUpdate(Timestep dT);

        void OnWindowResize(uint32_t width, uint32_t height);
    private:
        entt::registry m_Registry;
        entt::entity m_ActiveCamera = entt::null;

        friend class Entity;
    private:
        void OnCameraComponentConstruction(entt::registry&, entt::entity);
        void OnCameraComponentDestruction(entt::registry&, entt::entity);
    };

}
