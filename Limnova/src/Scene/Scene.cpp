#include "Scene.h"

#include "Components.h"
#include "Entity.h"

#include <Renderer/Renderer2D.h>


namespace Limnova
{

    static constexpr Vector3 kDefaultAim{ 0.f, 0.f, -1.f };


    Scene::Scene()
    {
#ifdef ENTT_DEMO
        struct Transform
        {
            Vector3 Position;
            Vector4 Orientation;

            Transform() = default;
            Transform(const Transform&) = default;
            Transform(const Vector3& position, const Vector4& orientation)
                : Position(position), Orientation(orientation) {}
        };

        struct Mesh;


        entt::entity entity = m_Registry.create();

        auto& transform = m_Registry.emplace<Transform>(entity, Vector3{ 1.f }, Vector4{ 0.f, 0.f, 0.f, 1.f });

        auto viewObj = m_Registry.view<Transform, Mesh>();
        for (auto entity : viewObj)
        {
            auto [transf, mesh] = viewObj.get<Transform, Mesh>(entity);
        }
#endif

        // Scene hierarchy
        auto root = CreateEntity("Root");
        m_Root = root.m_EnttId;

        // Signals
        m_Registry.on_construct<HierarchyComponent>().connect<&Scene::OnHierarchyComponentConstruction>(this);
        m_Registry.on_destroy<HierarchyComponent>().connect<&Scene::OnHierarchyComponentDestruction>(this);

        m_Registry.on_construct<PerspectiveCameraComponent>().connect<&Scene::OnCameraComponentConstruction>(this);
        m_Registry.on_destroy<PerspectiveCameraComponent>().connect<&Scene::OnCameraComponentDestruction>(this);
    }


    Entity Scene::CreateEntity(const std::string& name)
    {
        Entity entity{ m_Registry.create(), this };

        /* Essential components !!!
         * Every entity is assumed to have these components throughout its lifetime.
         */
        auto& tag = entity.AddComponent<TagComponent>();
        tag.Tag = name.empty() ? "UnnamedEntity" : name;
        entity.AddComponent<TransformComponent>();
        entity.AddComponent<HierarchyComponent>();

        return entity;
    }


    void Scene::Reparent(Entity entity, Entity newParent)
    {
        HierarchyDisconnect(entity);
        HierarchyConnect(entity, newParent);
    }


    void Scene::SetActiveCamera(Entity cameraEntity)
    {
        if (cameraEntity.HasComponent<PerspectiveCameraComponent>())
        {
            m_ActiveCamera = cameraEntity.m_EnttId;
            return;
        }
        LV_CORE_WARN("Attempted to set active camera to a non-camera entity!");
    }


    Entity Scene::GetActiveCamera()
    {
        return { m_ActiveCamera, this };
    }


    void Scene::OnUpdate(Timestep dT)
    {
        if (!m_Registry.valid(m_ActiveCamera) || !m_Registry.all_of<PerspectiveCameraComponent>(m_ActiveCamera))
        {
            LV_CORE_WARN("Scene has no active camera - no rendering!");
            return;
        }

        auto [camera, camTransform] = m_Registry.get<PerspectiveCameraComponent, TransformComponent>(m_ActiveCamera);

        camera.Camera.SetView(glm::inverse(camTransform.GetTransform()));
        Renderer2D::BeginScene(camera.Camera);

        // Sprites
        {
            auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
            for (auto entity : view)
            {
                auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);

                Renderer2D::DrawQuad(transform.GetTransform(), sprite.Color);
            }
        }

        Renderer2D::EndScene();
    }


    void Scene::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowResizeEvent>(LV_BIND_EVENT_FN(Scene::OnWindowResize));
    }


    bool Scene::OnWindowResize(WindowResizeEvent& e)
    {
        auto view = m_Registry.view<PerspectiveCameraComponent>();
        for (auto entity : view)
        {
            auto& camera = view.get<PerspectiveCameraComponent>(entity);
            if (camera.TieAspectToView)
            {
                camera.SetAspect((float)e.GetWidth() / (float)e.GetHeight());
            }
        }
        return false;
    }


    void Scene::OnHierarchyComponentConstruction(entt::registry&, entt::entity entity)
    {
        HierarchyConnect({ entity, this }, { m_Root, this });
    }


    void Scene::OnHierarchyComponentDestruction(entt::registry&, entt::entity entity)
    {
        HierarchyDisconnect({ entity, this });

        // Destroy children (this will destroy their hierarchy components and thus their children, and so on)
        auto& hierarchy = m_Registry.get<HierarchyComponent>(entity);
        auto first = hierarchy.FirstChild;
        auto child = first;
        do {
            auto next = m_Registry.get<HierarchyComponent>(child.m_EnttId).NextSibling;
            child.Destroy();
            child = next;
        } while (child != first);
    }


    void Scene::OnCameraComponentConstruction(entt::registry&, entt::entity entity)
    {
        if (!m_Registry.valid(m_ActiveCamera))
        {
            m_ActiveCamera = entity;
        }
    }


    void Scene::OnCameraComponentDestruction(entt::registry&, entt::entity entity)
    {
        if (entity == m_ActiveCamera)
        {
            // Active camera becomes the first other camera component in the scene,
            // or the null entity if none exists.
            m_ActiveCamera = m_Registry.view<PerspectiveCameraComponent>().front();
        }
    }


    void Scene::HierarchyConnect(Entity entity, Entity parent)
    {
        auto& hierarchy = m_Registry.get<HierarchyComponent>(entity.m_EnttId);
        auto& parentHierarchy = m_Registry.get<HierarchyComponent>(parent.m_EnttId);

        LV_CORE_ASSERT(!hierarchy.Parent && !hierarchy.NextSibling && !hierarchy.PrevSibling, "Hierarchy component has not been disconnected!");

        // Connect to parent
        hierarchy.Parent = parent;
        if (!parentHierarchy.FirstChild)
        {
            parentHierarchy.FirstChild = entity;
        }
        else
        {
            // Connect to siblings
            auto& nextHierarchy = m_Registry.get<HierarchyComponent>(parentHierarchy.FirstChild.m_EnttId);
            if (nextHierarchy.PrevSibling)
            {
                // More than one sibling
                auto& prevHierarchy = m_Registry.get<HierarchyComponent>(nextHierarchy.PrevSibling.m_EnttId);

                hierarchy.NextSibling = { parentHierarchy.FirstChild.m_EnttId, this };
                hierarchy.PrevSibling = { nextHierarchy.PrevSibling.m_EnttId, this };

                nextHierarchy.PrevSibling = entity;
                prevHierarchy.NextSibling = entity;
            }
            else
            {
                // Only one sibling
                hierarchy.PrevSibling = hierarchy.NextSibling = { parentHierarchy.FirstChild.m_EnttId, this };
                nextHierarchy.PrevSibling = nextHierarchy.NextSibling = entity;
            }
        }
    }


    void Scene::HierarchyDisconnect(Entity entity)
    {
        auto& hierarchy = m_Registry.get<HierarchyComponent>(entity.m_EnttId);

        LV_CORE_ASSERT(hierarchy.Parent, "Hierarchy component has not been connected!");

        // Disconnect from parent
        auto& parentHierarchy = m_Registry.get<HierarchyComponent>(hierarchy.Parent.m_EnttId);
        if (parentHierarchy.FirstChild == entity)
        {
            /* No need to check if this entity has siblings - NextSibling is the null entity in this case */
            parentHierarchy.FirstChild = hierarchy.NextSibling;
        }
        hierarchy.Parent = Entity{};

        // Disconnect from siblings
        if (hierarchy.NextSibling)
        {
            auto& nextHierarchy = m_Registry.get<HierarchyComponent>(hierarchy.NextSibling.m_EnttId);
            if (hierarchy.NextSibling == hierarchy.PrevSibling)
            {
                // Only one sibling
                nextHierarchy.NextSibling = nextHierarchy.PrevSibling = Entity{};
            }
            else
            {
                // More than one sibling
                auto& prevHierarchy = m_Registry.get<HierarchyComponent>(hierarchy.PrevSibling.m_EnttId);

                nextHierarchy.PrevSibling = hierarchy.PrevSibling;
                prevHierarchy.NextSibling = hierarchy.NextSibling;
            }

            hierarchy.NextSibling = hierarchy.PrevSibling = Entity{};
        }
    }
}
