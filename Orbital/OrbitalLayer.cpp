#include "OrbitalLayer.h"


namespace Limnova
{

    OrbitalLayer::OrbitalLayer()
    {
    }


    void OrbitalLayer::OnAttach()
    {
        auto camera = m_Scene.CreateEntity("Camera");
        {
            camera.AddComponent<PerspectiveCameraComponent>();

            class OrbitalCameraScript : public NativeScript
            {
            public:
                void OnCreate()
                {
                    std::tie(m_MousePos.x, m_MousePos.y) = Input::GetMousePosition();
                }

                void OnDestroy()
                {
                }

                void OnUpdate(Timestep dT)
                {
                    auto [newMouseX, newMouseY] = Input::GetMousePosition();
                    float deltaX = newMouseX - m_MousePos.x;
                    float deltaY = newMouseY - m_MousePos.y;
                    m_MousePos.x = newMouseX;
                    m_MousePos.y = newMouseY;

                    if (!IsActiveCamera()) return;

                    if (Input::IsMouseButtonPressed(LV_MOUSE_BUTTON_RIGHT))
                    {
                        m_Azimuth -= deltaX * m_MouseSens; /* Subtract so that left-mouse-drag produces counter-clockwise rotation about UP-axis */
                        m_Azimuth = Wrapf(m_Azimuth, 0.f, PI2f);

                        m_Elevation += deltaY * m_MouseSens;
                        m_Elevation = std::clamp(m_Elevation, m_MinElevation, m_MaxElevation);
                    }

                    Quaternion horzOrientation = Quaternion(Vector3::Up(), m_Azimuth);
                    Vector3 walkForward = horzOrientation.RotateVector(Vector3::Forward());
                    Vector3 walkLeft = horzOrientation.RotateVector(Vector3::Left());

                    Quaternion orientation = horzOrientation * Quaternion(Vector3::Left(), m_Elevation);
                    Vector3 aimDirection = orientation.RotateVector(Vector3::Forward());

                    Vector3 moveDir{ 0.f };
                    if (Input::IsKeyPressed(LV_KEY_A))
                        moveDir += walkLeft;
                    if (Input::IsKeyPressed(LV_KEY_D))
                        moveDir -= walkLeft;
                    if (Input::IsKeyPressed(LV_KEY_W))
                        moveDir += walkForward;
                    if (Input::IsKeyPressed(LV_KEY_S))
                        moveDir -= walkForward;
                    m_FocusOffset += moveDir.Normalized() * m_OffsetSpeed * dT;

                    auto& transform = GetComponent<TransformComponent>();
                    transform.SetPosition(m_FocusOffset - (aimDirection * m_Distance));
                    transform.SetOrientation(orientation);
                }

                void OnEvent(Event& e)
                {
                    EventDispatcher dispatcher{ e };
                    dispatcher.Dispatch<MouseScrolledEvent>(LV_BIND_EVENT_FN(OrbitalCameraScript::OnMouseScrolledEvent));
                }
            private:
                //Entity m_Focused;
                //Entity m_ViewFocused, m_ViewSpace;
                //uint32_t m_FocusViewLevel;
                Vector3 m_FocusOffset = { 0.f };
                const float m_OffsetSpeed = 0.1f;

                const float m_MaxDistance = 1.1f; // TODO : set to influence escape distance
                const float m_MinDistance = 0.1f; // TODO : get from focused entity influence/scaling space radius
                const float m_ScrollSens = 0.1f;
                float m_Distance = 1.f;

                const float m_MaxElevation = PIover2f * 0.9f;
                const float m_MinElevation = -m_MaxElevation;
                const float m_MouseSens = 0.01f;
                float m_Azimuth = 0.f, m_Elevation = 0.f;//0.5f * m_MaxElevation;

                Vector2 m_MousePos;
            private:
                bool OnMouseScrolledEvent(MouseScrolledEvent& e)
                {
                    float deltaDist = e.GetYOffset() * m_ScrollSens;
                    m_Distance = std::clamp(m_Distance - deltaDist, m_MinDistance, m_MaxDistance);

                    // TODO : update view focus/space if needed, pass updated values to scene

                    return false;
                }
            };
            camera.AddComponent<NativeScriptComponent>().Bind<OrbitalCameraScript>();
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
