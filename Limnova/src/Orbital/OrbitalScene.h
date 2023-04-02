#pragma once

#include "OrbitalPhysics.h"

#include <Scene/Scene.h>


namespace Limnova
{

    using Physics = OrbitalPhysics<entt::entity>;


    class OrbitalScene : public Scene
    {
    public:
        OrbitalScene();
        ~OrbitalScene() = default;

        void SetParent(Entity entity, Entity parent);

        void SetRootScaling(double meters);
        double GetRootScaling();

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
        float m_OrbitThickness = 0.004f;
        float m_OrbitThicknessFactor = 2.f;
        float m_OrbitAlpha = 0.4f;
    private:
        Physics m_Physics;
        entt::entity m_ViewPrimary;

        friend class OrbitalSceneSerializer;
    };

}