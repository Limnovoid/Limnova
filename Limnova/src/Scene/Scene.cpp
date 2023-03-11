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
        m_Registry.on_construct<PerspectiveCameraComponent>().connect<&Scene::OnCameraComponentConstruction>(this);
        m_Registry.on_destroy<PerspectiveCameraComponent>().connect<&Scene::OnCameraComponentDestruction>(this);
    }


    Entity Scene::CreateEntity(const std::string& name)
    {
        Entity entity{ m_Registry.create(), this };
        entity.AddComponent<TransformComponent>();
        auto& tag = entity.AddComponent<TagComponent>();
        tag.Tag = name.empty() ? "UnnamedEntity" : name;
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


    void Scene::OnUpdate(Timestep dT)
    {
        if (!m_Registry.valid(m_ActiveCamera))
        {
            // No active camera - no rendering!
            return;
        }

        auto [camera, camTransform] = m_Registry.get<PerspectiveCameraComponent, TransformComponent>(m_ActiveCamera);

        camera.Camera.SetView(glm::inverse(camTransform.Transform));
        Renderer2D::BeginScene(camera.Camera);

        // Sprites
        {
            auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
            for (auto entity : view)
            {
                auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);

                Renderer2D::DrawQuad(transform, sprite.Color);
            }
        }

        Renderer2D::EndScene();
    }


    void Scene::OnWindowResize(uint32_t width, uint32_t height)
    {
        auto view = m_Registry.view<PerspectiveCameraComponent>();
        for (auto entity : view)
        {
            auto& camera = view.get<PerspectiveCameraComponent>(entity);
            if (camera.TieAspectToView)
            {
                camera.SetAspect((float)width / (float)height);
            }
        }
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
