#include "Camera.h"


namespace Limnova
{

    Camera::Camera(const Vector3& position, const Vector3& aimDirection, const Vector3& upDirection)
    {
        SetView(position, aimDirection, upDirection);
        SetPerspectiveProjection(Radiansf(60.f), 16.f / 9.f, 0.01f, 1000.f);
        m_NeedRecompute = true;
    }


    Camera::Data const* Camera::GetData()
    {
        if (m_NeedRecompute)
        {
            RecomputeData();
        }
        return &m_Data;
    }


    void Camera::RecomputeData()
    {
        LV_PROFILE_FUNCTION();

        m_Data.ViewProj = m_Projection * m_View;
        m_NeedRecompute = false;
    }


    void Camera::SetView(const Vector3& position, const Vector3& aimDirection, const Vector3& upDirection)
    {
        LV_PROFILE_FUNCTION();

        m_Data.Position = (glm::vec3)position;
        m_Data.AimDirection = (glm::vec3)aimDirection;
        m_View = glm::lookAtRH((glm::vec3)position,
            (glm::vec3)position + (glm::vec3)aimDirection,
            (glm::vec3)upDirection
        );
        m_NeedRecompute = true;
    }


    void Camera::SetView(const glm::mat4& viewMatrix)
    {
        LV_PROFILE_FUNCTION();

        m_View = viewMatrix;
        m_NeedRecompute = true;
    }


    void Camera::SetOrthographicProjection(const float aspectRatio, const float scale, const float nearDistance, const float farDistance)
    {
        LV_PROFILE_FUNCTION();

        m_Projection = glm::orthoRH_ZO(-aspectRatio * scale, aspectRatio * scale, -scale, scale, nearDistance, farDistance);
        m_NeedRecompute = true;
    }


    void Camera::SetPerspectiveProjection(const float verticalFov, const float aspectRatio, const float nearDistance, const float farDistance)
    {
        LV_PROFILE_FUNCTION();

        m_Projection = glm::perspectiveRH_ZO(verticalFov, aspectRatio, nearDistance, farDistance);
        m_NeedRecompute = true;
    }

}
