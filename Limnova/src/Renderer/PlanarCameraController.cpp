#include "PlanarCameraController.h"

#include "Core/Input.h"
#include "Core/KeyCodes.h"
#include "Core/MouseButtonCodes.h"

#include "Core/Application.h"


namespace Limnova
{

    PlanarCameraController::PlanarCameraController(const Vector3& position, const Vector3& aimDirection, const float aspectRatio,
        const float nearDistance, const float farDistance)
        : m_Position(position), m_AimDirection(aimDirection), m_AspectRatio(aspectRatio), m_Near(nearDistance), m_Far(farDistance)
    {
        // TODO - dynamic UP vector
    }


    PlanarCameraController::~PlanarCameraController()
    {
    }


    void PlanarCameraController::OnUpdate(Timestep dT)
    {
        LV_PROFILE_FUNCTION();

        if (m_BeingControlled)
        {
            Vector3 cameraMovement;

            // Horizontal movement
            Vector3 cameraLeft = Vector3::Cross({ 0.f, 1.f, 0.f }, m_AimDirection).Normalized();
            if (Input::IsKeyPressed(LV_KEY_A))
            {
                cameraMovement += cameraLeft;
            }
            else if (Input::IsKeyPressed(LV_KEY_D))
            {
                cameraMovement -= cameraLeft;
            }
            Vector3 cameraUp = { 0.f, 1.f, 0.f };
            if (Input::IsKeyPressed(LV_KEY_W))
            {
                cameraMovement += cameraUp;
            }
            else if (Input::IsKeyPressed(LV_KEY_S))
            {
                cameraMovement -= cameraUp;
            }
            cameraMovement.Normalize();

            // Vertical movement
            if (Input::IsKeyPressed(LV_KEY_SPACE))
            {
                cameraMovement.y += 1.f;
            }
            else if (Input::IsKeyPressed(LV_KEY_LEFT_SHIFT))
            {
                cameraMovement.y -= 1.f;
            }

            m_Position += dT * m_CameraMoveSpeed * cameraMovement;

            SetView();
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
        dispatcher.Dispatch<MouseButtonPressedEvent>(LV_BIND_EVENT_FN(PlanarCameraController::OnMouseButtonPressedEvent));
        dispatcher.Dispatch<MouseScrolledEvent>(LV_BIND_EVENT_FN(PlanarCameraController::OnMouseScrolled));
        dispatcher.Dispatch<WindowResizeEvent>(LV_BIND_EVENT_FN(PlanarCameraController::OnWindowResized));
    }


    bool PlanarCameraController::OnMouseButtonPressedEvent(MouseButtonPressedEvent& event)
    {
        LV_PROFILE_FUNCTION();

        switch(event.GetMouseButton())
        {
            case LV_MOUSE_BUTTON_RIGHT:
                if (m_BeingControlled)
                {
                    m_BeingControlled = false;
                    Application::Get().GetWindow().EnableCursor();
                }
                else
                {
                    m_BeingControlled = true;
                    Application::Get().GetWindow().DisableCursor();
                }
                break;
            case LV_MOUSE_BUTTON_MIDDLE:
                m_ZoomLevel = 1.f;
                m_NeedSetProjection = true;
                break;
        }
        return false;
    }


    bool PlanarCameraController::OnMouseScrolled(MouseScrolledEvent& e)
    {
        LV_PROFILE_FUNCTION();

        m_ZoomLevel -= m_ZoomSensitivity * e.GetYOffset();
        m_ZoomLevel = std::clamp(m_ZoomLevel, m_MinZoom, m_MaxZoom);

        m_NeedSetProjection = true;
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
        : PlanarCameraController(position, aimDirection, aspectRatio, nearDistance, farDistance), m_BaseFov(fov),
        m_Camera(m_BaseFov, m_AspectRatio, m_Near, m_Far, m_Position, m_AimDirection, { 0.f, 1.f, 0.f })
    {
        LV_PROFILE_FUNCTION();

        m_MinZoom = 0.25;   // 60 * 0.25 = 15 degrees FoV
        m_MaxZoom = 1.5f;   // 60 * 1.5 = 90 degrees FoV
        m_ZoomSensitivity = 0.05;
    }


    PerspectivePlanarCameraController::~PerspectivePlanarCameraController()
    {
    }


    void PerspectivePlanarCameraController::SetView()
    {
        m_Camera.SetView(m_Position, m_AimDirection, { 0.f, 1.f, 0.f });
    }


    void PerspectivePlanarCameraController::SetProjection()
    {
        m_Camera.SetProjection(m_BaseFov * m_ZoomLevel, m_AspectRatio, m_Near, m_Far);
    }


    // Orthographic ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    OrthographicPlanarCameraController::OrthographicPlanarCameraController(const Vector3& position, const Vector3& aimDirection,
        const float aspectRatio, const float nearDistance, const float farDistance)
        : PlanarCameraController(position, aimDirection, aspectRatio, nearDistance, farDistance),
        m_Camera(m_AspectRatio, m_Near, m_Far, m_Position, m_AimDirection, { 0.f, 1.f, 0.f })
    {
        LV_PROFILE_FUNCTION();

        m_MinZoom = 0.1f;
        m_MaxZoom = 4.f;
        m_ZoomSensitivity = 0.1;
    }


    OrthographicPlanarCameraController::~OrthographicPlanarCameraController()
    {
    }


    void OrthographicPlanarCameraController::SetView()
    {
        m_Camera.SetView(m_Position, m_AimDirection, { 0.f, 1.f, 0.f });
    }


    void OrthographicPlanarCameraController::SetProjection()
    {
        m_Camera.SetProjection(m_AspectRatio, m_ZoomLevel, m_Near, m_Far);
    }

}
