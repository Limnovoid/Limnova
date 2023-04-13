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

        void OnUpdateRuntime(Timestep dT) override;
        void OnUpdateEditor(Timestep dT) override;
        void OnRenderRuntime() override;
        void OnRenderEditor(EditorCamera& camera) override;
    private:
        void UpdateOrbitalScene();
        void RenderOrbitalScene(Camera& camera, const Quaternion& cameraOrientation);

        void OnOrbitalComponentConstruct(entt::registry&, entt::entity);
        void OnOrbitalComponentDestruct(entt::registry&, entt::entity);
    public:
        Vector4 m_InfluenceColor = { 1.f, 1.f, 1.f, 0.3f };
        float m_InfluenceThickness = 0.002f;
        float m_InfluenceFade = 0.006f;

        float m_OrbitThickness = 0.004f;
        float m_OrbitThicknessFactor = 2.f;
        float m_OrbitAlpha = 0.4f;
        float m_OrbitPointRadius = 0.01f;

        bool m_ShowReferenceAxes = false;
        Vector4 m_ReferenceAxisColor = { 1.f, 1.f, 1.f, 0.4 };
        float m_ReferenceAxisLength = 1.f;
        float m_ReferenceAxisThickness = 0.01f;
    private:
        Physics m_Physics;
        entt::entity m_ViewPrimary;

        Quaternion m_OrbitalReferenceFrameOrientation; /* Orientation of the orbital physics system's reference frame relative to the scene frame */
        Vector3 m_OrbitalReferenceX, m_OrbitalReferenceY, m_OrbitalReferenceNormal;

        friend class OrbitalSceneSerializer;
    };

}
