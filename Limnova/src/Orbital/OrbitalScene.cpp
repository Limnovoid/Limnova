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


    void OrbitalScene::OnUpdate(Timestep dT)
    {
        Scene::OnUpdate(dT);

        //m_Physics.OnUpdate(dT);

        // Update orbital entity transforms
        Entity primary = Entity{ m_ViewPrimary, this };
        auto& pOrbital = m_Registry.get<OrbitalComponent>(m_ViewPrimary);
        {
            auto& transform = m_Registry.get<TransformComponent>(m_ViewPrimary);
            transform.SetScale(pOrbital.LocalScale);
            transform.SetPosition({ 0.f });
        }
        auto secondaries = m_Physics.GetChildren(pOrbital.PhysicsObjectId);
        for (auto secondary : secondaries)
        {
            auto& sOrbital = m_Registry.get<OrbitalComponent>(secondary);
            auto& transform = m_Registry.get<TransformComponent>(secondary);
            transform.SetScale(sOrbital.LocalScale * m_Physics.GetLocalSpaceRadius(sOrbital.PhysicsObjectId));
            transform.SetPosition(m_Physics.GetPosition(sOrbital.PhysicsObjectId));
        }
    }


    void OrbitalScene::OnRender()
    {
        // TODO : draw all superior orbital spaces (this primary's primary and siblings, etc) as sprites and point lights
        // : render as separate scenes, in their own scaling spaces, then superimpose - this allows the camera to have accurate
        // perspective without having to place the entities at accurate distances relative to the viewed orbital space.

        // Camera
        if (!m_Registry.valid(m_ActiveCamera) || !m_Registry.all_of<CameraComponent>(m_ActiveCamera))
        {
            LV_CORE_WARN("Scene has no active camera - no rendering!");
            return;
        }
        auto [camera, camTransform] = m_Registry.get<CameraComponent, TransformComponent>(m_ActiveCamera);
        camera.Camera.SetView(glm::inverse(camTransform.GetTransform()));

        Renderer2D::BeginScene(camera.Camera);

        // Render view primary, its secondaries, and its non-orbital children
        Entity primary = { m_ViewPrimary, this };
        Vector3 primaryPosition;
        {
            auto [transform, sprite] = m_Registry.get<TransformComponent, SpriteRendererComponent>(m_ViewPrimary);
            primaryPosition = transform.GetPosition();
            Renderer2D::DrawQuad(transform.GetTransform(), sprite.Color);
        }
        for (auto child : GetChildren(primary))
        {
            // Sprite stuff
            if (!m_Registry.all_of<SpriteRendererComponent>(child.m_EnttId)) continue;

            auto [transform, sprite] = m_Registry.get<TransformComponent, SpriteRendererComponent>(child.m_EnttId);
            Renderer2D::DrawQuad(transform.GetTransform(), sprite.Color);

            // TODO : recursively render all non-orbital children (down to the leaves):
            //      : a non-orbital entity cannot have orbital children so it is
            //      : guaranteed to be purely for extending a parent entity in the 
            //      : scene without affecting orbital behaviour.

            // Orbital stuff
            if (!m_Registry.all_of<OrbitalComponent>(child.m_EnttId)) continue;

            auto& orbital = m_Registry.get<OrbitalComponent>(child.m_EnttId);
            if (orbital.GetValidity() != Physics::Validity::Valid) continue;

            // TODO - point light/brightness from OrbitalComponent::Albedo

            float influenceThickness = m_InfluenceThickness / orbital.GetLocalSpaceRadius();
            Renderer2D::DrawCircle(transform.GetPosition(), orbital.GetLocalSpaceRadius(), m_InfluenceColor, influenceThickness, m_InfluenceFade);

            auto& elems = orbital.GetElements();

            // Orbit
            // TODO - orbits as child entities:
            // - use transform component to eliminate the manual transform computation below (constant orbits have constant transforms, below is unnecessary recomputation!)
            // - allows UI integration (clickable orbits) !!!
            Vector3 orbitCenter = primaryPosition + (elems.PerifocalX * elems.C);
            Vector4 orbitColor = { sprite.Color.XYZ(), m_OrbitAlpha };
            // TODO - constant absolute thickness
            float orbitThickness = m_OrbitThickness * elems.SemiMinor / powf(elems.SemiMajor, m_OrbitThicknessFactor);

            glm::mat4 orbitTransform = glm::translate(glm::mat4(1.f), (glm::vec3)orbitCenter);
            orbitTransform = orbitTransform * Matrix4(elems.PerifocalOrientation).mat;
            orbitTransform = glm::scale(orbitTransform, glm::vec3(glm::vec2{ 2.f * elems.SemiMajor, 2.f * elems.SemiMinor }, 0.f));
            Renderer2D::DrawEllipse(orbitTransform, elems.SemiMajor / elems.SemiMinor, orbitColor, orbitThickness, 0.f);

            // Centre
            Renderer2D::DrawCircle(orbitCenter, 0.01f, orbitColor, 1.f, 0.f);
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
