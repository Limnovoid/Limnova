#pragma once

#include "Renderer/Camera.h"

#include "Core/Timestep.h"
#include "Events/ApplicationEvent.h"
#include "Events/MouseEvent.h"
#include "Math/Math.h"


namespace Limnova
{

    class EditorCamera
    {
    public:
        EditorCamera();

        void OnUpdate(Timestep dT);
        void OnEvent(Event& e);

        void SetAspect(const float aspect) { m_AspectRatio = aspect; m_NeedSetProjection = true; }

        void SetControl(bool viewportHovered, bool viewportFocused, bool entitySelected)
        {
            m_IsViewportHovered = viewportHovered;
            m_IsViewportFocused = viewportFocused;
            m_IsEntitySelected = entitySelected;
        }

        void SetAzimuth(float azimuth) { m_Azimuth = azimuth; }
        void SetElevation(float elevation) { m_Elevation = elevation; }
        void SetDistance(float distance) { m_FocusDistance = distance; }

        const Quaternion& GetOrientation() { return m_Orientation; }

        Camera& GetCamera() { return m_Camera; }
        const Camera& GetCamera() const { return m_Camera; }
    private:
        bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
        bool OnMouseButtonReleased(MouseButtonReleasedEvent& e);
        bool OnMouseScrolled(MouseScrolledEvent& e);
        bool OnWindowResized(WindowResizeEvent& e);
    private:
        Camera m_Camera;

        // TODO : orthographic option
        float m_Fov = Radiansf(80.f);
        float m_AspectRatio = 16.f / 9.f;
        float m_NearClip = 0.01f, m_FarClip = 1000.f;
        bool m_NeedSetProjection = true;

        bool m_Dragging = false;
        bool m_IsViewportHovered = false, m_IsViewportFocused = false;
        bool m_IsEntitySelected = false;

        Vector3 m_FocusPoint = { 0.f };
        float m_FocusDistance = 2.f;

        float m_Azimuth = 0.f, m_Elevation = 0.f;
        const float m_MaxElevation = Radiansf(89.f);
        const float m_MinElevation = Radiansf(-89.f);
        Quaternion m_Orientation = Quaternion::Unit();

        float m_OrbitRate = 0.01f;
        float m_DragRate = 2.f;
        float m_ScrollRate = 0.1f;

        Vector2 m_MousePos;
    };

}
