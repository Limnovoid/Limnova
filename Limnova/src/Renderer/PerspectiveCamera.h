#pragma once

#include "Camera.h"

#include "Math/Math.h"
#include "Math/glm.h"


namespace Limnova
{

    class PerspectiveCamera : public Camera
    {
    public:
        PerspectiveCamera(const float fov, const float aspectRatio, const float nearClip, const float farClip,
            const Vector3& position, const Vector3& aimDirection, const Vector3& upDirection);
        ~PerspectiveCamera() = default;

        Data const* GetData() override;
        void RecomputeData() override;

        void SetProjection(const float fov, const float aspectRatio, const float nearClip, const float farClip);
        void SetView(const Vector3& position, const Vector3& aimDirection, const Vector3& upDirection);
        void SetView(const glm::mat4& viewMatrix);
    private:
        glm::mat4 m_Projection;
        glm::mat4 m_View;
        Data m_Data;
        bool m_NeedRecompute = false;
    };

}
