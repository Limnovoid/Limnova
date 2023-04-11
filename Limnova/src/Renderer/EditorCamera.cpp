#include "EditorCamera.h"

#include "Core/Input.h"
#include "Core/KeyCodes.h"
#include "Core/MouseButtonCodes.h"
#include "Core/Application.h"


namespace Limnova
{

    EditorCamera::EditorCamera()
        : m_Camera(m_FocusPoint - (Vector3::Forward() * m_FocusDistance), Vector3::Forward(), Vector3::Up())
    {
        std::tie(m_MousePos.x, m_MousePos.y) = Input::GetMousePosition();
    }


    void EditorCamera::OnUpdate(Timestep dT)
    {
        if (m_NeedSetProjection)
        {
            m_Camera.SetPerspectiveProjection(m_Fov, m_AspectRatio, m_NearClip, m_FarClip);
            m_NeedSetProjection = false;
        }

        auto [newMouseX, newMouseY] = Input::GetMousePosition();
        float deltaX = newMouseX - m_MousePos.x;
        float deltaY = newMouseY - m_MousePos.y;
        m_MousePos.x = newMouseX;
        m_MousePos.y = newMouseY;


        // Camera control //

        bool shift = Input::IsKeyPressed(LV_KEY_LEFT_SHIFT);
        bool ctrl = Input::IsKeyPressed(LV_KEY_LEFT_CONTROL);
        bool alt = Input::IsKeyPressed(LV_KEY_LEFT_ALT);

        if (m_Dragging && !shift && !ctrl && !alt)
        {
            // Orbiting aim control

            m_Azimuth -= deltaX * m_OrbitRate; /* Subtract so that left-mouse-drag produces counter-clockwise rotation about UP-axis */
            m_Azimuth = Wrapf(m_Azimuth, 0.f, PI2f);

            m_Elevation += deltaY * m_OrbitRate;
            m_Elevation = std::clamp(m_Elevation, m_MinElevation, m_MaxElevation);
        }

        Quaternion horzOrientation = Quaternion(Vector3::Up(), m_Azimuth);
        Vector3 walkForward = horzOrientation.RotateVector(Vector3::Forward());
        Vector3 walkLeft = horzOrientation.RotateVector(Vector3::Left());

        m_Orientation = horzOrientation * Quaternion(Vector3::Left(), m_Elevation);
        Vector3 aimDirection = m_Orientation.RotateVector(Vector3::Forward());

        Vector3 moveDir{ 0.f };
        if (m_Dragging && shift)
        {
            // Drag distance control:
            m_FocusDistance += deltaY / Application::Get().GetWindow().GetHeight() * m_DragRate;
            m_FocusDistance = std::max(m_FocusDistance, 0.f);
        }
        else if (m_Dragging && ctrl)
        {
            // Drag movement control: vertical and sideways
            moveDir += deltaY * m_Orientation.RotateVector(Vector3::Up()) + deltaX * walkLeft;
        }
        else if (m_Dragging && alt)
        {
            // Drag movement control: forwards and sideways
            moveDir += deltaY * walkForward + deltaX * walkLeft;
        }
        else if (m_IsViewportFocused && !m_IsEntitySelected)
        {
            // Key movement control
            if (Input::IsKeyPressed(LV_KEY_A))
                moveDir += walkLeft;
            if (Input::IsKeyPressed(LV_KEY_D))
                moveDir -= walkLeft;
            if (Input::IsKeyPressed(LV_KEY_W))
                moveDir += walkForward;
            if (Input::IsKeyPressed(LV_KEY_S))
                moveDir -= walkForward;
        }
        m_FocusPoint += moveDir.Normalized() * m_DragRate * dT;

        m_Camera.SetView(m_FocusPoint - (aimDirection * m_FocusDistance), aimDirection, Vector3::Up());
    }


    void EditorCamera::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseButtonPressedEvent>(LV_BIND_EVENT_FN(EditorCamera::OnMouseButtonPressed));
        dispatcher.Dispatch<MouseButtonReleasedEvent>(LV_BIND_EVENT_FN(EditorCamera::OnMouseButtonReleased));
        dispatcher.Dispatch<MouseScrolledEvent>(LV_BIND_EVENT_FN(EditorCamera::OnMouseScrolled));
        dispatcher.Dispatch<WindowResizeEvent>(LV_BIND_EVENT_FN(EditorCamera::OnWindowResized));
    }


    bool EditorCamera::OnMouseButtonPressed(MouseButtonPressedEvent& e)
    {
        if (e.GetMouseButton() == LV_MOUSE_BUTTON_RIGHT)
        {
            if (m_IsViewportHovered) {
                m_Dragging = true;
            }
        }
        return false;
    }


    bool EditorCamera::OnMouseButtonReleased(MouseButtonReleasedEvent& e)
    {
        if (e.GetMouseButton() == LV_MOUSE_BUTTON_RIGHT)
        {
            m_Dragging = false;
        }
        return false;
    }


    bool EditorCamera::OnMouseScrolled(MouseScrolledEvent& e)
    {
        if (m_IsViewportHovered) {
            m_FocusDistance -= m_ScrollRate * e.GetYOffset();
            m_FocusDistance = std::max(m_FocusDistance, 0.f);
        }
        return false;
    }


    bool EditorCamera::OnWindowResized(WindowResizeEvent& e)
    {
        if (e.GetWidth() > 0 && e.GetHeight() > 0)
        {
            m_AspectRatio = (float)e.GetWidth() / (float)e.GetHeight();
            m_NeedSetProjection = true;
        }
        return false;
    }

}
