#include "PlanarCameraController.h"

#include "Core/Input.h"
#include "Core/KeyCodes.h"
#include "Core/MouseButtonCodes.h"

#include "Core/Application.h"


namespace Limnova
{

    PlanarCameraController::PlanarCameraController(const Vector3& position, const Vector3& aimDirection, const float aspectRatio,
        const float nearDistance, const float farDistance)
        : m_Camera(position, aimDirection, Vector3::Up()), m_Position(position), m_AimDirection(aimDirection),
        m_AspectRatio(aspectRatio), m_Near(nearDistance), m_Far(farDistance)
    {
        // TODO - dynamic UP vector
    }


    void PlanarCameraController::OnUpdate(Timestep dT)
    {
        LV_PROFILE_FUNCTION();

        if (m_BeingControlled)
        {
            Vector3 cameraMovement;

            // Horizontal movement
            Vector3 cameraLeft = Vector3::Cross({ 0.f, 1.f, 0.f }, m_AimDirection).Normalized();
            if (Input::IsKeyPressed(KEY_A))
            {
                cameraMovement += cameraLeft;
            }
            else if (Input::IsKeyPressed(KEY_D))
            {
                cameraMovement -= cameraLeft;
            }
            Vector3 cameraUp = { 0.f, 1.f, 0.f };
            if (Input::IsKeyPressed(KEY_W))
            {
                cameraMovement += cameraUp;
            }
            else if (Input::IsKeyPressed(KEY_S))
            {
                cameraMovement -= cameraUp;
            }
            cameraMovement.Normalize();

            // Vertical movement
            if (Input::IsKeyPressed(KEY_SPACE))
            {
                cameraMovement.y += 1.f;
            }
            else if (Input::IsKeyPressed(KEY_LEFT_SHIFT))
            {
                cameraMovement.y -= 1.f;
            }

            m_Position += dT * m_CameraMoveSpeed * cameraMovement;

            m_NeedSetView = true;
        }

        if (m_NeedSetView)
        {
            SetView();
            m_NeedSetView = false;
        }

        if (m_NeedSetProjection)
        {
            SetProjection();
            m_NeedSetProjection = false;
        }
    }


    void PlanarCameraController::OnEvent(Event& e)
    {
        LV_PROFILE_FUNCTION();

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseButtonPressedEvent>(LV_BIND_EVENT_FN(PlanarCameraController::OnMouseButtonPressed));
        dispatcher.Dispatch<MouseScrolledEvent>(LV_BIND_EVENT_FN(PlanarCameraController::OnMouseScrolled));
        dispatcher.Dispatch<WindowResizeEvent>(LV_BIND_EVENT_FN(PlanarCameraController::OnWindowResized));
    }


    bool PlanarCameraController::OnMouseButtonPressed(MouseButtonPressedEvent& e)
    {
        LV_PROFILE_FUNCTION();

        switch(e.GetMouseButton())
        {
            case MOUSE_BUTTON_MIDDLE:
                if (m_BeingControlled)
                {
                    m_Position = { 0.f, 0.f, m_Position.z };
                    m_ZoomLevel = 1.f;
                    m_NeedSetProjection = true;
                }
                break;
        }
        return false;
    }


    bool PlanarCameraController::OnMouseScrolled(MouseScrolledEvent& e)
    {
        LV_PROFILE_FUNCTION();

        if (m_BeingControlled)
        {
            m_ZoomLevel -= m_ZoomSensitivity * e.GetYOffset();
            m_ZoomLevel = std::clamp(m_ZoomLevel, m_MinZoom, m_MaxZoom);

            m_NeedSetProjection = true;
        }
        return false;
    }


    bool PlanarCameraController::OnWindowResized(WindowResizeEvent& e)
    {
        LV_PROFILE_FUNCTION();

        m_AspectRatio = (float)e.GetWidth() / (float)e.GetHeight();

        m_NeedSetProjection = true;
        return false;
    }


    // Perspective /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    PerspectivePlanarCameraController::PerspectivePlanarCameraController(const Vector3& position, const Vector3& aimDirection,
        const float aspectRatio, const float nearDistance, const float farDistance, const float fov)
        : PlanarCameraController(position, aimDirection, aspectRatio, nearDistance, farDistance), m_BaseFov(fov)
    {
        LV_PROFILE_FUNCTION();

        m_Camera.SetPerspectiveProjection(m_BaseFov, m_AspectRatio, m_Near, m_Far);

        m_MinZoom = 0.25f;   // 60 * 0.25 = 15 degrees FoV
        m_MaxZoom = 1.5f;   // 60 * 1.5 = 90 degrees FoV
        m_ZoomSensitivity = 0.05f;
    }


    void PerspectivePlanarCameraController::SetView()
    {
        m_Camera.SetView(m_Position, m_AimDirection, { 0.f, 1.f, 0.f });
    }


    void PerspectivePlanarCameraController::SetProjection()
    {
        m_Camera.SetPerspectiveProjection(m_BaseFov * m_ZoomLevel, m_AspectRatio, m_Near, m_Far);
    }


    Vector3 PerspectivePlanarCameraController::GetWorldPos(const Vector2& screenPos, const float viewDepth)
    {
        /* !!! UNTESTED !!! */
        LV_CORE_ASSERT(false, "Called untested function: PerspectivePlanarCameraController::GetWorldPos()!");

        float screenW = Application::Get().GetWindow().GetWidth();
        float screenH = Application::Get().GetWindow().GetHeight();

        float nearH = 2.f * tanf(0.5f * m_BaseFov * m_ZoomLevel) * m_Near;
        float nearW = m_AspectRatio * nearH;

        Vector3 nearPos{ nearW * (screenPos.x - 0.5f * screenW) / screenW, nearH * (0.5f * screenH - screenPos.y) / screenH, m_Near };
        return viewDepth / m_Near * nearPos + m_Position;
    }


    // Orthographic ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    OrthographicPlanarCameraController::OrthographicPlanarCameraController(const Vector3& position, const Vector3& aimDirection,
        const float aspectRatio, const float nearDistance, const float farDistance)
        : PlanarCameraController(position, aimDirection, aspectRatio, nearDistance, farDistance)
    {
        LV_PROFILE_FUNCTION();

        m_Camera.SetOrthographicProjection(m_AspectRatio, m_ZoomLevel, m_Near, m_Far);

        m_MinZoom = 0.1f;
        m_MaxZoom = 4.f;
        m_ZoomSensitivity = 0.1f;
    }


    void OrthographicPlanarCameraController::SetView()
    {
        m_Camera.SetView(m_Position, m_AimDirection, { 0.f, 1.f, 0.f });
    }


    void OrthographicPlanarCameraController::SetProjection()
    {
        m_Camera.SetOrthographicProjection(m_AspectRatio, m_ZoomLevel, m_Near, m_Far);
    }


    Vector2 OrthographicPlanarCameraController::GetWorldXY(const Vector2& screenXY)
    {
        float screenW = Application::Get().GetWindow().GetWidth();
        float screenH = Application::Get().GetWindow().GetHeight();

        float x = 2.f * m_ZoomLevel * (screenXY.x - 0.5f * screenW) / screenW * m_AspectRatio;
        float y = 2.f * m_ZoomLevel * (0.5f * screenH - screenXY.y) / screenH;

        x += m_Position.x;
        y += m_Position.y;

        return { x, y };
    }

}
