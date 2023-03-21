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
        if (!m_Registry.valid(m_ActiveCamera) || !m_Registry.all_of<PerspectiveCameraComponent>(m_ActiveCamera))
        {
            LV_CORE_WARN("Scene has no active camera - no rendering!");
            return;
        }
        auto [camera, camTransform] = m_Registry.get<PerspectiveCameraComponent, TransformComponent>(m_ActiveCamera);
        camera.Camera.SetView(glm::inverse(camTransform.GetTransform()));

        Renderer2D::BeginScene(camera.Camera);

        // Render view primary, its secondaries, and its non-orbital children
        {
            auto [transform, sprite] = m_Registry.get<TransformComponent, SpriteRendererComponent>(m_ViewPrimary);
            Renderer2D::DrawQuad(transform.GetTransform(), sprite.Color);
        }
        Entity primary = { m_ViewPrimary, this };
        for (auto child : GetChildren(primary))
        {
            if (!m_Registry.all_of<SpriteRendererComponent>(child.m_EnttId)) continue;
            
            auto [transform, sprite] = m_Registry.get<TransformComponent, SpriteRendererComponent>(child.m_EnttId);
            Renderer2D::DrawQuad(transform.GetTransform(), sprite.Color);

            // TODO - point light/brightness from OrbitalComponent::Albedo
        }

        // TODO : draw tertiaries as point lights orbiting secondaries

        Renderer2D::EndScene();

        // TODO : draw orbits
    }


    void OrbitalScene::OnOrbitalComponentConstruct(entt::registry&, entt::entity entity)
    {
        auto[orbital, hierarchy, transform] = m_Registry.get<OrbitalComponent, HierarchyComponent, TransformComponent>(entity);
        if (hierarchy.Parent.HasComponent<OrbitalComponent>())
        {
            orbital.PhysicsObjectId = m_Physics.Create(entity, m_Registry.get<OrbitalComponent>(hierarchy.Parent.m_EnttId).PhysicsObjectId, 0.0, transform.GetPosition());
        }
        else
        {
            SetParent(Entity{ entity, this }, Entity{ m_Root, this });
            orbital.PhysicsObjectId = m_Physics.Create(entity);
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
