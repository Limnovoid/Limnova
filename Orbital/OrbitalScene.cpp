#include "OrbitalScene.h"


namespace Limnova
{

    OrbitalScene::OrbitalScene()
    {
        m_Registry.on_construct<OrbitalComponent>().connect<&OrbitalScene::OnOrbitalComponentConstruct>(this);
        m_Registry.on_destroy<OrbitalComponent>().connect<&OrbitalScene::OnOrbitalComponentDestruct>(this);
    }


    void OrbitalScene::OnOrbitalComponentConstruct(entt::registry&, entt::entity entity)
    {
        m_Registry.get<OrbitalComponent>(entity).PhysicsObjectId = m_Physics.Create(entity);
    }


    void OrbitalScene::OnOrbitalComponentDestruct(entt::registry&, entt::entity entity)
    {
        m_Physics.Destroy(m_Registry.get<OrbitalComponent>(entity).PhysicsObjectId);
    }

}
