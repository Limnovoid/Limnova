#include "Scene.h"

#include "Components.h"
#include "Entity.h"

#include <Renderer/Renderer2D.h>


namespace Limnova
{

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
    }


    Scene::~Scene()
    {

    }


    Entity Scene::CreateEntity(const std::string& name)
    {
        Entity entity{ m_Registry.create(), this };
        entity.AddComponent<TransformComponent>();
        auto& tag = entity.AddComponent<TagComponent>();
        tag.Tag = name.empty() ? "UnnamedEntity" : name;
        return entity;
    }


    void Scene::OnUpdate(Timestep dT)
    {
        auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
        for (auto entity : view)
        {
            auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);

            Renderer2D::DrawQuad(transform, sprite.Color);
        }
    }

}
