#pragma once

#include "OrbitalPhysics.h"

#include "Limnova.h"


namespace Limnova
{

    using Physics = OrbitalPhysics<entt::entity>;


    class OrbitalComponent
    {
        friend class OrbitalScene;
    private:
        Physics::TObjectId PhysicsObjectId = Physics::Null;
    public:
        OrbitalComponent() = default;
        //OrbitalComponent(const OrbitalComponent&) = default;
        //OrbitalComponent(const OrbitalPhysics<entt::entity>::TObjectId& physicsObjectId)
        //    : PhysicsObjectId(physicsObjectId) {}
    };


    class OrbitalScene : public Scene
    {
    public:
        OrbitalScene();
        ~OrbitalScene() = default;

    private:
        void OnOrbitalComponentConstruct(entt::registry&, entt::entity);
        void OnOrbitalComponentDestruct(entt::registry&, entt::entity);
    private:
        Physics m_Physics;
    };

}
