#pragma once

#include "OrbitalPhysics.h"

#include "Limnova.h"


namespace Limnova
{

    using Physics = OrbitalPhysics<entt::entity>;


    struct OrbitalComponent
    {
        friend class OrbitalScene;
    private:
        Physics::TObjectId PhysicsObjectId = Physics::Null;
        Physics* Physics = nullptr;
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

        void SetParent(Entity entity, Entity parent);

        void OnUpdate(Timestep dT) override;
    private:
        void OnOrbitalComponentConstruct(entt::registry&, entt::entity);
        void OnOrbitalComponentDestruct(entt::registry&, entt::entity);
    private:
        Physics m_Physics;
    };

}
