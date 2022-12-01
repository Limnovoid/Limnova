#pragma once

#include "Renderer/PerspectiveCamera.h"
#include "Renderer/OrthographicCamera.h"

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
        ~PlanarCameraController();

        void OnUpdate(Timestep dT);

        inline bool IsBeingControlled() { return m_BeingControlled; }

        virtual Camera& GetCamera() = 0;                // override per camera type
        virtual const Camera& GetCamera() const = 0;    // override per camera type

        void SetZoomLimits(const float minZoom, const float maxZoom) { m_MinZoom = minZoom; m_MaxZoom = maxZoom; }
        void SetZoomSensitivity(const float sensitivity) { m_ZoomSensitivity = m_ZoomSensitivity; }

        void OnEvent(Event& e);
    private:
        bool OnMouseButtonPressedEvent(MouseButtonPressedEvent& event);
        bool OnMouseScrolled(MouseScrolledEvent& e);
        bool OnWindowResized(WindowResizeEvent& e);

        virtual void SetView() = 0;         // override per camera type
        virtual void SetProjection() = 0;   // override per camera type
    protected:
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
        bool m_NeedSetProjection = false;
    };


    class PerspectivePlanarCameraController : public PlanarCameraController
    {
    public:
        PerspectivePlanarCameraController(const Vector3& position, const Vector3& aimDirection,
            const float aspectRatio, const float nearDistance = 0.1f, const float farDistance = 100.f,
            const float fov = glm::radians(60.f));
        ~PerspectivePlanarCameraController();

        Camera& GetCamera() override { return m_Camera; }
        const Camera& GetCamera() const override { return m_Camera; }
    private:
        void SetView() override;
        void SetProjection() override;
    private:
        float m_BaseFov;
        PerspectiveCamera m_Camera;
    };


    class OrthographicPlanarCameraController : public PlanarCameraController
    {
    public:
        OrthographicPlanarCameraController(const Vector3& position, const Vector3& aimDirection,
            const float aspectRatio, const float nearDistance = 0.1f, const float farDistance = 100.f);
        ~OrthographicPlanarCameraController();

        Camera& GetCamera() override { return m_Camera; }
        const Camera& GetCamera() const override { return m_Camera; }
    private:
        void SetView() override;
        void SetProjection() override;
    private:
        OrthographicCamera m_Camera;
    };

}