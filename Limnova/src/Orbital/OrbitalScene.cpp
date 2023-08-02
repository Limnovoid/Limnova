#include "OrbitalScene.h"

#include <Scene/Entity.h>
#include <Scene/Components.h>
#include <Renderer/Renderer2D.h>


namespace Limnova
{

    OrbitalScene::OrbitalScene()
        : Scene()
    {
        // Orbital scene setup
        /* NOTE : root MUST be assigned before signal setup - the root's OrbitalComponent should NOT create
         * a new object in OrbitalPhysics */
        AddComponent<OrbitalComponent>(m_Entities.at(m_Root));

        m_OrbitalReferenceFrameOrientation = Quaternion(Vector3::Left(), PIover2f);
        m_OrbitalReferenceX = m_OrbitalReferenceFrameOrientation.RotateVector(Vector3::X());
        m_OrbitalReferenceY = m_OrbitalReferenceFrameOrientation.RotateVector(Vector3::Y());
        m_OrbitalReferenceNormal = m_OrbitalReferenceFrameOrientation.RotateVector(Vector3::Z());

        m_TrackingEntity = m_Root;
        m_ViewSpaceRelativeToTrackedEntity = 0; /* root local space (first local space owned by root object) */

        // Entt signals
        m_Registry.on_construct<OrbitalComponent>().connect<&OrbitalScene::OnOrbitalComponentConstruct>(this);
        m_Registry.on_destroy<OrbitalComponent>().connect<&OrbitalScene::OnOrbitalComponentDestruct>(this);

        // Physics callbacks
        m_Physics.m_LSpaceChangedCallback = { [&](OrbitalPhysics::ObjectNode objNode) {
            entt::entity objectEnttId = m_PhysicsToEnttIds[objNode.Id()];
            HierarchyDisconnect(objectEnttId);
            HierarchyConnect(objectEnttId, m_PhysicsToEnttIds[objNode.ParentObj().Id()]);
        } };
    }


    Ref<OrbitalScene> OrbitalScene::Copy(Ref<OrbitalScene> scene)
    {
        Ref<OrbitalScene> newScene = CreateRef<OrbitalScene>();

        // Copy base Scene
        Scene::Copy(scene, newScene);

        // Copy OrbitalPhysics
        newScene->m_Physics = scene->m_Physics;
        /* Suspend OrbitalComponent dependencies while copying, to avoid creating unnecessary physics objects */
        newScene->m_Registry.on_construct<OrbitalComponent>().disconnect<&OrbitalScene::OnOrbitalComponentConstruct>(newScene.get());
        newScene->CopyAllOfComponent<OrbitalComponent>(scene->m_Registry);
        newScene->m_Registry.on_construct<OrbitalComponent>().connect<&OrbitalScene::OnOrbitalComponentConstruct>(newScene.get());

        // Copy OrbitalScene settings
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

        newScene->m_TrackingEntity = scene->m_TrackingEntity;

        newScene->m_OrbitalReferenceFrameOrientation = scene->m_OrbitalReferenceFrameOrientation;
        newScene->m_OrbitalReferenceX = scene->m_OrbitalReferenceX;
        newScene->m_OrbitalReferenceY = scene->m_OrbitalReferenceY;
        newScene->m_OrbitalReferenceNormal = scene->m_OrbitalReferenceNormal;

        return newScene;
    }


    Entity OrbitalScene::CreateEntityFromUUID(UUID uuid, const std::string& name, UUID parent)
    {
        LV_CORE_ASSERT(false, "Just checking this actually gets called");
        Entity newEntity = Scene::CreateEntityFromUUID(uuid, name, parent);
        AddComponent<OrbitalHierarchyComponent>(newEntity.m_EnttId).LocalSpaceRelativeToParent = -1;
        return newEntity;
    }


    Entity OrbitalScene::DuplicateEntity(Entity entity)
    {
        Entity newEntity = CreateEntityAsChild(GetParent(entity), entity.GetName() + " (copy)");

        CopyComponentIfExists<TransformComponent>(newEntity.m_EnttId, entity.m_EnttId);
        /* DO NOT copy HierarchyComponent - sibling relationships must be different and should be handled by CreateEntity() */
        CopyComponentIfExists<CameraComponent>(newEntity.m_EnttId, entity.m_EnttId);
        CopyComponentIfExists<NativeScriptComponent>(newEntity.m_EnttId, entity.m_EnttId);
        CopyComponentIfExists<SpriteRendererComponent>(newEntity.m_EnttId, entity.m_EnttId);
        CopyComponentIfExists<BillboardSpriteRendererComponent>(newEntity.m_EnttId, entity.m_EnttId);
        CopyComponentIfExists<CircleRendererComponent>(newEntity.m_EnttId, entity.m_EnttId);
        CopyComponentIfExists<BillboardCircleRendererComponent>(newEntity.m_EnttId, entity.m_EnttId);
        CopyComponentIfExists<EllipseRendererComponent>(newEntity.m_EnttId, entity.m_EnttId);
        CopyComponentIfExists<OrbitalHierarchyComponent>(newEntity.m_EnttId, entity.m_EnttId);

        if (entity.HasComponent<OrbitalComponent>()) {
            auto& srcOc = entity.GetComponent<OrbitalComponent>();
            auto& dstOc = newEntity.AddComponent<OrbitalComponent>();

            dstOc.Object.SetDynamic(srcOc.Object.IsDynamic());
            dstOc.Object.SetMass(srcOc.Object.GetObj().State.Mass);
            dstOc.Object.SetPosition(srcOc.Object.GetObj().State.Position);
            dstOc.Object.SetVelocity(srcOc.Object.GetObj().State.Velocity);

            for (auto srcLsp : srcOc.LocalSpaces) {
                if (srcLsp.Influencing()) continue; /* influencing LSPs are handled by OrbitalPhysics */
                auto newLsp = OrbitalPhysics::AddLocalSpace(dstOc.Object);
                newLsp.SetRadius(srcLsp.GetLSpace().Radius);
            }

            dstOc.Albedo = srcOc.Albedo;
            dstOc.UIColor = srcOc.UIColor;
            dstOc.ShowMajorMinorAxes = srcOc.ShowMajorMinorAxes;
            dstOc.ShowNormal = srcOc.ShowNormal;

            LV_CORE_ASSERT((dstOc.Object.GetObj().State.Position - srcOc.Object.GetObj().State.Position).SqrMagnitude() < 1e-5f, "Failed to adequately replicate position!");
            LV_CORE_ASSERT(dstOc.Object.GetElements().E - srcOc.Object.GetElements().E < 1e-5f, "Failed to adequately replicate orbit shape!");
            LV_CORE_ASSERT(dstOc.Object.GetDynamics().EscapeTrueAnomaly - srcOc.Object.GetDynamics().EscapeTrueAnomaly < 1e-5f, "Failed to adequately replicate dynamics!");
        }
        return newEntity;
    }


    void OrbitalScene::SetRootScaling(double meters)
    {
        OrbitalPhysics::SetRootSpaceScaling(meters);
    }


    double OrbitalScene::GetRootScaling()
    {
        return OrbitalPhysics::GetRootLSpaceNode().GetLSpace().MetersPerRadius;
    }


    void OrbitalScene::SetTrackingEntity(Entity primary)
    {
        if (primary.GetUUID() == m_TrackingEntity) return;

        m_TrackingEntity = primary.GetUUID();

        // TODO : update relative view space index to keep the view space the same

        // TODO : update transforms to be correctly scaled for the viewed scaling space in the editor (no updates outside playtime) ?
    }

    void OrbitalScene::SetRelativeViewSpace(int viewSpaceRelativeToTrackingEntity)
    {
        LV_CORE_ASSERT(false, "TODO: OrbitalScene::SetRelativeViewSpace()");
    }


    Entity OrbitalScene::GetViewPrimary()
    {
        return Entity{ m_Entities[m_TrackingEntity], this };
    }


    void OrbitalScene::SetParent(Entity entity, Entity parent)
    {
        SetParentAndLocalSpace(entity, parent, -1);
    }

    void OrbitalScene::SetParentAndLocalSpace(Entity entity, Entity parent, int localSpaceRelativeToParent)
    {
        LV_ASSERT(entity.GetUUID() != m_Root, "Cannot set local space of root object!");
        LV_ASSERT(localSpaceRelativeToParent >= -1, "Invalid localSpaceRelativeToParent!");
        LV_ASSERT(localSpaceRelativeToParent == -1 || (parent.HasComponent<OrbitalComponent>()
            && localSpaceRelativeToParent < parent.GetComponent<OrbitalComponent>().LocalSpaces.size()),
            "Given localSpaceRelativeToParent is out of bounds!");

        if (entity.HasComponent<OrbitalComponent>())
        {
            LV_ASSERT(parent.HasComponent<OrbitalComponent>(), "Cannot parent orbital entity to a non-orbital entity!");

            // Update physics system to reflect new parentage
            auto& oc = entity.GetComponent<OrbitalComponent>();
            auto& parentOc = parent.GetComponent<OrbitalComponent>();

            OrbitalPhysics::LSpaceNode newLsp;
            if (localSpaceRelativeToParent == -1) {
                newLsp = parentOc.Object.IsRoot() ? OrbitalPhysics::GetRootLSpaceNode() : parentOc.Object.ParentLsp();
            }
            else {
                newLsp = parentOc.LocalSpaces[localSpaceRelativeToParent];
            }
            oc.Object.SetLocalSpace(newLsp);
        }
        Scene::SetParent(entity, parent);
        entity.GetComponent<OrbitalHierarchyComponent>().LocalSpaceRelativeToParent = localSpaceRelativeToParent;
    }

    void OrbitalScene::SetLocalSpace(Entity entity, int localSpaceRelativeToParent)
    {
        LV_ASSERT(localSpaceRelativeToParent >= -1, "Invalid localSpaceRelativeToParent!");
        LV_ASSERT(entity.GetUUID() != m_Root, "Cannot set local space of root object!");

        Entity parent = entity.GetParent();
        LV_ASSERT(parent.HasComponent<OrbitalComponent>(),
            "Cannot set local space of an object which is parented to a non-orbital object - the parent does not have local spaces!");

        auto& oc = entity.GetComponent<OrbitalComponent>();
        auto& parentOc = parent.GetComponent<OrbitalComponent>();

        OrbitalPhysics::LSpaceNode newLsp;
        if (localSpaceRelativeToParent == -1) {
            newLsp = parentOc.Object.IsRoot() ? OrbitalPhysics::GetRootLSpaceNode() : parentOc.Object.ParentLsp();
        }
        else {
            LV_ASSERT(localSpaceRelativeToParent < parentOc.LocalSpaces.size(), "Given localSpaceRelativeToParent is out of bounds!");
            newLsp = parentOc.LocalSpaces[localSpaceRelativeToParent];
        }
        oc.Object.SetLocalSpace(newLsp);
    }

    OrbitalPhysics::LSpaceNode OrbitalScene::GetLocalSpace(Entity entity)
    {
        return GetEntityLSpace(entity.m_EnttId);
    }


    /// <summary>
    /// Returns vector containing all orbital entities in local spaces belonging to the given entity, sorted in descending order of local spaces.
    /// </summary>
    std::vector<Entity> OrbitalScene::GetSecondaries(Entity entity)
    {
        LV_ASSERT(entity.HasComponent<OrbitalComponent>(), "Cannot get secondaries of a non-orbital component!");

        std::vector<Entity> secondaries;
        for (auto lsp : entity.GetComponent<OrbitalComponent>().LocalSpaces)
        {
            std::vector<OrbitalPhysics::ObjectNode> objNodes;
            lsp.GetLocalObjects(objNodes);
            for (auto objNode : objNodes) {
                secondaries.push_back({ m_PhysicsToEnttIds[objNode.Id()], this });
            }
        }
        return secondaries;
    }

    /// <summary>
    /// Returns vector containing all orbital entities in local spaces belonging to or descended from the given primary. sprted in descending order of local spaces.
    /// E.g, contains entities in the primary's local spaces, then the entities in the local spaces of those entities, and so on.
    /// </summary>
    std::vector<Entity> OrbitalScene::GetSatellites(Entity primary)
    {
        LV_ASSERT(primary.HasComponent<OrbitalComponent>(), "Cannot get satellites of a non-orbital component!");

        std::vector<Entity> satellites = GetSecondaries(primary);
        size_t numAdded = satellites.size();
        do {
            size_t end = satellites.size();
            size_t idx = end - numAdded;
            numAdded = 0;
            for (; idx < end; idx++)
            {
                auto children = GetSecondaries(satellites[idx]);
                satellites.insert(satellites.end(), children.begin(), children.end());
                numAdded += children.size();
            }
        } while (numAdded > 0);
        return satellites;
    }


    void OrbitalScene::OnStartRuntime()
    {
        Scene::OnStartRuntime();
    }


    void OrbitalScene::OnUpdateRuntime(Timestep dT)
    {
        Scene::OnUpdateRuntime(dT);

        OrbitalPhysics::OnUpdate(dT);

        UpdateOrbitalScene();
    }


    void OrbitalScene::OnUpdateEditor(Timestep dT)
    {
        UpdateOrbitalScene();
    }


    void OrbitalScene::UpdateOrbitalScene()
    {
        auto tcView = m_Registry.view<OrbitalComponent>();
        for (auto entity : tcView) {
            auto& tc = m_Registry.get<TransformComponent>(entity);
            tc.SetScale({ 0.f });
        }

        auto& lsp = m_ViewLSpace.GetLSpace();

        auto viewParent = m_ViewLSpace.ParentObj();
        {
            auto [tc, ohc] = GetComponents<TransformComponent, OrbitalHierarchyComponent>(m_PhysicsToEnttIds[viewParent.Id()]);
            tc.SetScale((Vector3)(ohc.AbsoluteScale / lsp.MetersPerRadius));
            tc.SetPosition({ 0.f });
        }

        std::vector<OrbitalPhysics::ObjectNode> viewObjs;
        m_ViewLSpace.GetLocalObjects(viewObjs);
        for (auto viewObj : viewObjs)
        {
            auto [tc, ohc, oc] = GetComponents<TransformComponent, OrbitalHierarchyComponent, OrbitalComponent>(m_PhysicsToEnttIds[viewObj.Id()]);
            tc.SetScale((Vector3)(ohc.AbsoluteScale / lsp.MetersPerRadius));
            tc.SetPosition(oc.Object.GetObj().State.Position);
        }


        // Update orbital entity transforms
        //auto view = m_Registry.view<OrbitalComponent>();
        //for (auto entity : view)
        //{
        //    auto [idc, tc, oc, hc, ohc] = m_Registry.get<IDComponent, TransformComponent, OrbitalComponent, HierarchyComponent, OrbitalHierarchyComponent>(entity);

        //    if (idc.ID == m_TrackingEntity)
        //    {
        //        // View primary
        //        tc.SetScale(oc.AbsoluteScale);
        //        tc.SetPosition({ 0.f });
        //    }
        //    else if (hc.Parent == m_TrackingEntity)
        //    {
        //        // View secondary
        //        tc.SetScale(oc.LocalScale * oc.GetLocalSpaceRadius());
        //        tc.SetPosition(oc.GetPosition());
        //    }
        //    else
        //    {
        //        // Not in view
        //        tc.SetScale({ 0.f });
        //        tc.SetPosition({ 0.f });
        //    }
        //}

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
        auto& lsp = m_ViewLSpace.GetLSpace();
        auto viewParentObjNode = m_ViewLSpace.ParentObj();
        std::vector<OrbitalPhysics::ObjectNode> viewObjs;
        m_ViewLSpace.GetLocalObjects(viewObjs);

        Vector3 viewCenter = GetComponent<TransformComponent>(m_PhysicsToEnttIds[viewParentObjNode.Id()]).GetPosition();
        if (m_ShowReferenceAxes) {
            // X
            Renderer2D::DrawDashedArrow(viewCenter, viewCenter + (m_OrbitalReferenceX * m_ReferenceAxisLength),
                m_ReferenceAxisColor, m_ReferenceAxisThickness, m_ReferenceAxisArrowSize, 4.f, 2.f);
            // Y
            Renderer2D::DrawDashedArrow(viewCenter, viewCenter + (m_OrbitalReferenceY * m_ReferenceAxisLength),
                m_ReferenceAxisColor, m_ReferenceAxisThickness, m_ReferenceAxisArrowSize, 4.f, 2.f);
            // Normal
            Renderer2D::DrawDashedArrow(viewCenter, viewCenter + (m_OrbitalReferenceNormal * 0.5f * m_ReferenceAxisLength),
                m_ReferenceAxisColor, m_ReferenceAxisThickness, m_ReferenceAxisArrowSize, 4.f, 2.f);
        }

        float orbitDrawingThickness = m_OrbitThickness * cameraDistance;

        for (auto viewObjNode : viewObjs)
        {
            auto entity = m_PhysicsToEnttIds[viewObjNode.Id()];
            int editorPickingId = (int)(entity);
            auto [tc, oc] = GetComponents<TransformComponent, OrbitalComponent>(entity);
            auto& object = oc.Object.GetObj();
            auto& elems = oc.Object.GetElements();

            if (object.Validity != OrbitalPhysics::Validity::Valid) continue;

            // TODO - point light/brightness from OrbitalComponent::Albedo

            // Orbit path
            Vector3 posFromPrimary = oc.Object.LocalPositionFromPrimary();
            Vector3 orbitCenter = tc.GetPosition() - posFromPrimary + (elems.PerifocalX * elems.C);
            Vector4 uiColor = Vector4{ oc.UIColor, 0.4f };
            {
                switch (elems.Type)
                {
                case OrbitalPhysics::OrbitType::Circle:
                case OrbitalPhysics::OrbitType::Ellipse:
                    Renderer2D::DrawOrbitalEllipse(orbitCenter, elems.PerifocalOrientation * m_OrbitalReferenceFrameOrientation, oc,
                        uiColor, orbitDrawingThickness, m_OrbitFade, editorPickingId);
                    break;
                case OrbitalPhysics::OrbitType::Hyperbola:
                    Renderer2D::DrawOrbitalHyperbola(orbitCenter, elems.PerifocalOrientation * m_OrbitalReferenceFrameOrientation, oc,
                        uiColor, orbitDrawingThickness, m_OrbitFade, editorPickingId);
                    break;
                }
            }

            // Local spaces
            for (auto lspNode : oc.LocalSpaces)
            {
                Matrix4 lsTransform = glm::translate(glm::mat4(1.f), (glm::vec3)(tc.GetPosition()));
                lsTransform = lsTransform * Matrix4(cameraOrientation);

                float lsRadius = lspNode.GetLSpace().Radius;
                lsTransform = glm::scale((glm::mat4)lsTransform, glm::vec3(lsRadius));

                float lsThickness = m_LocalSpaceThickness * cameraDistance / lsRadius;
                Renderer2D::DrawCircle(lsTransform, m_LocalSpaceColor, lsThickness, m_LocalSpaceFade, editorPickingId);
            }

            // Perifocal frame
            if (oc.ShowMajorMinorAxes)
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
            if (oc.ShowNormal)
            {
                Renderer2D::DrawArrow(tc.GetPosition(), tc.GetPosition() + elems.PerifocalNormal * 0.5f * elems.SemiMinor,
                    uiColor, m_PerifocalAxisThickness, m_PerifocalAxisArrowSize, editorPickingId);
            }


            // debug
#ifdef EXCLUDE
            if (orbital.IsDynamic())
            {
                auto& dynamics = orbital.GetDynamics();
                if (dynamics.EscapeTrueAnomaly > 0.f)
                {
                    static constexpr Vector4 escapePointColor{ 1.f, 0.3f, 0.2f, 0.4f };
                    static constexpr Vector4 entryPointColor{ 0.3f, 1.f, 0.2f, 0.4f };
                    Renderer2D::DrawCircle(primaryPosition + dynamics.EscapePoint, 0.01f, escapePointColor, 1.f, 0.f, (int)secondary);
                    Renderer2D::DrawCircle(primaryPosition + dynamics.EntryPoint, 0.01f, entryPointColor, 1.f, 0.f, (int)secondary);
                }
            }
            if (m_Physics.IsIntegrationLinear(orbital.PhysicsObjectId)) {
                Renderer2D::DrawDashedLine(transform.GetPosition(), primaryPosition, { 1.f, 0.3f, 0.2f, m_OrbitAlpha }, m_OrbitThickness, 2.f, 2.f, editorPickingId);
            }
#endif
        }

        // TODO : draw tertiaries as point lights orbiting secondaries

        Renderer2D::EndScene();
    }


    void OrbitalScene::OnStopRuntime()
    {
        Scene::OnStopRuntime();
    }


    OrbitalPhysics::LSpaceNode OrbitalScene::GetEntityLSpace(entt::entity entity)
    {
        auto [hc, ohc] = GetComponents<HierarchyComponent, OrbitalHierarchyComponent>(entity);
        if (ohc.LocalSpaceRelativeToParent == -1) {
            return hc.Parent == m_Root ? OrbitalPhysics::GetRootLSpaceNode() : GetEntityLSpace(m_Entities[hc.Parent]);
        }
        else {
            LV_CORE_ASSERT(HasComponent<OrbitalComponent>(m_Entities[hc.Parent]), "Invalid LocalSpaceRelativeToParent!");
            auto& parentOc = GetComponent<OrbitalComponent>(m_Entities[hc.Parent]);
            LV_CORE_ASSERT(ohc.LocalSpaceRelativeToParent < parentOc.LocalSpaces.size(), "Invalid LocalSpaceRelativeToParent!");
            return parentOc.LocalSpaces[ohc.LocalSpaceRelativeToParent];
        }
    }


    void OrbitalScene::OnOrbitalComponentConstruct(entt::registry&, entt::entity entity)
    {
        auto[oc, hc, tc, ohc] = GetComponents<OrbitalComponent, HierarchyComponent, TransformComponent, OrbitalHierarchyComponent>(entity);

        /* Parent of an orbital entity must also be orbital */
        OrbitalPhysics::LSpaceNode localSpace = GetEntityLSpace(entity);
        {
            entt::entity parent = m_PhysicsToEnttIds[localSpace.ParentObj().Id()];
            if (parent != m_Entities[hc.Parent]) {
                HierarchyDisconnect(entity);
                HierarchyConnect(entity, parent);
            }
        }
        oc.Object = OrbitalPhysics::Create(localSpace, 0.0, tc.GetPosition());

        //if (HasComponent<OrbitalComponent>(m_Entities[hc.Parent])) {
        //    oc.Object = OrbitalPhysics::Create(GetComponent<OrbitalComponent>(
        //        m_Entities[hc.Parent]).LocalSpaces[ohc.LocalSpaceRelativeToParent],
        //        0.0, tc.GetPosition());
        //}
        //else {
        //    /* Default to the root entity, which is guaranteed to be orbital in OrbitalScene */
        //    HierarchyDisconnect(entity);
        //    HierarchyConnect(entity, m_Entities[m_Root]);

        //    oc.Object = OrbitalPhysics::Create(); /* Primary defaults to root physics object which corresponds to the root entity */
        //    oc.Object.SetPosition(tc.GetPosition());
        //}
    }


    void OrbitalScene::OnOrbitalComponentDestruct(entt::registry&, entt::entity entity)
    {
        OrbitalPhysics::Destroy(GetComponent<OrbitalComponent>(entity).Object);
    }

}
