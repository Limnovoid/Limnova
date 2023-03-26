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
        Vector3 LocalScale = { 1.f };
        float Albedo; /* Surface reflectivity of orbital object - determines object's brightness as a star-like object when viewed from far away */

        OrbitalComponent() = default;
        //OrbitalComponent(const OrbitalComponent&) = default;
        //OrbitalComponent(const OrbitalPhysics<entt::entity>::TObjectId& physicsObjectId)
        //    : PhysicsObjectId(physicsObjectId) {}

        Physics::Validity GetValidity() { return Physics->GetValidity(PhysicsObjectId); }

        void SetLocalSpaceRadius(float radius)
        {
            float oldRadius = Physics->GetLocalSpaceRadius(PhysicsObjectId);
            if (Physics->SetLocalSpaceRadius(PhysicsObjectId, radius))
            {
                LocalScale *= oldRadius / radius;
            }
        }
        float GetLocalSpaceRadius() { return Physics->GetLocalSpaceRadius(PhysicsObjectId); }

        void SetMass(double mass) { Physics->SetMass(PhysicsObjectId, mass); }
        void SetPosition(const Vector3& position) { Physics->SetPosition(PhysicsObjectId, position); }
        void SetVelocity(const Vector3& velocity) { Physics->SetVelocity(PhysicsObjectId, velocity); }

        double GetMass() { return Physics->GetMass(PhysicsObjectId); }
        Vector3 GetPosition() { return Physics->GetPosition(PhysicsObjectId); }
        Vector3d GetVelocity() { return Physics->GetVelocity(PhysicsObjectId); }
    };


    class OrbitalScene : public Scene
    {
    public:
        OrbitalScene();
        ~OrbitalScene() = default;

        void SetParent(Entity entity, Entity parent);

        void SetViewPrimary(Entity primary);
        Entity GetViewPrimary();
        std::vector<Entity> GetSecondaries(Entity entity);

        void OnUpdate(Timestep dT) override;
        void OnRender() override;
    private:
        void OnOrbitalComponentConstruct(entt::registry&, entt::entity);
        void OnOrbitalComponentDestruct(entt::registry&, entt::entity);
    public:
        Vector4 m_InfluenceColor = { 1.f, 1.f, 1.f, 0.3f };
        float m_InfluenceThickness = 0.002f;
        float m_InfluenceFade = 0.006f;
    private:
        Physics m_Physics;
        entt::entity m_ViewPrimary;
    };

}
