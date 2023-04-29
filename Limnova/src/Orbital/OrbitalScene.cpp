#include "OrbitalScene.h"

#include <Scene/Entity.h>
#include <Scene/Components.h>
#include <Renderer/Renderer2D.h>


namespace Limnova
{

    OrbitalScene::OrbitalScene()
        : Scene()
    {
        // Orbital setup
        /* NOTE : root MUST be assigned before signal setup - the root's OrbitalComponent should NOT create
         * a new object in OrbitalPhysics */
        auto& rootOrbital = m_Registry.emplace<OrbitalComponent>(m_Root);
        rootOrbital.PhysicsObjectId = m_Physics.AssignRoot(m_Root);
        rootOrbital.Physics = &m_Physics;

        m_OrbitalReferenceFrameOrientation = Quaternion(Vector3::Left(), PIover2f);
        m_OrbitalReferenceX = m_OrbitalReferenceFrameOrientation.RotateVector(Vector3::X());
        m_OrbitalReferenceY = m_OrbitalReferenceFrameOrientation.RotateVector(Vector3::Y());
        m_OrbitalReferenceNormal = m_OrbitalReferenceFrameOrientation.RotateVector(Vector3::Z());

        m_ViewPrimary = m_Root;

        // Signals
        m_Registry.on_construct<OrbitalComponent>().connect<&OrbitalScene::OnOrbitalComponentConstruct>(this);
        m_Registry.on_destroy<OrbitalComponent>().connect<&OrbitalScene::OnOrbitalComponentDestruct>(this);
    }


    void OrbitalScene::SetParent(Entity entity, Entity parent)
    {
        if (entity.HasComponent<OrbitalComponent>())
        {
            if (!parent.HasComponent<OrbitalComponent>()) {
                LV_WARN("Cannot parent orbital entity to a non-orbital entity!");
                return;
            }

            // Update physics system to reflect new parentage
            m_Physics.SetParent(entity.GetComponent<OrbitalComponent>().PhysicsObjectId,
                parent.GetComponent<OrbitalComponent>().PhysicsObjectId);
        }
        Scene::SetParent(entity, parent);
    }


    void OrbitalScene::SetRootScaling(double meters)
    {
        m_Physics.SetRootScaling(meters);
    }


    double OrbitalScene::GetRootScaling()
    {
        return m_Physics.GetRootScaling();
    }


    void OrbitalScene::SetViewPrimary(Entity primary)
    {
        if (primary.m_EnttId == m_ViewPrimary) return;

        m_ViewPrimary = primary.m_EnttId;

        // TODO : update transforms to be correctly scaled for the viewed scaling space in the editor (no updates outside playtime) ?
    }


    Entity OrbitalScene::GetViewPrimary()
    {
        return Entity{ m_ViewPrimary, this };
    }


    std::vector<Entity> OrbitalScene::GetSecondaries(Entity entity)
    {
        auto entityIds = m_Physics.GetChildren(m_Registry.get<OrbitalComponent>(entity.m_EnttId).PhysicsObjectId);
        std::vector<Entity> secondaries(entityIds.size());
        for (uint32_t i = 0; i < entityIds.size(); i++)
        {
            secondaries[i] = { entityIds[i], this };
        }
        return secondaries;
    }


    void OrbitalScene::OnUpdateRuntime(Timestep dT)
    {
        Scene::OnUpdateRuntime(dT);

        m_Physics.OnUpdate(dT);

        UpdateOrbitalScene();
    }


    void OrbitalScene::OnUpdateEditor(Timestep dT)
    {
        UpdateOrbitalScene();
    }


    void OrbitalScene::UpdateOrbitalScene()
    {
        // Update orbital entity transforms
        auto view = m_Registry.view<OrbitalComponent>();
        for (auto entity : view)
        {
            auto [tc, oc, hc] = m_Registry.get<TransformComponent, OrbitalComponent, HierarchyComponent>(entity);

            if (entity == m_ViewPrimary)
            {
                // View primary
                tc.SetScale(oc.LocalScale);
                tc.SetPosition({ 0.f });
            }
            else if (hc.Parent.m_EnttId == m_ViewPrimary)
            {
                // View secondary
                tc.SetScale(oc.LocalScale * m_Physics.GetLocalSpaceRadius(oc.PhysicsObjectId));
                tc.SetPosition(m_Physics.GetPosition(oc.PhysicsObjectId));
            }
            else
            {
                // Not in view
                tc.SetScale({ 0.f });
                tc.SetPosition({ 0.f });
            }
        }

        // TODO : place non-orbital children (down to hierarchy leaves):
        //      : a non-orbital entity cannot have orbital children so it is
        //      : guaranteed to be purely for extending a parent entity in the 
        //      : scene without affecting orbital behaviour.
    }


    void OrbitalScene::OnRenderRuntime()
    {
        if (!m_Registry.valid(m_ActiveCamera) || !m_Registry.all_of<CameraComponent>(m_ActiveCamera))
        {
            LV_CORE_WARN("Scene has no active camera - no rendering!");
            return;
        }
        auto [camera, camTransform] = m_Registry.get<CameraComponent, TransformComponent>(m_ActiveCamera);
        camera.Camera.SetView(camTransform.GetTransform().Inverse());

        float cameraDistance = sqrtf(camTransform.GetPosition().SqrMagnitude());
        RenderOrbitalScene(camera.Camera, camTransform.GetOrientation(), cameraDistance);
    }


    void OrbitalScene::OnRenderEditor(EditorCamera& camera)
    {
        RenderOrbitalScene(camera.GetCamera(), camera.GetOrientation(), camera.GetDistance());
    }


    void OrbitalScene::RenderOrbitalScene(Camera& camera, const Quaternion& cameraOrientation, float cameraDistance)
    {
        Scene::RenderScene(camera, cameraOrientation);

        // TODO : draw all superior orbital spaces (this primary's primary and siblings, etc) as sprites and point lights
        // : render as separate scenes, in their own scaling spaces, then superimpose - this allows the camera to have accurate
        // perspective without having to place the entities at accurate distances relative to the viewed orbital space.

        Renderer2D::BeginScene(camera);

        // Render orbital visuals
        Entity primary = { m_ViewPrimary, this };
        Vector3 primaryPosition = m_Registry.get<TransformComponent>(m_ViewPrimary).GetPosition();

        if (m_ShowReferenceAxes)
        {
            // X
            Renderer2D::DrawDashedArrow(primaryPosition, primaryPosition + (m_OrbitalReferenceX * m_ReferenceAxisLength),
                m_ReferenceAxisColor, m_ReferenceAxisThickness, m_ReferenceAxisArrowSize, 4.f, 2.f);
            // Y
            Renderer2D::DrawDashedArrow(primaryPosition, primaryPosition + (m_OrbitalReferenceY * m_ReferenceAxisLength),
                m_ReferenceAxisColor, m_ReferenceAxisThickness, m_ReferenceAxisArrowSize, 4.f, 2.f);
            // Normal
            Renderer2D::DrawDashedArrow(primaryPosition, primaryPosition + (m_OrbitalReferenceNormal * 0.5f * m_ReferenceAxisLength),
                m_ReferenceAxisColor, m_ReferenceAxisThickness, m_ReferenceAxisArrowSize, 4.f, 2.f);
        }

        float orbitDrawingThickness = m_OrbitThickness * cameraDistance;

        auto secondaries = m_Physics.GetChildren(m_Registry.get<OrbitalComponent>(m_ViewPrimary).PhysicsObjectId);
        for (auto secondary : secondaries)
        {
            auto& transform = m_Registry.get<TransformComponent>(secondary);
            auto& orbital = m_Registry.get<OrbitalComponent>(secondary);
            if (orbital.GetValidity() != Physics::Validity::Valid) continue;
            const auto& elems = orbital.GetElements();

            // TODO - point light/brightness from OrbitalComponent::Albedo

            // Orbit path
            Vector3 orbitCenter = primaryPosition + (elems.PerifocalX * elems.C);
            Vector4 uiColor = Vector4{ orbital.UIColor, 0.4f };
            {
                switch (elems.Type)
                {
                case Physics::OrbitType::Circle:
                case Physics::OrbitType::Ellipse:
                    Renderer2D::DrawOrbitalEllipse(orbitCenter, elems.PerifocalOrientation * m_OrbitalReferenceFrameOrientation, orbital,
                        uiColor, orbitDrawingThickness, m_OrbitFade, (int)secondary);
                    break;
                case Physics::OrbitType::Hyperbola:
                    Renderer2D::DrawOrbitalHyperbola(orbitCenter, elems.PerifocalOrientation * m_OrbitalReferenceFrameOrientation, orbital,
                        uiColor, orbitDrawingThickness, m_OrbitFade, (int)secondary);
                    break;
                }
            }

            // Local space
            {
                Matrix4 lsTransform = glm::translate(glm::mat4(1.f), (glm::vec3)(transform.GetPosition()));
                lsTransform = lsTransform * Matrix4(cameraOrientation);

                float lsRadius = orbital.GetLocalSpaceRadius();
                lsTransform = glm::scale((glm::mat4)lsTransform, glm::vec3(lsRadius));

                float lsThickness = m_LocalSpaceThickness * cameraDistance / lsRadius;
                Renderer2D::DrawCircle(lsTransform, m_LocalSpaceColor, lsThickness, m_LocalSpaceFade, (int)secondary);
            }

            // Perifocal frame
            if (orbital.ShowMajorMinorAxes)
            {
                // Semi-major axis
                Renderer2D::DrawArrow(orbitCenter, orbitCenter + elems.PerifocalX * elems.SemiMajor,
                    uiColor, m_PerifocalAxisThickness, m_PerifocalAxisArrowSize, (int)secondary);
                Renderer2D::DrawDashedLine(orbitCenter, orbitCenter - elems.PerifocalX * elems.SemiMajor,
                    uiColor, m_PerifocalAxisThickness, 4.f, 2.f, (int)secondary);
                // Semi-minor axis
                Renderer2D::DrawArrow(orbitCenter, orbitCenter + elems.PerifocalY * elems.SemiMinor,
                    uiColor, m_PerifocalAxisThickness, m_PerifocalAxisArrowSize, (int)secondary);
                Renderer2D::DrawDashedLine(orbitCenter, orbitCenter - elems.PerifocalY * elems.SemiMinor,
                    uiColor, m_PerifocalAxisThickness, 4.f, 2.f, (int)secondary);
            }
            if (orbital.ShowNormal)
            {
                Renderer2D::DrawArrow(orbitCenter, orbitCenter + elems.PerifocalNormal * 0.5f * elems.SemiMinor,
                    uiColor, m_PerifocalAxisThickness, m_PerifocalAxisArrowSize, (int)secondary);
            }


            // debug
            /*if (orbital.IsDynamic())
            {
                auto& dynamics = orbital.GetDynamics();
                if (dynamics.EscapeTrueAnomaly > 0.f)
                {
                    static constexpr Vector4 escapePointColor{ 1.f, 0.3f, 0.2f, 0.4f };
                    static constexpr Vector4 entryPointColor{ 0.3f, 1.f, 0.2f, 0.4f };
                    Renderer2D::DrawCircle(primaryPosition + dynamics.EscapePoint, 0.01f, escapePointColor, 1.f, 0.f, (int)secondary);
                    Renderer2D::DrawCircle(primaryPosition + dynamics.EntryPoint, 0.01f, entryPointColor, 1.f, 0.f, (int)secondary);
                }
            }*/
        }

        // TODO : draw tertiaries as point lights orbiting secondaries

        Renderer2D::EndScene();
    }


    void OrbitalScene::OnOrbitalComponentConstruct(entt::registry&, entt::entity entity)
    {
        auto[orbital, hierarchy, transform] = m_Registry.get<OrbitalComponent, HierarchyComponent, TransformComponent>(entity);

        /* Parent of an orbital entity must also be orbital */
        if (hierarchy.Parent.HasComponent<OrbitalComponent>())
        {
            orbital.PhysicsObjectId = m_Physics.Create(entity, m_Registry.get<OrbitalComponent>(hierarchy.Parent.m_EnttId).PhysicsObjectId, 0.0, transform.GetPosition());
        }
        else
        {
            /* Default to the root entity, which is guaranteed to be orbital */
            SetParent(Entity{ entity, this }, Entity{ m_Root, this });

            orbital.PhysicsObjectId = m_Physics.Create(entity); /* Primary defaults to root physics object which corresponds to the root entity */
            m_Physics.SetPosition(orbital.PhysicsObjectId, transform.GetPosition());
        }
        orbital.Physics = &m_Physics;
        orbital.LocalScale = transform.GetScale();
    }


    void OrbitalScene::OnOrbitalComponentDestruct(entt::registry&, entt::entity entity)
    {
        m_Physics.Destroy(m_Registry.get<OrbitalComponent>(entity).PhysicsObjectId);
    }

}
