#pragma once

#include "Camera.h"

#include "Math/glm.h"


namespace Limnova
{

    class PointCamera : public Camera
    {
    public:
        PointCamera(const float fov, const float aspectRatio, const float nearDistance, const float farDistance);
        ~PointCamera();

        void RecomputeData() override;
        BufferData const* GetData() override;

        inline bool IsActive() override { return m_IsActive; }
        inline void SetActive() override { m_IsActive = true; }
        inline void SetNotActive() override { m_IsActive = false; }

        void SetFov(const float fov);
        void SetAspect(const float aspect);
        void SetNearFar(const float nearDistance, const float farDistance);

        void SetPosition(const glm::vec3& position);
        void SetAimDirection(const glm::vec3& aimDirection);
        void SetUpDirection(const glm::vec3& upDirection);
    private:
        BufferData m_Data;
        bool m_NeedRecompute;
        bool m_IsActive;

        float m_Fov;
        float m_Aspect;
        float m_Near;
        float m_Far;

        glm::vec3 m_Position;
        glm::vec3 m_AimDirection;
        glm::vec3 m_UpDirection;
    };

}
