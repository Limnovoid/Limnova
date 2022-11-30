#include "OrthographicCamera.h"

#include "Core/Application.h"


namespace Limnova
{

    OrthographicCamera::OrthographicCamera(const float aspectRatio, const float nearDistance, const float farDistance,
        const Vector3& position, const Vector3& aimDirection, const Vector3& upDirection)
        : m_Projection(glm::orthoRH_ZO(-aspectRatio, aspectRatio, -1.f, 1.f, nearDistance, farDistance)),
        m_View(glm::lookAtRH((glm::vec3)position, (glm::vec3)position + (glm::vec3)aimDirection, (glm::vec3)upDirection)),
        m_Data(m_Projection * m_View, (glm::vec3)position, (glm::vec3)aimDirection)
    {
    }


    OrthographicCamera::~OrthographicCamera()
    {
    }


    Camera::Data const* OrthographicCamera::GetData()
    {
        if (m_NeedRecompute)
        {
            RecomputeData();
        }
        return &m_Data;
    }


    void OrthographicCamera::RecomputeData()
    {
        m_Data.ViewProj = m_Projection * m_View;
        m_NeedRecompute = false;
    }


    void OrthographicCamera::SetProjection(const float aspectRatio, const float scale, const float nearDistance, const float farDistance)
    {
        m_Projection = glm::orthoRH_ZO(-aspectRatio * scale, aspectRatio * scale, -scale, scale, nearDistance, farDistance);
        m_NeedRecompute = true;
    }


    void OrthographicCamera::SetView(const Vector3& position, const Vector3& aimDirection, const Vector3& upDirection)
    {
        m_Data.Position = (glm::vec3)position;
        m_Data.AimDirection = (glm::vec3)aimDirection;
        m_View = glm::lookAtRH((glm::vec3)position,
            (glm::vec3)position + (glm::vec3)aimDirection,
            (glm::vec3)upDirection
        );
        m_NeedRecompute = true;
    }

}
