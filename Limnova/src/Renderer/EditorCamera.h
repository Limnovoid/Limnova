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

        inline void SetAspect(const float aspect) { m_AspectRatio = aspect; m_NeedSetProjection = true; }

        inline void SetControl(bool viewportHovered, bool viewportFocused, bool entitySelected)
        {
            m_IsViewportHovered = viewportHovered;
            m_IsViewportFocused = viewportFocused;
            m_IsEntitySelected = entitySelected;
        }

        inline void SetAzimuth(float azimuth) { m_Azimuth = azimuth; }
        inline void SetElevation(float elevation) { m_Elevation = elevation; }
        inline void SetFocus(Vector3 const& focusPoint) { m_FocusPoint = focusPoint; }
        inline void SetDistance(float distance) { m_FocusDistance = distance; UpdateProportionalScrollRate(); }

        inline Vector3 const& GetFocus() { return m_FocusPoint; }
        inline float GetDistance() { return m_FocusDistance; }
        inline const Quaternion& GetOrientation() { return m_Orientation; }

        inline Camera& GetCamera() { return m_Camera; }
        inline const Camera& GetCamera() const { return m_Camera; }
    private:
        bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
        bool OnMouseButtonReleased(MouseButtonReleasedEvent& e);
        bool OnMouseScrolled(MouseScrolledEvent& e);
        bool OnWindowResized(WindowResizeEvent& e);

        inline void UpdateProportionalScrollRate()
        {
            static const float kMaxScrollRate;

            m_ScrollRate = 0.1f * m_FocusDistance;
        }
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
        float m_ScrollRate; /* initialized/controlled by UpdateProportionalScrollRate() */

        Vector2 m_MousePos;
    };

}
