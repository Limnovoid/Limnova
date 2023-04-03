#pragma once

#include "Renderer/Camera.h"

#include "Core/Timestep.h"
#include "Events/ApplicationEvent.h"
#include "Events/MouseEvent.h"

#include "Math/Math.h"


namespace Limnova
{

    class PlanarCameraController
    {
    protected:
        PlanarCameraController(const Vector3& position, const Vector3& aimDirection, const float aspectRatio,
            const float nearDistance, const float farDistance);
    public:
        ~PlanarCameraController() = default;

        void OnUpdate(Timestep dT);
        void OnEvent(Event& e);

        virtual Camera& GetCamera() { return m_Camera; }
        virtual const Camera& GetCamera() const { return m_Camera; }

        void SetAspect(const float aspect) { m_AspectRatio = aspect; m_NeedSetProjection = true; }

        void SetControlled(const bool isControlled) { m_BeingControlled = isControlled; }

        void SetZoom(const float zoom) { m_ZoomLevel = std::clamp(zoom, m_MinZoom, m_MaxZoom); m_NeedSetProjection = true; }
        void SetZoomLimits(const float minZoom, const float maxZoom) { m_MinZoom = minZoom; m_MaxZoom = maxZoom; SetZoom(m_ZoomLevel); }
        void SetZoomSensitivity(const float sensitivity) { m_ZoomSensitivity = m_ZoomSensitivity; }

        void SetXY(const Vector2& position) { m_Position.x = position.x; m_Position.y = position.y; m_NeedSetView = true; }
        void TranslateXY(const Vector2& translation) { m_Position.x += translation.x; m_Position.y += translation.y; m_NeedSetView = true; }

        inline bool IsBeingControlled() { return m_BeingControlled; }
        Vector2 GetXY() const { return { m_Position.x, m_Position.y }; }
        float GetZoom() const { return m_ZoomLevel; }
    private:
        bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
        bool OnMouseScrolled(MouseScrolledEvent& e);
        bool OnWindowResized(WindowResizeEvent& e);

        virtual void SetView() = 0;         // override per camera type
        virtual void SetProjection() = 0;   // override per camera type
    protected:
        Camera m_Camera;

        Vector3 m_Position;
        Vector3 m_AimDirection;
        float m_AspectRatio;
        float m_Near, m_Far;

        float m_ZoomLevel = 1.f;
        float m_MinZoom, m_MaxZoom;
        float m_ZoomSensitivity;
    private:
        float m_CameraMoveSpeed = 1.f;

        bool m_BeingControlled = false;
        bool m_NeedSetView = false;
        bool m_NeedSetProjection = false;
    };


    class PerspectivePlanarCameraController : public PlanarCameraController
    {
    public:
        PerspectivePlanarCameraController(const Vector3& position, const Vector3& aimDirection,
            const float aspectRatio, const float nearDistance = 0.1f, const float farDistance = 100.f,
            const float fov = glm::radians(60.f));
        ~PerspectivePlanarCameraController() = default;

        /* !!! UNTESTED !!! */
        Vector3 GetWorldPos(const Vector2& screenPos, const float viewDepth);
    private:
        void SetView() override;
        void SetProjection() override;
    private:
        float m_BaseFov;
    };


    class OrthographicPlanarCameraController : public PlanarCameraController
    {
    public:
        OrthographicPlanarCameraController(const Vector3& position, const Vector3& aimDirection,
            const float aspectRatio, const float nearDistance = 0.1f, const float farDistance = 100.f);
        ~OrthographicPlanarCameraController() = default;

        Vector2 GetWorldXY(const Vector2& screenXY);
    private:
        void SetView() override;
        void SetProjection() override;
    };

}
