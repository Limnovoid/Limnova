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
        auto& oc = AddComponent<OrbitalComponent>(m_Entities.at(m_Root));
        oc.PhysicsObjectId = m_Physics.AssignRoot(m_Root);
        oc.Physics = &m_Physics;

        m_OrbitalReferenceFrameOrientation = Quaternion(Vector3::Left(), PIover2f);
        m_OrbitalReferenceX = m_OrbitalReferenceFrameOrientation.RotateVector(Vector3::X());
        m_OrbitalReferenceY = m_OrbitalReferenceFrameOrientation.RotateVector(Vector3::Y());
        m_OrbitalReferenceNormal = m_OrbitalReferenceFrameOrientation.RotateVector(Vector3::Z());

        m_ViewPrimary = m_Root;

        // Signals
        m_Registry.on_construct<OrbitalComponent>().connect<&OrbitalScene::OnOrbitalComponentConstruct>(this);
        m_Registry.on_destroy<OrbitalComponent>().connect<&OrbitalScene::OnOrbitalComponentDestruct>(this);
    }


    Ref<OrbitalScene> OrbitalScene::Copy(Ref<OrbitalScene> scene)
    {
        Ref<OrbitalScene> newScene = CreateRef<OrbitalScene>();

        // Replicate scene hierarchy
        newScene->SetRootId(scene->m_Root);
        newScene->m_ViewportAspectRatio = scene->m_ViewportAspectRatio;
        newScene->m_ActiveCamera = scene->m_ActiveCamera;

        auto& srcRegistry = scene->m_Registry;
        auto& dstRegistry = newScene->m_Registry;
        auto idView = srcRegistry.view<IDComponent>();
        for (auto e : idView)
        {
            UUID uuid = srcRegistry.get<IDComponent>(e).ID;
            if (uuid == scene->m_Root) continue; /* root is already created so we skip it here */

            const std::string& name = srcRegistry.get<TagComponent>(e).Tag;
            newScene->CreateEntityFromUUID(uuid, name);
        }

        CopyAllOfComponent<TransformComponent>(dstRegistry, srcRegistry, newScene->m_Entities);
        CopyAllOfComponent<HierarchyComponent>(dstRegistry, srcRegistry, newScene->m_Entities);
        CopyAllOfComponent<CameraComponent>(dstRegistry, srcRegistry, newScene->m_Entities);
        CopyAllOfComponent<NativeScriptComponent>(dstRegistry, srcRegistry, newScene->m_Entities);
        CopyAllOfComponent<SpriteRendererComponent>(dstRegistry, srcRegistry, newScene->m_Entities);
        CopyAllOfComponent<BillboardSpriteRendererComponent>(dstRegistry, srcRegistry, newScene->m_Entities);
        CopyAllOfComponent<CircleRendererComponent>(dstRegistry, srcRegistry, newScene->m_Entities);
        CopyAllOfComponent<BillboardCircleRendererComponent>(dstRegistry, srcRegistry, newScene->m_Entities);
        CopyAllOfComponent<EllipseRendererComponent>(dstRegistry, srcRegistry, newScene->m_Entities);

        // Replicate scene physics state
        // TODO - copy physics root settings, then copy components should take care of the rest ??
        CopyAllOfComponent<OrbitalComponent>(dstRegistry, srcRegistry, newScene->m_Entities);
        LV_CORE_ASSERT(false, "TODO - replicate physics state explicitly");

        // Copy scene settings
        newScene->m_LocalSpaceColor = scene->m_LocalSpaceColor;
        newScene->m_LocalSpaceThickness = scene->m_LocalSpaceThickness;
        newScene->m_LocalSpaceFade = scene->m_LocalSpaceFade;
        newScene->m_OrbitThickness = scene->m_OrbitThickness;
        newScene->m_OrbitFade = scene->m_OrbitFade;
        newScene->m_OrbitAlpha = scene->m_OrbitAlpha;
        newScene->m_OrbitPointRadius = scene->m_OrbitPointRadius;
        newScene->m_ShowReferenceAxes = scene->m_ShowReferenceAxes;
        newScene->m_ReferenceAxisColor = scene->m_ReferenceAxisColor;
        newScene->m_ReferenceAxisLength = scene->m_ReferenceAxisLength;
        newScene->m_ReferenceAxisThickness = scene->m_ReferenceAxisThickness;
        newScene->m_ReferenceAxisArrowSize = scene->m_ReferenceAxisArrowSize;
        newScene->m_PerifocalAxisThickness = scene->m_PerifocalAxisThickness;
        newScene->m_PerifocalAxisArrowSize = scene->m_PerifocalAxisArrowSize;

        newScene->m_ViewPrimary = scene->m_ViewPrimary;

        newScene->m_OrbitalReferenceFrameOrientation = scene->m_OrbitalReferenceFrameOrientation;
        newScene->m_OrbitalReferenceX = scene->m_OrbitalReferenceX;
        newScene->m_OrbitalReferenceY = scene->m_OrbitalReferenceY;
        newScene->m_OrbitalReferenceNormal = scene->m_OrbitalReferenceNormal;

        return newScene;
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
        if (primary.GetUUID() == m_ViewPrimary) return;

        m_ViewPrimary = primary.GetUUID();

        // TODO : update transforms to be correctly scaled for the viewed scaling space in the editor (no updates outside playtime) ?
    }


    Entity OrbitalScene::GetViewPrimary()
    {
        return Entity{ m_Entities[m_ViewPrimary], this };
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


    std::vector<Entity> OrbitalScene::GetSecondaries(Entity entity)
    {
        auto entityIds = m_Physics.GetChildren(m_Registry.get<OrbitalComponent>(entity.m_EnttId).PhysicsObjectId);
        std::vector<Entity> secondaries(entityIds.size());
        for (uint32_t i = 0; i < entityIds.size(); i++)
        {
            secondaries[i] = Entity{ m_Entities[entityIds[i]], this };
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
            auto [id, tc, oc, hc] = m_Registry.get<IDComponent, TransformComponent, OrbitalComponent, HierarchyComponent>(entity);

            if (id.ID == m_ViewPrimary)
            {
                // View primary
                tc.SetScale(oc.LocalScale);
                tc.SetPosition({ 0.f });
            }
            else if (hc.Parent == m_ViewPrimary)
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
        if (!Valid(m_Entities[m_ActiveCamera]) || !HasComponent<CameraComponent>(m_Entities[m_ActiveCamera]))
        {
            LV_CORE_WARN("Scene has no active camera - no rendering!");
            return;
        }
        auto [camera, camTransform] = GetComponents<CameraComponent, TransformComponent>(m_Entities[m_ActiveCamera]);
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
        auto primary = m_Entities[m_ViewPrimary];
        Vector3 primaryPosition = GetComponent<TransformComponent>(primary).GetPosition();

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

        auto secondaries = m_Physics.GetChildren(GetComponent<OrbitalComponent>(primary).PhysicsObjectId);
        for (auto secondary : secondaries)
        {
            auto entity = m_Entities[secondary];
            int editorPickingId = (int)(entity);
            auto& transform = GetComponent<TransformComponent>(entity);
            auto& orbital = GetComponent<OrbitalComponent>(entity);
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
                        uiColor, orbitDrawingThickness, m_OrbitFade, editorPickingId);
                    break;
                case Physics::OrbitType::Hyperbola:
                    Renderer2D::DrawOrbitalHyperbola(orbitCenter, elems.PerifocalOrientation * m_OrbitalReferenceFrameOrientation, orbital,
                        uiColor, orbitDrawingThickness, m_OrbitFade, editorPickingId);
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
                Renderer2D::DrawCircle(lsTransform, m_LocalSpaceColor, lsThickness, m_LocalSpaceFade, editorPickingId);
            }

            // Perifocal frame
            if (orbital.ShowMajorMinorAxes)
            {
                // Semi-major axis
                Renderer2D::DrawArrow(orbitCenter, orbitCenter + elems.PerifocalX * elems.SemiMajor,
                    uiColor, m_PerifocalAxisThickness, m_PerifocalAxisArrowSize, editorPickingId);
                Renderer2D::DrawDashedLine(orbitCenter, orbitCenter - elems.PerifocalX * elems.SemiMajor,
                    uiColor, m_PerifocalAxisThickness, 4.f, 2.f, editorPickingId);
                // Semi-minor axis
                Renderer2D::DrawArrow(orbitCenter, orbitCenter + elems.PerifocalY * elems.SemiMinor,
                    uiColor, m_PerifocalAxisThickness, m_PerifocalAxisArrowSize, editorPickingId);
                Renderer2D::DrawDashedLine(orbitCenter, orbitCenter - elems.PerifocalY * elems.SemiMinor,
                    uiColor, m_PerifocalAxisThickness, 4.f, 2.f, editorPickingId);
            }
            if (orbital.ShowNormal)
            {
                Renderer2D::DrawArrow(orbitCenter, orbitCenter + elems.PerifocalNormal * 0.5f * elems.SemiMinor,
                    uiColor, m_PerifocalAxisThickness, m_PerifocalAxisArrowSize, editorPickingId);
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
        auto id = m_Registry.get<IDComponent>(entity).ID;
        if (HasComponent<OrbitalComponent>(m_Entities[hierarchy.Parent]))
        {
            orbital.PhysicsObjectId = m_Physics.Create(id, GetComponent<OrbitalComponent>(m_Entities[hierarchy.Parent]).PhysicsObjectId, 0.0, transform.GetPosition());
        }
        else
        {
            /* Default to the root entity, which is guaranteed to be orbital in OrbitalScene */
            HierarchyDisconnect(entity);
            HierarchyConnect(entity, m_Entities[m_Root]);

            orbital.PhysicsObjectId = m_Physics.Create(id); /* Primary defaults to root physics object which corresponds to the root entity */
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
