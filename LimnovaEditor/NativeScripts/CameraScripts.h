#pragma once

#include <Limnova.h>


namespace Limnova
{

    class PlanarCameraScript : public NativeScript
    {
    public:
        void OnCreate()
        {
        }

        void OnDestroy()
        {
        }

        void OnUpdate(Timestep dT)
        {
            if (!IsActiveCamera()) return;

            auto& transform = GetComponent<TransformComponent>();
            Vector3 moveDir{ 0.f };
            if (Input::IsKeyPressed(LV_KEY_A))
                moveDir.x = -1.f;
            if (Input::IsKeyPressed(LV_KEY_D))
                moveDir.x = 1.f;
            if (Input::IsKeyPressed(LV_KEY_W))
                moveDir.y = 1.f;
            if (Input::IsKeyPressed(LV_KEY_S))
                moveDir.y = -1.f;
            if (Input::IsKeyPressed(LV_KEY_Q))
                moveDir.z = 1.f;
            if (Input::IsKeyPressed(LV_KEY_E))
                moveDir.z = -1.f;
            static constexpr float moveSpeed = 1.f;
            transform.SetPosition(transform.GetPosition() + (moveDir.Normalized() * moveSpeed * dT));
        }
    };


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
        bool OnMouseScrolledEvent(MouseScrolledEvent& e)
        {
            float deltaDist = e.GetYOffset() * m_ScrollSens;
            m_Distance = std::clamp(m_Distance - deltaDist, m_MinDistance, m_MaxDistance);

            // TODO : update view focus/space if needed, pass updated values to scene

            return false;
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

        const float m_MaxElevation = Radiansf(179.f);
        const float m_MinElevation = Radiansf(-179.f);
        const float m_MouseSens = 0.01f;
        float m_Azimuth = 0.f, m_Elevation = 0.f;

        Vector2 m_MousePos;
    };

}
