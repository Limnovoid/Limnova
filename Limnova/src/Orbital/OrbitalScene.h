#pragma once

#include "OrbitalPhysics.h"

#include <Scene/Scene.h>


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

        Physics::Validity GetValidity() { return Physics->GetValidity(PhysicsObjectId); }

        void SetMass(double mass) { Physics->SetMass(PhysicsObjectId, mass); }
        void SetPosition(Vector3 position) { Physics->SetPosition(PhysicsObjectId, position); }
        void SetVelocity(Vector3 velocity) { Physics->SetVelocity(PhysicsObjectId, velocity); }

        double GetMass() { return Physics->GetMass(PhysicsObjectId); }
        Vector3 GetPosition() { return Physics->GetPosition(PhysicsObjectId); }
        Vector3 GetVelocity() { return Physics->GetVelocity(PhysicsObjectId); }
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
