#include "OrbitalScene.h"

#include <Scene/Entity.h>
#include <Scene/Components.h>


namespace Limnova
{

    OrbitalScene::OrbitalScene()
        : Scene()
    {
        // Orbital setup
        /* NOTE : root MUST be assigned before signal setup - the root's OrbitalComponent should NOT create
         * a new object in OrbitalPhysics */
        auto& rootOrbital = m_Registry.emplace<OrbitalComponent>(m_Root);
        rootOrbital.PhysicsObjectId = m_Physics.AssignRoot(m_Root);
        rootOrbital.Physics = &m_Physics;

        // Signals
        m_Registry.on_construct<OrbitalComponent>().connect<&OrbitalScene::OnOrbitalComponentConstruct>(this);
        m_Registry.on_destroy<OrbitalComponent>().connect<&OrbitalScene::OnOrbitalComponentDestruct>(this);
    }


    void OrbitalScene::SetParent(Entity entity, Entity parent)
    {
        if (entity.HasComponent<OrbitalComponent>())
        {
            if (!parent.HasComponent<OrbitalComponent>()) return; /* Cannot parent orbital entity to a non-orbital entity */

            // Update physics system to reflect new parentage
            auto& orbital = entity.GetComponent<OrbitalComponent>();
            auto& porbital = parent.GetComponent<OrbitalComponent>();
            m_Physics.SetParent(orbital.PhysicsObjectId, porbital.PhysicsObjectId);
        }
        Scene::SetParent(entity, parent);
    }


    void OrbitalScene::OnUpdate(Timestep dT)
    {
        Scene::OnUpdate(dT);
    }


    void OrbitalScene::OnOrbitalComponentConstruct(entt::registry&, entt::entity entity)
    {
        auto[orbital, hierarchy] = m_Registry.get<OrbitalComponent, HierarchyComponent>(entity);
        if (hierarchy.Parent.HasComponent<OrbitalComponent>())
        {
            orbital.PhysicsObjectId = m_Physics.Create(entity, hierarchy.Parent.GetComponent<OrbitalComponent>().PhysicsObjectId);
        }
        else
        {
            SetParent(Entity{ entity, this }, Entity{ m_Root, this });
            orbital.PhysicsObjectId = m_Physics.Create(entity);
        }
        orbital.Physics = &m_Physics;
    }


    void OrbitalScene::OnOrbitalComponentDestruct(entt::registry&, entt::entity entity)
    {
        m_Physics.Destroy(m_Registry.get<OrbitalComponent>(entity).PhysicsObjectId);
    }

}
