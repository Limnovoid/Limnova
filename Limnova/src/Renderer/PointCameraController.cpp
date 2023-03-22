#include "PointCameraController.h"

#include "Core/Input.h"
#include "Core/KeyCodes.h"
#include "Core/MouseButtonCodes.h"

#include "Core/Application.h"


namespace Limnova
{

    PointCameraController::PointCameraController(const Vector3& position, const Vector3& aimDirection, const float aspectRatio,
        const float nearDistance, const float farDistance)
        : m_Camera(position, aimDirection, Vector3::Up()), m_Position(position), m_AimDirection(aimDirection),
        m_AspectRatio(aspectRatio), m_Near(nearDistance), m_Far(farDistance)
    {
        std::tie(m_MouseX, m_MouseY) = Input::GetMousePosition();

        // TODO - dynamic UP vector
    }


    PointCameraController::~PointCameraController()
    {
    }


    void PointCameraController::OnUpdate(Timestep dT)
    {
        LV_PROFILE_FUNCTION();

        auto [mouseX, mouseY] = Input::GetMousePosition();
        float deltaMouseX = mouseX - m_MouseX;
        float deltaMouseY = mouseY - m_MouseY;
        m_MouseX = mouseX;
        m_MouseY = mouseY;
        if (m_BeingControlled)
        {
            // Wrap azimuth around [0,360]
            float scaledSensitivity = m_MouseSensitivity * m_ZoomLevel;
            m_CameraAzimuth -= scaledSensitivity * deltaMouseX;
            if (m_CameraAzimuth >= 360.f) m_CameraAzimuth -= 360.f;
            if (m_CameraAzimuth < 0.f) m_CameraAzimuth += 360.f;

            // Clamp elevation to [min,max] - prevents invalid UP vector in
            // projection matrix calculation
            m_CameraElevation += scaledSensitivity * deltaMouseY;
            m_CameraElevation = std::clamp(m_CameraElevation, m_MinElevation, m_MaxElevation);

            // Vertical aim: rotate default aim direction (0,0,-1) around default tilt axis (-1,0,0)
            m_AimDirection = Rotate({ 0.f, 0.f,-1.f }, {-1.f, 0.f, 0.f }, glm::radians(m_CameraElevation));
            // Horizontal aim: rotate tilted aim direction around default up axis (0,1,0)
            m_AimDirection = Rotate(m_AimDirection, { 0.f, 1.f, 0.f }, glm::radians(m_CameraAzimuth));
            m_AimDirection.Normalize();

            Vector3 cameraMovement;

            // Horizontal movement
            Vector3 cameraHorzLeft = Vector3::Cross({ 0.f, 1.f, 0.f }, m_AimDirection).Normalized();
            if (Input::IsKeyPressed(LV_KEY_A))
            {
                cameraMovement += cameraHorzLeft;
            }
            else if (Input::IsKeyPressed(LV_KEY_D))
            {
                cameraMovement -= cameraHorzLeft;
            }
            Vector3 cameraHorzForward = cameraHorzLeft.Cross({ 0.f, 1.f, 0.f });
            if (Input::IsKeyPressed(LV_KEY_W))
            {
                cameraMovement += cameraHorzForward;
            }
            else if (Input::IsKeyPressed(LV_KEY_S))
            {
                cameraMovement -= cameraHorzForward;
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


    void PointCameraController::OnEvent(Event& e)
    {
        LV_PROFILE_FUNCTION();

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseButtonPressedEvent>(LV_BIND_EVENT_FN(PointCameraController::OnMouseButtonPressed));
        dispatcher.Dispatch<MouseScrolledEvent>(LV_BIND_EVENT_FN(PointCameraController::OnMouseScrolled));
        dispatcher.Dispatch<WindowResizeEvent>(LV_BIND_EVENT_FN(PointCameraController::OnWindowResized));
    }


    bool PointCameraController::OnMouseButtonPressed(MouseButtonPressedEvent& e)
    {
        LV_PROFILE_FUNCTION();

        switch(e.GetMouseButton())
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
                if (m_BeingControlled)
                {
                    m_ZoomLevel = 1.f;
                    m_NeedSetProjection = true;
                }
                break;
        }
        return false;
    }


    bool PointCameraController::OnMouseScrolled(MouseScrolledEvent& e)
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


    bool PointCameraController::OnWindowResized(WindowResizeEvent& e)
    {
        LV_PROFILE_FUNCTION();

        m_AspectRatio = (float)e.GetWidth() / (float)e.GetHeight();

        m_NeedSetProjection = true;
        return false;
    }


    // Perspective /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    PerspectivePointCameraController::PerspectivePointCameraController(const Vector3& position, const Vector3& aimDirection,
        const float aspectRatio, const float nearDistance, const float farDistance, const float fov)
        : PointCameraController(position, aimDirection, aspectRatio, nearDistance, farDistance), m_BaseFov(fov)
    {
        LV_PROFILE_FUNCTION();

        m_Camera.SetPerspectiveProjection(m_BaseFov, m_AspectRatio, m_Near, m_Far);

        m_MinZoom = 0.25;   // 60 * 0.25 = 15 degrees FoV
        m_MaxZoom = 1.5f;   // 60 * 1.5 = 90 degrees FoV
        m_ZoomSensitivity = 0.05;
    }


    PerspectivePointCameraController::~PerspectivePointCameraController()
    {
    }


    void PerspectivePointCameraController::SetView()
    {
        m_Camera.SetView(m_Position, m_AimDirection, { 0.f, 1.f, 0.f });
    }


    void PerspectivePointCameraController::SetProjection()
    {
        m_Camera.SetPerspectiveProjection(m_BaseFov * m_ZoomLevel, m_AspectRatio, m_Near, m_Far);
    }


    // Orthographic ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    OrthographicPointCameraController::OrthographicPointCameraController(const Vector3& position, const Vector3& aimDirection,
        const float aspectRatio, const float nearDistance, const float farDistance)
        : PointCameraController(position, aimDirection, aspectRatio, nearDistance, farDistance)
    {
        LV_PROFILE_FUNCTION();

        m_Camera.SetOrthographicProjection(m_AspectRatio, m_ZoomLevel, m_Near, m_Far);

        m_MinZoom = 0.1f;
        m_MaxZoom = 4.f;
        m_ZoomSensitivity = 0.1;
    }


    OrthographicPointCameraController::~OrthographicPointCameraController()
    {
    }


    void OrthographicPointCameraController::SetView()
    {
        m_Camera.SetView(m_Position, m_AimDirection, { 0.f, 1.f, 0.f });
    }


    void OrthographicPointCameraController::SetProjection()
    {
        m_Camera.SetOrthographicProjection(m_AspectRatio, m_ZoomLevel, m_Near, m_Far);
    }

}
