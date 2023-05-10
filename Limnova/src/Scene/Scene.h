#pragma once

#include "Components.h"

#include <entt.hpp>
#include <Core/Timestep.h>
#include <Core/UUID.h>
#include <Events/Event.h>
#include <Events/ApplicationEvent.h>
#include <Renderer/EditorCamera.h>


namespace Limnova
{

    class Entity;

    class Scene
    {
    public:
        Scene();
        ~Scene() = default;

        static Ref<Scene> Copy(Ref<Scene> scene);

        Entity CreateEntity(const std::string& name = std::string());
        Entity CreateEntityAsChild(Entity parent, const std::string& name = std::string());
        Entity CreateEntityFromUUID(UUID uuid, const std::string& name = std::string(), UUID parent = UUID::Null);
        void DestroyEntity(Entity entity);

        Entity GetEntity(UUID uuid);

        template<typename First, typename... Rest>
        std::vector<Entity> GetEntitiesByComponents()
        {
            std::vector<Entity> entities;
            auto view = m_Registry.view<First, Rest...>();
            for (auto entity : view)
            {
                entities.emplace_back(entity, this);
            }
            return entities;
        }

        void SetActiveCamera(Entity cameraEntity);
        Entity GetActiveCamera();
        void OnWindowChangeAspect(float aspect);

    protected: void SetRootId(UUID id); /* for serializing */
    public:
        Entity GetRoot();

        void SetParent(Entity entity, Entity newParent);
        Entity GetParent(Entity entity);
        std::vector<Entity> GetChildren(Entity entity);
        std::vector<Entity> GetTree(Entity root);

        virtual void OnStartRuntime();
        virtual void OnUpdateRuntime(Timestep dT);
        virtual void OnUpdateEditor(Timestep dT);
        virtual void OnRenderRuntime();
        virtual void OnRenderEditor(EditorCamera& camera);
        virtual void OnStopRuntime();

        virtual void OnEvent(Event& e);
    protected:
        // A call to any member of this function set must come after a call to Renderer2D::BeginScene and before a corresponding call to Renderer2D::EndScene
        //template<typename T>
        //void RenderComponent(const T& component) { /* Do nothing by default */ }

        void RenderScene(Camera& camera, const Quaternion& cameraOrientation);
    private:
        /* Events */
        bool OnWindowResize(WindowResizeEvent& e);

        /* Signals */
        void OnHierarchyComponentConstruction(entt::registry&, entt::entity);
        void OnHierarchyComponentDestruction(entt::registry&, entt::entity);
        void OnCameraComponentConstruction(entt::registry&, entt::entity);
        void OnCameraComponentDestruction(entt::registry&, entt::entity);

    protected:
        /* Helpers */
        template<typename T>
        static void CopyAllOfComponent(entt::registry& dst, entt::registry& src, std::unordered_map<UUID, entt::entity>& dstEntities)
        {
            auto view = src.view<T>();
            for (auto e : view)
            {
                LV_CORE_ASSERT(src.all_of<IDComponent>(e), "All entities must have UUID!");
                UUID uuid = src.get<IDComponent>(e).ID;
                const T& srcComponent = src.get<T>(e);
                LV_CORE_ASSERT(dstEntities.find(uuid) != dstEntities.end(), "Could not find entity with matching ID in the destination scene!");
                dst.emplace_or_replace<T>(dstEntities[uuid], srcComponent);
            }
        }

        void HierarchyConnect(entt::entity entity, entt::entity parent);
        void HierarchyDisconnect(entt::entity entity);

        template<typename T>
        bool HasComponent(entt::entity entity) {
            return m_Registry.all_of<T>(entity);
        }
        template<typename First, typename... Rest>
        bool HasComponents(entt::entity entity) {
            return m_Registry.all_of<First, Rest...>(entity);
        }
        template<typename T, typename... Args>
        T& AddComponent(entt::entity entity, Args&&... args)
        {
            LV_CORE_ASSERT(!HasComponent<T>(entity), "Entity already has component!");
            return m_Registry.emplace<T>(entity, std::forward<Args>(args)...);
        }
        template<typename T>
        T& GetComponent(entt::entity entity) {
            LV_CORE_ASSERT(HasComponent<T>(entity), "Entity does not have component!");
            return m_Registry.get<T>(entity);
        }
        template<typename First, typename... Rest>
        std::tuple<First&, Rest&...> GetComponents(entt::entity entity) {
            LV_CORE_ASSERT(HasComponent<First>(entity), "Entity does not have component!");
            LV_CORE_ASSERT(HasComponents<Rest...>(entity), "Entity does not have component(s)!");
            return m_Registry.get<First, Rest...>(entity);
        }
        bool Valid(entt::entity entity) {
            return m_Registry.valid(entity);
        }
        void Destroy(entt::entity entity) {
            LV_CORE_ASSERT(Valid(entity), "Attempting to destroy invalid entity!");
            m_Registry.destroy(entity);
        }
    protected:
        entt::registry m_Registry;
        std::unordered_map<UUID, entt::entity> m_Entities;

        UUID m_Root = UUID::Null; /* Scene hierarchy root entity */
        UUID m_ActiveCamera = UUID::Null;
        float m_ViewportAspectRatio = 16.f / 9.f;

        friend class Entity;
        friend class SceneHierarchyPanel;
        friend class SceneSerializer;
    };

}
