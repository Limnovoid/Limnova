#pragma once

#include "OrbitalPhysics.h"

#include <Scene/Scene.h>


namespace Limnova
{

    using Physics = OrbitalPhysics<UUID>;


    class OrbitalScene : public Scene
    {
    public:
        OrbitalScene();
        ~OrbitalScene() = default;

        static Ref<OrbitalScene> Copy(Ref<OrbitalScene> scene);

        void SetRootScaling(double meters);
        double GetRootScaling();

        void SetViewPrimary(Entity primary);
        Entity GetViewPrimary();

        void SetParent(Entity entity, Entity parent);
        std::vector<Entity> GetSecondaries(Entity entity);

        void OnUpdateRuntime(Timestep dT) override;
        void OnUpdateEditor(Timestep dT) override;
        void OnRenderRuntime() override;
        void OnRenderEditor(EditorCamera& camera) override;
    private:
        void UpdateOrbitalScene();
        void RenderOrbitalScene(Camera& camera, const Quaternion& cameraOrientation, float cameraDistance);

        void OnOrbitalComponentConstruct(entt::registry&, entt::entity);
        void OnOrbitalComponentDestruct(entt::registry&, entt::entity);
    public:
        Vector4 m_LocalSpaceColor = { 1.f, 1.f, 1.f, 0.3f };
        float m_LocalSpaceThickness = 0.002f;
        float m_LocalSpaceFade = 0.006f;

        float m_OrbitThickness = 0.006f;
        float m_OrbitFade = 0.f;
        float m_OrbitAlpha = 0.4f;
        float m_OrbitPointRadius = 0.01f;

        bool m_ShowReferenceAxes = false;
        Vector4 m_ReferenceAxisColor = { 1.f, 1.f, 1.f, 0.2 };
        float m_ReferenceAxisLength = 0.4f;
        float m_ReferenceAxisThickness = 0.006f;
        float m_ReferenceAxisArrowSize = 0.024f;

        float m_PerifocalAxisThickness = 0.006f;
        float m_PerifocalAxisArrowSize = 0.024f;
    private:
        Physics m_Physics;
        UUID m_ViewPrimary;

        Quaternion m_OrbitalReferenceFrameOrientation; /* Orientation of the orbital physics system's reference frame relative to the scene frame */
        Vector3 m_OrbitalReferenceX, m_OrbitalReferenceY, m_OrbitalReferenceNormal;

        friend class SceneSerializer;
    };

}
