#include "OrbitalLayer.h"


namespace Limnova
{

    OrbitalLayer::OrbitalLayer()
    {
    }


    void OrbitalLayer::OnAttach()
    {
        m_Camera = m_Scene.CreateEntity("Camera");
        {
            m_Camera.AddComponent<PerspectiveCameraComponent>();
            auto& transform = m_Camera.GetComponent<TransformComponent>();
            m_CameraPos = { 0.f, 0.f, 2.f };
            transform.Set({ 1.f }, m_CameraPos);
        }

        auto root = m_Scene.GetRoot();
        {
            root.AddComponent<SpriteRendererComponent>(Vector4{ 1.f, 1.f, 0.9f, 1.f });
            root.GetComponent<TransformComponent>().SetScale({ 0.05f, 0.05f, 0.f });
        }

        auto orbital0 = m_Scene.CreateEntity("Orbital 0");
        {
            orbital0.AddComponent<OrbitalComponent>();
            orbital0.AddComponent<SpriteRendererComponent>(Vector4{ 1.f, 0.3f, 0.2f, 1.f });
            auto& transform = orbital0.GetComponent<TransformComponent>();
            transform.SetPosition({ 0.9f, 0.f, 0.f });
            transform.SetScale({ 0.01f, 0.01f, 0.f });
        }

        auto orbital1 = m_Scene.CreateEntity("Orbital 1");
        {
            orbital1.AddComponent<OrbitalComponent>();
            orbital1.AddComponent<SpriteRendererComponent>(Vector4{ 0.3f, 0.2f, 1.f, 1.f });
            auto& transform = orbital1.GetComponent<TransformComponent>();
            transform.SetPosition({ 0.f, 0.5f, 0.f });
            transform.SetScale({ 0.01f, 0.01f, 0.f });
        }
    }


    void OrbitalLayer::OnDetach()
    {
    }


    void OrbitalLayer::OnUpdate(Timestep dT)
    {
        auto orbitalEntities = m_Scene.GetEntitiesByComponents<OrbitalComponent>();
        for (auto entity : orbitalEntities)
        {
            auto& orbital = entity.GetComponent<OrbitalComponent>();
            /* do orbital stuff */
        }

        RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.f });
        RenderCommand::Clear();
        m_Scene.OnUpdate(dT);
    }


    void OrbitalLayer::OnImGuiRender()
    {
        ImGui::Begin("Scene Properties");
        {
            auto [tag, transform] = m_Camera.GetComponents<TagComponent, TransformComponent>();
            if (ImGui::DragFloat3(tag.Tag.c_str(), (float*)&m_CameraPos, 0.01f))
            {
                transform.SetPosition(m_CameraPos);
            }
        }
        ImGui::Separator();
        std::vector<Entity> orbitalEntities = m_Scene.GetEntitiesByComponents<OrbitalComponent>();
        for (auto entity : orbitalEntities)
        {
            auto [orbital, sprite, tag] = entity.GetComponents<OrbitalComponent, SpriteRendererComponent, TagComponent>();
            /* orbital debug info */
            ImGui::ColorEdit4(tag.Tag.c_str(), glm::value_ptr(*(glm::vec4*)&sprite.Color));
        }
        ImGui::End(); // Scene Properties

        ImGui::Begin("Hierarchy");
        {
            HierarchyNode(m_Scene.GetRoot());
        }
        ImGui::End(); // Hierarchy

        //bool showdemo = true;
        //ImGui::ShowDemoWindow(&showdemo);
    }


    void OrbitalLayer::OnEvent(Event& e)
    {
        m_Scene.OnEvent(e);
    }


    void OrbitalLayer::HierarchyNode(Entity entity)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        auto children = m_Scene.GetChildren(entity);
        if (children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;
        if (ImGui::TreeNodeEx(entity.GetComponent<TagComponent>().Tag.c_str(), flags))
        {
            for (auto child : children)
            {
                HierarchyNode(child);
            }
            ImGui::TreePop();
        }
    }

}
