#pragma once

#include <entt.hpp>
#include <Core/Timestep.h>
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

        Entity CreateEntity(const std::string& name = std::string());
        void DestroyEntity(Entity entity);

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

        Entity GetRoot();
        void SetParent(Entity entity, Entity newParent);
        Entity GetParent(Entity entity);
        std::vector<Entity> GetChildren(Entity entity);

        virtual void OnUpdateRuntime(Timestep dT);
        virtual void OnUpdateEditor(Timestep dT);
        virtual void OnRenderRuntime();
        virtual void OnRenderEditor(EditorCamera& camera);
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

        /* Helpers */
        void HierarchyConnect(Entity entity, Entity parent);
        void HierarchyDisconnect(Entity entity);
    protected:
        entt::registry m_Registry;

        entt::entity m_Root; // Scene hierarchy

        entt::entity m_ActiveCamera;
        float m_ViewportAspectRatio = 16.f / 9.f;

        friend class Entity;
        friend class SceneHierarchyPanel;
        friend class SceneSerializer;
    };

}
