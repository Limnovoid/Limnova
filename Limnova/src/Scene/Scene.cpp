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

        // Signals
        m_Registry.on_destroy<HierarchyComponent>().connect<&Scene::OnHierarchyComponentDestruction>(this);

        m_Registry.on_construct<PerspectiveCameraComponent>().connect<&Scene::OnCameraComponentConstruction>(this);
        m_Registry.on_destroy<PerspectiveCameraComponent>().connect<&Scene::OnCameraComponentDestruction>(this);
    }


    Entity Scene::CreateEntity(const std::string& name)
    {
        Entity entity{ m_Registry.create(), this };
        auto& tag = entity.AddComponent<TagComponent>();
        tag.Tag = name.empty() ? "UnnamedEntity" : name;
        entity.AddComponent<TransformComponent>();
        entity.AddComponent<HierarchyComponent>();
        return entity;
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
            // No active camera - no rendering!
            return;
        }

        auto [camera, camTransform] = m_Registry.get<PerspectiveCameraComponent, TransformComponent>(m_ActiveCamera);

        camera.Camera.SetView(camTransform.GetTransform());
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
    }


    void Scene::OnHierarchyComponentDestruction(entt::registry&, entt::entity entity)
    {
        auto& hierarchy = m_Registry.get<HierarchyComponent>(entity);

        // Destroy children (this will destroy their hierarchy components and thus their children, and so on)
        auto first = hierarchy.FirstChild;
        auto child = first;
        do {
            auto next = child.GetComponent<HierarchyComponent>().NextSibling;
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

}
