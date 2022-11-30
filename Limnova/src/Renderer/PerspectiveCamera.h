#pragma once

#include "Camera.h"

#include "Math/Math.h"
#include "Math/glm.h"


namespace Limnova
{

    class PerspectiveCamera : public Camera
    {
    public:
        PerspectiveCamera(const float fov, const float aspectRatio, const float nearDistance, const float farDistance,
            const Vector3& position, const Vector3& aimDirection, const Vector3& upDirection);
        ~PerspectiveCamera();

        Data const* GetData() override;
        void RecomputeData() override;

        void SetProjection(const float fov, const float aspectRatio, const float nearDistance, const float farDistance);
        void SetView(const Vector3& position, const Vector3& aimDirection, const Vector3& upDirection);
    private:
        glm::mat4 m_Projection;
        glm::mat4 m_View;
        Data m_Data;
        bool m_NeedRecompute = false;
    };

}
