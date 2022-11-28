#pragma once

#include "Renderer/PerspectiveCamera.h"

#include "Core/Timestep.h"
#include "Events/ApplicationEvent.h"
#include "Events/MouseEvent.h"

#include "Math/Math.h"


namespace Limnova
{

    class PerspectiveCameraController
    {
    public:
        PerspectiveCameraController(const Vector3& position, const Vector3& aimDirection, const float aspectRatio);

        void OnUpdate(Timestep dT);
        void OnEvent(Event& e);

        inline PerspectiveCamera& GetCamera() { return m_Camera; }
        inline const PerspectiveCamera& GetCamera() const { return m_Camera; }

        inline bool IsBeingControlled() { return m_BeingControlled; }
    private:
        bool OnMouseButtonPressedEvent(MouseButtonPressedEvent& event);
        bool OnMouseScrolled(MouseScrolledEvent& e);
        bool OnWindowResized(WindowResizeEvent& e);
    private:
        float m_AspectRatio;
        float m_Fov = glm::radians(60.f);
        float m_Near = 0.1, m_Far = 100.f;
        PerspectiveCamera m_Camera;
        bool m_BeingControlled = false;
        float m_MouseX, m_MouseY;
        float m_MouseSensitivity = 0.1f;
        float m_MinFov = glm::radians(10.f), m_MaxFov = glm::radians(90.f);
        float m_ZoomSensitivity = 0.1f;

        // PerspectiveCamera
        Vector3 m_Position      = { 0.f, 0.f, 0.f };
        Vector3 m_AimDirection  = { 0.f, 0.f,-1.f };
        float m_CameraAzimuth = 0.f, m_CameraElevation = 0.f;
        float m_MinElevation = -85.f, m_MaxElevation = 85.f;
        float m_CameraMoveSpeed = 1.f;
        bool m_NeedSetProjection = false;
    };

}
