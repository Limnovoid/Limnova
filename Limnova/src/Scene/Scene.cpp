#include "Scene.h"

#include "Entity.h"
#include "Script.h"

#include <Renderer/Renderer2D.h>


namespace Limnova
{

    static constexpr Vector3 kDefaultAim{ 0.f, 0.f, -1.f };


    Scene::Scene()
    {
        // Scene hierarchy
        Entity root = Entity{ m_Registry.create(), this };
        auto& idc = root.AddComponent<IDComponent>();
        m_Entities[idc.ID] = root.m_EnttId;
        m_Root = idc.ID;
        root.AddComponent<TagComponent>("Root");
        root.AddComponent<TransformComponent>();
        root.AddComponent<HierarchyComponent>(); /* All relationships NULL */

        // Signals
        m_Registry.on_destroy<HierarchyComponent>().connect<&Scene::OnHierarchyComponentDestruction>(this);
        /* No on_construct signal for HierarchyComponent: construction dependencies are handled by CreateEntity */

        m_Registry.on_construct<CameraComponent>().connect<&Scene::OnCameraComponentConstruction>(this);
        m_Registry.on_destroy<CameraComponent>().connect<&Scene::OnCameraComponentDestruction>(this);
    }


    Ref<Scene> Scene::Copy(Ref<Scene> scene)
    {
        Ref<Scene> newScene = CreateRef<Scene>();

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

        newScene->CopyAllOfComponent<TransformComponent>(scene->m_Registry);
        newScene->CopyAllOfComponent<HierarchyComponent>(scene->m_Registry);
        newScene->CopyAllOfComponent<CameraComponent>(scene->m_Registry);
        newScene->CopyAllOfComponent<NativeScriptComponent>(scene->m_Registry);
        newScene->CopyAllOfComponent<SpriteRendererComponent>(scene->m_Registry);
        newScene->CopyAllOfComponent<BillboardSpriteRendererComponent>(scene->m_Registry);
        newScene->CopyAllOfComponent<CircleRendererComponent>(scene->m_Registry);
        newScene->CopyAllOfComponent<BillboardCircleRendererComponent>(scene->m_Registry);
        newScene->CopyAllOfComponent<EllipseRendererComponent>(scene->m_Registry);

        return newScene;
    }


    Entity Scene::CreateEntity(const std::string& name)
    {
        return CreateEntityFromUUID(UUID(), name);
    }


    Entity Scene::CreateEntityAsChild(Entity parent, const std::string& name)
    {
        return CreateEntityFromUUID(UUID(), name, parent.GetUUID());
    }


    Entity Scene::CreateEntityFromUUID(UUID uuid, const std::string& name, UUID parent)
    {
        Entity entity{ m_Registry.create(), this };

        /* Essential components:
         * every entity is assumed to have the following components throughout its lifetime: */
        auto& idc = entity.AddComponent<IDComponent>(uuid);
        m_Entities[idc.ID] = entity.m_EnttId;
        entity.AddComponent<TagComponent>(name.empty() ? "Unnamed Entity" : name);
        entity.AddComponent<TransformComponent>();
        entity.AddComponent<HierarchyComponent>();
        HierarchyConnect(entity.m_EnttId, m_Entities[(parent == UUID::Null) ? m_Root : parent]);
        return entity;
    }


    Entity Scene::DuplicateEntity(Entity entity)
    {
        Entity newEntity = CreateEntityAsChild(GetParent(entity), entity.GetName() + " (copy)");

        CopyComponentIfExists<TransformComponent>(newEntity.m_EnttId, entity.m_EnttId);
        /* DO NOT copy HierarchyComponent - the original and copy entities' relationships are necessarily different and are handled by CreateEntity() */
        CopyComponentIfExists<CameraComponent>(newEntity.m_EnttId, entity.m_EnttId);
        CopyComponentIfExists<NativeScriptComponent>(newEntity.m_EnttId, entity.m_EnttId);
        CopyComponentIfExists<SpriteRendererComponent>(newEntity.m_EnttId, entity.m_EnttId);
        CopyComponentIfExists<BillboardSpriteRendererComponent>(newEntity.m_EnttId, entity.m_EnttId);
        CopyComponentIfExists<CircleRendererComponent>(newEntity.m_EnttId, entity.m_EnttId);
        CopyComponentIfExists<BillboardCircleRendererComponent>(newEntity.m_EnttId, entity.m_EnttId);
        CopyComponentIfExists<EllipseRendererComponent>(newEntity.m_EnttId, entity.m_EnttId);

        return newEntity;
    }


    void Scene::DestroyEntity(Entity entity)
    {
        Destroy(entity.m_EnttId);
    }


    Entity Scene::GetEntity(UUID uuid)
    {
        auto it = m_Entities.find(uuid);
        if (it == m_Entities.end()) {
            LV_WARN("Could not find entity with ID {0}!", uuid);
            return Entity::Null;
        }
        return Entity{ it->second, this };
    }


    void Scene::SetRootId(UUID id)
    {
        LV_CORE_ASSERT(m_Entities.find(id) == m_Entities.end(), "Root ID clash!");
        m_Entities.insert({ id, m_Entities.at(m_Root) });
        m_Entities.erase(m_Root);
        m_Root = id;

        auto& idc = GetComponent<IDComponent>(m_Entities.at(m_Root));
        idc.ID = id;

        auto rootChildren = GetChildren({ m_Entities.at(m_Root), this });
        for (auto child : rootChildren) {
            auto& hc = child.GetComponent<HierarchyComponent>();
            hc.Parent = id;
        }
    }


    Entity Scene::GetRoot()
    {
        return Entity{ m_Entities[m_Root], this };
    }


    void Scene::SetParent(Entity entity, Entity newParent)
    {
        HierarchyDisconnect(entity.m_EnttId);
        HierarchyConnect(entity.m_EnttId, newParent.m_EnttId);
    }


    Entity Scene::GetParent(Entity entity)
    {
        return Entity{ m_Entities[ GetComponent<HierarchyComponent>(entity.m_EnttId).Parent ], this };
    }


    std::vector<Entity> Scene::GetChildren(Entity entity)
    {
        std::vector<Entity> children;
        auto& hierarchy = m_Registry.get<HierarchyComponent>(entity.m_EnttId);
        auto first = hierarchy.FirstChild;
        auto child = first;
        do {
            if (child == UUID::Null) break;
            children.push_back(Entity{ m_Entities[child], this });
            child = GetComponent<HierarchyComponent>(m_Entities[child]).NextSibling;
        } while (child != first);
        return children;
    }


    /// <summary>
    /// Ordered by hierarchy level: the children of the root entity will be placed in a continuous sequence at the beginning of the array,
    /// followed by a continuous sequence of each of their children (the root's grandchildren), and so on.
    /// Equivalent to a breadth-first search of all entities in the hierarchy attached to root.
    /// </summary>
    std::vector<Entity> Scene::GetTree(Entity root)
    {
        std::vector<Entity> entities = GetChildren(root);
        size_t numAdded = entities.size();
        do {
            size_t end = entities.size();
            size_t idx = end - numAdded;
            numAdded = 0;
            for (; idx < end; idx++)
            {
                auto children = GetChildren(entities[idx]);
                entities.insert(entities.end(), children.begin(), children.end());
                numAdded += children.size();
            }
        } while (numAdded > 0);
        return entities;
    }


    void Scene::SetActiveCamera(Entity cameraEntity)
    {
        LV_CORE_ASSERT(cameraEntity.HasComponent<CameraComponent>(), "Attempted to set active camera to a non-camera entity!");
        m_ActiveCamera = cameraEntity.GetUUID();
    }


    Entity Scene::GetActiveCamera()
    {
        if (m_ActiveCamera != UUID::Null) {
            return Entity{ m_Entities[m_ActiveCamera], this };
        }
        else {
            return Entity::Null;
        }
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


    void Scene::OnStartRuntime()
    {
        // Scripts
        m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& script)
        {
            if (script.InstantiateScript) {
                script.InstantiateScript(&script.Instance);
                script.Instance->m_Entity = Entity{ entity, this };

                script.Instance->OnCreate();
            }
            else {
                LV_CORE_WARN("Entity {0} has unbound script component!", (uint32_t)entity);
            }
        });
    }


    void Scene::OnUpdateRuntime(Timestep dT)
    {
        // Scripts
        m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& script)
        {
            if (script.Instance) {
                script.Instance->OnUpdate(dT);
            }
        });
    }


    void Scene::OnUpdateEditor(Timestep dT)
    {
    }


    void Scene::OnRenderRuntime()
    {
        if (m_ActiveCamera == UUID::Null) {
            LV_CORE_WARN("Scene has no active camera - no rendering!");
            return;
        }
        auto cameraEntity = m_Entities[m_ActiveCamera];
        auto [camera, camTransform] = GetComponents<CameraComponent, TransformComponent>(cameraEntity);
        camera.Camera.SetView(camTransform.GetTransform().Inverse());

        RenderScene(camera.Camera, camTransform.GetOrientation());
    }


    void Scene::OnRenderEditor(EditorCamera& camera)
    {
        RenderScene(camera.GetCamera(), camera.GetOrientation());
    }


    void Scene::RenderScene(Camera& camera, const Quaternion& cameraOrientation)
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

        // Billboard sprites
        {
            auto view = m_Registry.view<TransformComponent, BillboardSpriteRendererComponent>();

            for (auto entity : view)
            {
                auto [tc, bsrc] = view.get<TransformComponent, BillboardSpriteRendererComponent>(entity);

                Matrix4 billboardTransform = glm::translate(glm::mat4(1.f), (glm::vec3)(tc.GetPosition()));
                billboardTransform = billboardTransform * Matrix4(cameraOrientation);
                billboardTransform = glm::scale((glm::mat4)billboardTransform, (glm::vec3)(tc.GetScale()));
                Renderer2D::DrawQuad(billboardTransform, bsrc.Color, (int)entity);
            }
        }

        // Circles
        {
            auto view = m_Registry.view<TransformComponent, CircleRendererComponent>();
            for (auto entity : view)
            {
                auto [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);

                Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade, (int)entity);
            }
        }

        // Billboard circles
        {
            auto view = m_Registry.view<TransformComponent, BillboardCircleRendererComponent>();

            for (auto entity : view)
            {
                auto [tc, bcrc] = view.get<TransformComponent, BillboardCircleRendererComponent>(entity);

                Matrix4 billboardTransform = glm::translate(glm::mat4(1.f), (glm::vec3)(tc.GetPosition()));
                billboardTransform = billboardTransform * Matrix4(cameraOrientation);
                billboardTransform = glm::scale((glm::mat4)billboardTransform, (glm::vec3)(tc.GetScale()));
                Renderer2D::DrawCircle(billboardTransform, bcrc.Color, bcrc.Thickness, bcrc.Fade, (int)entity);
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
                Renderer2D::DrawEllipse(transform.GetTransform(), axisRatio, ellipse.Color, ellipse.Thickness, ellipse.Fade, (int)entity);
            }
        }

        Renderer2D::EndScene();
    }


    void Scene::OnStopRuntime()
    {
        // Scripts
        {
            m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& script)
            {
                if (script.Instance)
                {
                    script.DeleteScript(&script.Instance);
                }
            });
        }
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
        HierarchyConnect(entity, m_Entities[m_Root]);
    }


    void Scene::OnHierarchyComponentDestruction(entt::registry&, entt::entity entity)
    {
        HierarchyDisconnect(entity);

        // Destroy children (this will destroy their hierarchy components and thus their children, and so on)
        auto& hierarchy = GetComponent<HierarchyComponent>(entity);
        auto first = hierarchy.FirstChild;
        auto child = first;
        do {
            if (!child) break;
            auto next = GetComponent<HierarchyComponent>(m_Entities[child]).NextSibling;
            Destroy(m_Entities[child]);
            child = next;
        } while (child != first);
    }


    void Scene::OnCameraComponentConstruction(entt::registry&, entt::entity entity)
    {
        if (m_ActiveCamera == UUID::Null)
        {
            m_ActiveCamera = m_Registry.get<IDComponent>(entity).ID;
        }

        auto& camera = m_Registry.get<CameraComponent>(entity);
        if (camera.TieAspectToView && m_ViewportAspectRatio > 0)
        {
            camera.SetAspectRatio(m_ViewportAspectRatio);
        }
    }


    void Scene::OnCameraComponentDestruction(entt::registry&, entt::entity entity)
    {
        if (m_Registry.get<IDComponent>(entity).ID == m_ActiveCamera)
        {
            m_ActiveCamera = UUID::Null;

            // Find a replacement camera entity
            auto view = m_Registry.view<CameraComponent>();
            for (auto cameraEntity : view)
            {
                /* on_destroy signal is emitted before the component in question is deleted, so
                 * this entity will appear in the view and we have to perform this check. */
                if (cameraEntity != entity)
                {
                    m_ActiveCamera = m_Registry.get<IDComponent>(cameraEntity).ID;
                    break;
                }
            }
        }
    }


    void Scene::HierarchyConnect(entt::entity entity, entt::entity parent)
    {
        auto& id = GetComponent<IDComponent>(entity);
        auto& hierarchy = GetComponent<HierarchyComponent>(entity);
        auto& parentId = GetComponent<IDComponent>(parent);
        auto& parentHierarchy = GetComponent<HierarchyComponent>(parent);

        LV_CORE_ASSERT(hierarchy.Parent == UUID::Null && hierarchy.NextSibling == UUID::Null && hierarchy.PrevSibling == UUID::Null, "Hierarchy component has not been disconnected!");

        // Connect to parent
        hierarchy.Parent = parentId;
        if (parentHierarchy.FirstChild == UUID::Null)
        {
            parentHierarchy.FirstChild = id;
        }
        else
        {
            // Connect to siblings
            auto& nextHierarchy = GetComponent<HierarchyComponent>(m_Entities[parentHierarchy.FirstChild]);
            if (nextHierarchy.PrevSibling != UUID::Null)
            {
                // More than one sibling
                auto& prevHierarchy = GetComponent<HierarchyComponent>(m_Entities[nextHierarchy.PrevSibling]);

                hierarchy.NextSibling = parentHierarchy.FirstChild;
                hierarchy.PrevSibling = nextHierarchy.PrevSibling;

                nextHierarchy.PrevSibling = id;
                prevHierarchy.NextSibling = id;
            }
            else
            {
                // Only one sibling
                hierarchy.PrevSibling = hierarchy.NextSibling = parentHierarchy.FirstChild;
                nextHierarchy.PrevSibling = nextHierarchy.NextSibling = id;
            }
        }
    }


    void Scene::HierarchyDisconnect(entt::entity entity)
    {
        auto& id = GetComponent<IDComponent>(entity);
        auto& hierarchy = GetComponent<HierarchyComponent>(entity);

        LV_CORE_ASSERT(hierarchy.Parent != UUID::Null, "Hierarchy component has not been connected!");

        // Disconnect from parent
        auto& parentHierarchy = GetComponent<HierarchyComponent>(m_Entities[hierarchy.Parent]);
        if (parentHierarchy.FirstChild == id.ID)
        {
            /* No need to check if this entity has siblings - NextSibling is the null entity in this case */
            parentHierarchy.FirstChild = hierarchy.NextSibling;
        }
        hierarchy.Parent = UUID::Null;

        // Disconnect from siblings
        if (hierarchy.NextSibling != UUID::Null)
        {
            auto& nextHierarchy = GetComponent<HierarchyComponent>(m_Entities[hierarchy.NextSibling]);
            if (hierarchy.NextSibling == hierarchy.PrevSibling)
            {
                // Only one sibling
                nextHierarchy.NextSibling = nextHierarchy.PrevSibling = UUID::Null;
            }
            else
            {
                // More than one sibling
                auto& prevHierarchy = GetComponent<HierarchyComponent>(m_Entities[hierarchy.PrevSibling]);

                nextHierarchy.PrevSibling = hierarchy.PrevSibling;
                prevHierarchy.NextSibling = hierarchy.NextSibling;
            }

            hierarchy.NextSibling = hierarchy.PrevSibling = UUID::Null;
        }
    }
}
