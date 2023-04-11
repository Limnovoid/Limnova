#pragma once

#include "Math/Math.h"
#include "Math/glm.h"


namespace Limnova
{

    class Camera
    {
    public:
        struct Data
        {
            Matrix4 ViewProj;
            Vector3 Position;
        /*--pad 1byte-------------------------*/private: float pad0; public:
            Vector3 AimDirection;
        /*--pad 1byte-------------------------*/private: float pad1; public:

            Data() = default;
            Data(const Matrix4& viewProj, const Vector3& position, const Vector3& aimDirection)
                : ViewProj(viewProj), Position(position), AimDirection(aimDirection) {}
        };
    public:
        Camera(const Vector3& position, const Vector3& aimDirection, const Vector3& upDirection);
        ~Camera() = default;

        void SetView(const Vector3& position, const Vector3& aimDirection, const Vector3& upDirection);
        void SetView(const glm::mat4& viewMatrix);

        void SetOrthographicProjection(const float aspectRatio, const float height, const float nearClip, const float farClip);
        void SetPerspectiveProjection(const float verticalFov, const float aspectRatio, const float nearClip, const float farClip);

        const Matrix4& GetProjection() { return m_Projection; }
        const Matrix4& GetView() { return m_View; }
    private:
        virtual Data const* GetData();
        void RecomputeData();
    private:
        Matrix4 m_Projection;
        Matrix4 m_View;
        Data m_Data;
        bool m_NeedRecompute = false;

        friend class Renderer;
        friend class Renderer2D;
    };

}
