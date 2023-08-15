#pragma once

#include "OrbitalPhysics.h"

#include <Scene/Scene.h>


namespace Limnova
{
    class OrbitalScene : public Scene
    {
    public:
        OrbitalScene();
        OrbitalScene(Scene const& baseScene);
        ~OrbitalScene() = default;

        static Ref<OrbitalScene> Copy(Ref<OrbitalScene> scene);

        Entity CreateEntityFromUUID(UUID uuid, const std::string& name = std::string(), UUID parent = UUID::Null) override;
        Entity DuplicateEntity(Entity entity) override;
    public:
        void PhysicsUseContext();

        void SetRootScaling(double meters);
        double GetRootScaling();

        void SetTrackingEntity(Entity primary);
        void SetRelativeViewSpace(int viewSpaceRelativeToTrackingEntity = 0);
        Entity GetViewPrimary();

        void SetParent(Entity entity, Entity parent) override;
        void SetParentAndLocalSpace(Entity entity, Entity parent, int localSpaceRelativeToParent);
        void SetLocalSpace(Entity entity, int localSpaceRelativeToParent);
        OrbitalPhysics::LSpaceNode GetLocalSpace(Entity entity);
        std::vector<Entity> GetSecondaries(Entity entity);
        std::vector<Entity> GetSatellites(Entity primary);

        void OnStartRuntime() override;
        void OnUpdateRuntime(Timestep dT) override;
        void OnUpdateEditor(Timestep dT) override;
        void OnRenderRuntime() override;
        void OnRenderEditor(EditorCamera& camera) override;
        void OnStopRuntime() override;

#ifdef LV_DEBUG
        //OrbitalPhysics::Stats const& GetPhysicsStats() { return m_Physics.GetStats(); }
#endif
    private:
        void UpdateOrbitalScene();
        void RenderOrbitalScene(Camera& camera, const Quaternion& cameraOrientation, float cameraDistance);

        OrbitalPhysics::LSpaceNode GetEntityLSpace(entt::entity entity);

        void OnOrbitalComponentConstruct(entt::registry&, entt::entity);
        void OnOrbitalComponentUpdate(entt::registry&, entt::entity);
        void OnOrbitalComponentDestruct(entt::registry&, entt::entity);
    public:
        Vector4 m_InfluencingSpaceColor = { 1.f, 0.9f, 0.3f, 0.3f };
        Vector4 m_LocalSpaceColor = { 1.f, 1.f, 1.f, 0.3f };
        float m_LocalSpaceThickness = 0.003f;
        float m_LocalSpaceFade = 0.f;
        bool m_ShowViewSpace = true;

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
        OrbitalPhysics::Context m_Physics;
        std::map<OrbitalPhysics::TNodeId, entt::entity> m_PhysicsToEnttIds;

        UUID m_TrackingEntity;
        int m_ViewSpaceRelativeToTrackedEntity;
        OrbitalPhysics::LSpaceNode m_ViewLSpace;

        Quaternion m_OrbitalReferenceFrameOrientation; /* Orientation of the orbital physics system's reference frame relative to the scene frame */
        Vector3 m_OrbitalReferenceX, m_OrbitalReferenceY, m_OrbitalReferenceNormal;

        friend class SceneHierarchyPanel;
        friend class SceneSerializer;
    };

}
