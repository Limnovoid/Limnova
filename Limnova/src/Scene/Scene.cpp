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

        m_Registry.on_construct<CameraComponent>().connect<&Scene::OnCameraComponentConstruction>(this);
        m_Registry.on_destroy<CameraComponent>().connect<&Scene::OnCameraComponentDestruction>(this);
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


    void Scene::DestroyEntity(Entity entity)
    {
        m_Registry.destroy(entity.m_EnttId);
    }


    Entity Scene::GetRoot()
    {
        return Entity{ m_Root, this };
    }


    void Scene::SetParent(Entity entity, Entity newParent)
    {
        HierarchyDisconnect(entity);
        HierarchyConnect(entity, newParent);
    }


    Entity Scene::GetParent(Entity entity)
    {
        return m_Registry.get<HierarchyComponent>(entity.m_EnttId).Parent;
    }


    std::vector<Entity> Scene::GetChildren(Entity entity)
    {
        std::vector<Entity> children;
        auto& hierarchy = m_Registry.get<HierarchyComponent>(entity.m_EnttId);
        auto first = hierarchy.FirstChild;
        auto child = first;
        do {
            if (!child) break;
            children.push_back(child);
            child = m_Registry.get<HierarchyComponent>(child.m_EnttId).NextSibling;
        } while (child != first);
        return children;
    }


    void Scene::SetActiveCamera(Entity cameraEntity)
    {
        LV_CORE_ASSERT(cameraEntity.HasComponent<CameraComponent>(), "Attempted to set active camera to a non-camera entity!");
        m_ActiveCamera = cameraEntity.m_EnttId;
    }


    Entity Scene::GetActiveCamera()
    {
        return Entity{ m_ActiveCamera, this };
    }


    void Scene::OnWindowChangeAspect(float aspect)
    {
        m_ViewportAspectRatio = aspect;

        auto view = m_Registry.view<CameraComponent>();
        for (auto entity : view)
        {
            auto& camera = view.get<CameraComponent>(entity);
            if (camera.TieAspectToView)
            {
                camera.SetAspectRatio(m_ViewportAspectRatio);
            }
        }
    }


    void Scene::OnUpdateRuntime(Timestep dT)
    {
        // Scripts
        {
            m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& script)
            {
                // TODO : move to Scene::OnPlay()
                if (!script.Instance)
                {
                    if (script.InstantiateScript)
                    {
                        script.InstantiateScript(&script.Instance);
                        script.Instance->m_Entity = Entity{ entity, this };

                        script.Instance->OnCreate();
                    }
                    else {
                        LV_CORE_WARN("Entity {0} has unbound script component!", (uint32_t)entity);
                    }
                }
                else {
                    script.Instance->OnUpdate(dT);
                }

                // TODO : move to Scene::OnStop()
                /*if (script.Instance)
                {
                    script.DeleteScript(&script.Instance);
                }*/
            });
        }
    }


    void Scene::OnUpdateEditor(Timestep dT)
    {
    }


    void Scene::OnRenderRuntime()
    {
        // Camera
        if (!m_Registry.valid(m_ActiveCamera) || !m_Registry.all_of<CameraComponent>(m_ActiveCamera))
        {
            LV_CORE_WARN("Scene has no active camera - no rendering!");
            return;
        }
        auto [camera, camTransform] = m_Registry.get<CameraComponent, TransformComponent>(m_ActiveCamera);
        camera.Camera.SetView(camTransform.GetTransform().Inverse());

        RenderScene(camera.Camera);
    }


    void Scene::OnRenderEditor(EditorCamera& camera)
    {
        RenderScene(camera.GetCamera());
    }


    void Scene::RenderScene(Camera& camera)
    {
        Renderer2D::BeginScene(camera);

        // Sprites
        {
            auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();

            for (auto entity : view)
            {
                auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);

                Renderer2D::DrawQuad(transform.GetTransform(), sprite.Color, (int)entity);
            }
        }

        // Circles
        {
            auto view = m_Registry.view<TransformComponent, CircleRendererComponent>();
            for (auto entity : view)
            {
                auto [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);

                Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade);
            }
        }

        // Ellipses
        {
            auto view = m_Registry.view<TransformComponent, EllipseRendererComponent>();
            for (auto entity : view)
            {
                auto [transform, ellipse] = view.get<TransformComponent, EllipseRendererComponent>(entity);
                float sx = abs(transform.GetScale().x);
                float sy = abs(transform.GetScale().y);
                float axisRatio = sx > sy ? sx / sy : sy / sx;
                Renderer2D::DrawEllipse(transform.GetTransform(), axisRatio, ellipse.Color, ellipse.Thickness, ellipse.Fade);
            }
        }

        Renderer2D::EndScene();
    }


    void Scene::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowResizeEvent>(LV_BIND_EVENT_FN(Scene::OnWindowResize));

        // Scripts
        m_Registry.view<NativeScriptComponent>().each([&](auto entity, auto& script)
        {
            if (script.Instance) {
                script.Instance->OnEvent(e);
            }
        });
    }


    bool Scene::OnWindowResize(WindowResizeEvent& e)
    {
        OnWindowChangeAspect((float)e.GetWidth() / (float)e.GetHeight());
        return false;
    }


    void Scene::OnHierarchyComponentConstruction(entt::registry&, entt::entity entity)
    {
        HierarchyConnect(Entity{ entity, this }, Entity{ m_Root, this });
    }


    void Scene::OnHierarchyComponentDestruction(entt::registry&, entt::entity entity)
    {
        HierarchyDisconnect(Entity{ entity, this });

        // Destroy children (this will destroy their hierarchy components and thus their children, and so on)
        auto& hierarchy = m_Registry.get<HierarchyComponent>(entity);
        auto first = hierarchy.FirstChild;
        auto child = first;
        do {
            if (!child) break;
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

        auto& camera = m_Registry.get<CameraComponent>(entity);
        if (camera.TieAspectToView && m_ViewportAspectRatio > 0)
        {
            camera.SetAspectRatio(m_ViewportAspectRatio);
        }
    }


    void Scene::OnCameraComponentDestruction(entt::registry&, entt::entity entity)
    {
        if (entity == m_ActiveCamera)
        {
            m_ActiveCamera = entt::null;

            // Find a replacement camera entity
            auto view = m_Registry.view<CameraComponent>();
            for (auto cameraEntity : view)
            {
                /* on_destroy signal is emitted before the component in question is deleted, so
                 * this entity will appear in view and we have to perform this check. */
                if (cameraEntity != entity)
                {
                    m_ActiveCamera = cameraEntity;
                    break;
                }
            }
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

                hierarchy.NextSibling = Entity{ parentHierarchy.FirstChild.m_EnttId, this };
                hierarchy.PrevSibling = Entity{ nextHierarchy.PrevSibling.m_EnttId, this };

                nextHierarchy.PrevSibling = entity;
                prevHierarchy.NextSibling = entity;
            }
            else
            {
                // Only one sibling
                hierarchy.PrevSibling = hierarchy.NextSibling = Entity{ parentHierarchy.FirstChild.m_EnttId, this };
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
        hierarchy.Parent = Entity::Null;

        // Disconnect from siblings
        if (hierarchy.NextSibling)
        {
            auto& nextHierarchy = m_Registry.get<HierarchyComponent>(hierarchy.NextSibling.m_EnttId);
            if (hierarchy.NextSibling == hierarchy.PrevSibling)
            {
                // Only one sibling
                nextHierarchy.NextSibling = nextHierarchy.PrevSibling = Entity::Null;
            }
            else
            {
                // More than one sibling
                auto& prevHierarchy = m_Registry.get<HierarchyComponent>(hierarchy.PrevSibling.m_EnttId);

                nextHierarchy.PrevSibling = hierarchy.PrevSibling;
                prevHierarchy.NextSibling = hierarchy.NextSibling;
            }

            hierarchy.NextSibling = hierarchy.PrevSibling = Entity::Null;
        }
    }
}
