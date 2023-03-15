#pragma once

#include "Scene.h"

#include <entt.hpp>


namespace Limnova
{

    class Entity
    {
    public:
        Entity() = default;
        Entity(const Entity& other) = default;
        Entity(entt::entity id, Scene* scene)
            : m_EnttId(id), m_Scene(scene) {}

        template<typename T>
        bool HasComponent()
        {
            return m_Scene->m_Registry.all_of<T>(m_EnttId);
        }

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            LV_CORE_ASSERT(!m_Scene->m_Registry.all_of<T>(m_EnttId), "Entity already has component!");
            return m_Scene->m_Registry.emplace<T>(m_EnttId, std::forward<Args>(args)...);
        }

        template<typename T>
        T& GetComponent()
        {
            LV_CORE_ASSERT(m_Scene->m_Registry.all_of<T>(m_EnttId), "Entity does not have component!");
            return m_Scene->m_Registry.get<T>(m_EnttId);
        }

        template<typename First, typename... Rest>
        std::tuple<First&, Rest&...> GetComponents()
        {
            LV_CORE_ASSERT(m_Scene->m_Registry.all_of<First>(m_EnttId), "Entity does not have component!");
            LV_CORE_ASSERT(m_Scene->m_Registry.all_of<Rest...>(m_EnttId), "Entity does not have component(s)!");
            return m_Scene->m_Registry.get<First, Rest...>(m_EnttId);
        }

        template<typename T>
        void RemoveComponent()
        {
            LV_CORE_ASSERT(m_Scene->m_Registry.all_of<T>(m_EnttId), "Entity does not have component!");
            m_Scene->m_Registry.erase<T>(m_EnttId);
        }

        void Destroy();
    public:
        operator bool() const { return m_Scene->m_Registry.valid(m_EnttId); }

        bool operator==(const Entity& rhs) { return m_EnttId == rhs.m_EnttId; }
        bool operator!=(const Entity& rhs) { return m_EnttId != rhs.m_EnttId; }
    private:
        entt::entity m_EnttId = entt::null;
        Scene* m_Scene = nullptr;

        friend class Scene;
    };

}
