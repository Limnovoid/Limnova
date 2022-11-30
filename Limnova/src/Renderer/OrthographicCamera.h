#pragma once

#include "Camera.h"

#include "Math/Math.h"
#include "Math/glm.h"


namespace Limnova
{

    class OrthographicCamera : public Camera
    {
    public:
        OrthographicCamera(const float aspectRatio, const float nearDistance, const float farDistance,
            const Vector3& position, const Vector3& aimDirection, const Vector3& upDirection);
        ~OrthographicCamera();

        Data const* GetData() override;
        void RecomputeData() override;

        void SetProjection(const float aspectRatio, const float scale, const float nearDistance, const float farDistance);
        void SetView(const Vector3& position, const Vector3& aimDirection, const Vector3& upDirection);
    private:
        glm::mat4 m_Projection;
        glm::mat4 m_View;
        Data m_Data;
        bool m_NeedRecompute = false;
    };

}
