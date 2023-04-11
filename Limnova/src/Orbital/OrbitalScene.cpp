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

        m_ViewPrimary = m_Root;

        // Signals
        m_Registry.on_construct<OrbitalComponent>().connect<&OrbitalScene::OnOrbitalComponentConstruct>(this);
        m_Registry.on_destroy<OrbitalComponent>().connect<&OrbitalScene::OnOrbitalComponentDestruct>(this);
    }


    void OrbitalScene::SetParent(Entity entity, Entity parent)
    {
        if (entity.HasComponent<OrbitalComponent>())
        {
            if (!parent.HasComponent<OrbitalComponent>()) return; /* Cannot parent orbital entity to a non-orbital entity */

            // Update physics system to reflect new parentage
            auto& orbital = entity.GetComponent<OrbitalComponent>();
            auto& porbital = parent.GetComponent<OrbitalComponent>();
            m_Physics.SetParent(orbital.PhysicsObjectId, porbital.PhysicsObjectId);
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
            else if (hc.Parent == Entity(m_ViewPrimary, this))
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

        RenderOrbitalScene(camera.Camera, camTransform.GetOrientation());
    }


    void OrbitalScene::OnRenderEditor(EditorCamera& camera)
    {
        RenderOrbitalScene(camera.GetCamera(), camera.GetOrientation());
    }


    void OrbitalScene::RenderOrbitalScene(Camera& camera, const Quaternion& cameraOrientation)
    {
        Scene::RenderScene(camera, cameraOrientation);

        // TODO : draw all superior orbital spaces (this primary's primary and siblings, etc) as sprites and point lights
        // : render as separate scenes, in their own scaling spaces, then superimpose - this allows the camera to have accurate
        // perspective without having to place the entities at accurate distances relative to the viewed orbital space.

        Renderer2D::BeginScene(camera);

        // Render orbital visuals
        Entity primary = { m_ViewPrimary, this };
        Vector3 primaryPosition = m_Registry.get<TransformComponent>(m_ViewPrimary).GetPosition();
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
                // TODO - get thickness from camera distance
                float orbitThickness = m_OrbitThickness * elems.SemiMinor / powf(elems.SemiMajor, m_OrbitThicknessFactor);

                Matrix4 orbitTransform = glm::translate(glm::mat4(1.f), (glm::vec3)orbitCenter);
                orbitTransform = orbitTransform * Matrix4(elems.PerifocalOrientation * m_OrbitalReferenceFrameOrientation);
                orbitTransform = glm::scale((glm::mat4)orbitTransform, glm::vec3(glm::vec2{ 2.f * elems.SemiMajor, 2.f * elems.SemiMinor }, 0.f));
                Renderer2D::DrawEllipse(orbitTransform, elems.SemiMajor / elems.SemiMinor, uiColor, orbitThickness, 0.f, (int)secondary);
            }

            // Influence
            {
                Matrix4 influenceTransform = glm::translate(glm::mat4(1.f), (glm::vec3)(transform.GetPosition()));
                influenceTransform = influenceTransform * Matrix4(cameraOrientation);

                float influenceRadius = orbital.GetLocalSpaceRadius();
                influenceTransform = glm::scale((glm::mat4)influenceTransform, glm::vec3(influenceRadius));

                float influenceThickness = m_InfluenceThickness / influenceRadius;
                Renderer2D::DrawCircle(influenceTransform, m_InfluenceColor, influenceThickness, m_InfluenceFade, (int)secondary);
            }

            // Orbit centre
            {
                //Renderer2D::DrawCircle(orbitCenter, 0.01f, uiColor, 1.f, 0.f, (int)secondary);
                Matrix4 centreTransform = glm::translate(glm::mat4(1.f), (glm::vec3)(orbitCenter));
                centreTransform = centreTransform * Matrix4(cameraOrientation);
                centreTransform = glm::scale((glm::mat4)centreTransform, glm::vec3(m_OrbitPointRadius));
                Renderer2D::DrawCircle(centreTransform, uiColor, 1.f, 0.f, (int)secondary);
            }
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
