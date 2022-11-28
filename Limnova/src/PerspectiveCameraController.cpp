#include "PerspectiveCameraController.h"

#include "Input.h"
#include "KeyCodes.h"
#include "MouseButtonCodes.h"

#include "Application.h"


namespace Limnova
{

    PerspectiveCameraController::PerspectiveCameraController(const Vector3& position, const Vector3& aimDirection, const float aspectRatio)
        : m_AspectRatio(aspectRatio), m_Camera(m_Fov, aspectRatio, m_Near, m_Far, position, aimDirection, { 0.f, 1.f, 0.f }),
        m_Position(position), m_AimDirection(aimDirection)
    {
        std::tie(m_MouseX, m_MouseY) = Input::GetMousePosition();
    }


    void PerspectiveCameraController::OnUpdate(Timestep dT)
    {
        auto [mouseX, mouseY] = Input::GetMousePosition();
        float deltaMouseX = mouseX - m_MouseX;
        float deltaMouseY = mouseY - m_MouseY;
        m_MouseX = mouseX;
        m_MouseY = mouseY;
        if (m_BeingControlled)
        {
            // Wrap azimuth around [0,360]
            float scaledSensitivity = m_MouseSensitivity * m_Fov / glm::radians(60.f);
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

            m_Camera.SetView(m_Position, m_AimDirection, { 0.f, 1.f, 0.f });
        }

        if (m_NeedSetProjection)
        {
            m_Camera.SetProjection(m_Fov, m_AspectRatio, m_Near, m_Far);
            m_NeedSetProjection = false;
        }
    }


    void PerspectiveCameraController::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseButtonPressedEvent>(LV_BIND_EVENT_FN(PerspectiveCameraController::OnMouseButtonPressedEvent));
        dispatcher.Dispatch<MouseScrolledEvent>(LV_BIND_EVENT_FN(PerspectiveCameraController::OnMouseScrolled));
        dispatcher.Dispatch<WindowResizeEvent>(LV_BIND_EVENT_FN(PerspectiveCameraController::OnWindowResized));
    }


    bool PerspectiveCameraController::OnMouseButtonPressedEvent(MouseButtonPressedEvent& event)
    {
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
                m_Fov = glm::radians(60.f);
                m_NeedSetProjection = true;
                break;
        }
        return false;
    }


    bool PerspectiveCameraController::OnMouseScrolled(MouseScrolledEvent& e)
    {
        m_Fov -= m_ZoomSensitivity * e.GetYOffset();
        m_Fov = std::clamp(m_Fov, m_MinFov, m_MaxFov);

        m_NeedSetProjection = true;
        return false;
    }


    bool PerspectiveCameraController::OnWindowResized(WindowResizeEvent& e)
    {
        m_AspectRatio = (float)e.GetWidth() / (float)e.GetHeight();

        m_NeedSetProjection = true;
        return false;
    }

}
