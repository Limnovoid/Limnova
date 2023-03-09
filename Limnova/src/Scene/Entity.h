#pragma once

#include "Scene.h"

#include <entt.hpp>


namespace Limnova
{

    class Entity
    {
    public:
        Entity() = default;
        Entity(entt::entity id, Scene* scene);
        Entity(const Entity& other) = default;

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

        template<typename T>
        void RemoveComponent()
        {
            LV_CORE_ASSERT(m_Scene->m_Registry.all_of<T>(m_EnttId), "Entity does not have component!");
            m_Scene->m_Registry.erase<T>(m_EnttId);
        }
    public:
        operator bool() const { return m_Scene->m_Registry.valid(m_EnttId); }
    private:
        entt::entity m_EnttId{ entt::null };
        Scene* m_Scene;

        friend class Scene;
    };

}
