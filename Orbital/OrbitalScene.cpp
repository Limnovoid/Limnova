#include "OrbitalScene.h"


namespace Limnova
{

    OrbitalScene::OrbitalScene()
    {
        m_Registry.on_construct<OrbitalComponent>().connect<&OrbitalScene::OnOrbitalComponentConstruct>(this);
        m_Registry.on_destroy<OrbitalComponent>().connect<&OrbitalScene::OnOrbitalComponentDestruct>(this);
    }


    void OrbitalScene::SetParent(Entity entity, Entity parent)
    {
        if (entity.HasComponent<OrbitalComponent>())
        {
            if (!parent.HasComponent<OrbitalComponent>()) return; /* Cannot set orbital entity to a non-orbital entity */

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
        auto& orbital = m_Registry.get<OrbitalComponent>(entity);
        orbital.PhysicsObjectId = m_Physics.Create(entity);
        orbital.Physics = &m_Physics;
    }


    void OrbitalScene::OnOrbitalComponentDestruct(entt::registry&, entt::entity entity)
    {
        m_Physics.Destroy(m_Registry.get<OrbitalComponent>(entity).PhysicsObjectId);
    }

}
