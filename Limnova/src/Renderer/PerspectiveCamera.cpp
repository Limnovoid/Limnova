#include "PerspectiveCamera.h"

#include "Core/Application.h"


namespace Limnova
{

    PerspectiveCamera::PerspectiveCamera(const float fov, const float aspectRatio, const float nearDistance, const float farDistance,
        const Vector3& position, const Vector3& aimDirection, const Vector3& upDirection)
        : m_Projection(glm::perspectiveRH_ZO(fov, aspectRatio, nearDistance, farDistance)),
        m_View(glm::lookAtRH((glm::vec3)position, (glm::vec3)position + (glm::vec3)aimDirection, (glm::vec3)upDirection)),
        m_Data(m_Projection * m_View, (glm::vec3)position, (glm::vec3)aimDirection)
    {
    }


    PerspectiveCamera::~PerspectiveCamera()
    {
    }


    Camera::Data const* PerspectiveCamera::GetData()
    {
        if (m_NeedRecompute)
        {
            RecomputeData();
        }
        return &m_Data;
    }


    void PerspectiveCamera::RecomputeData()
    {
        m_Data.ViewProj = m_Projection * m_View;
        m_NeedRecompute = false;
    }


    void PerspectiveCamera::SetProjection(const float fov, const float aspectRatio, const float nearDistance, const float farDistance)
    {
        m_Projection = glm::perspectiveRH_ZO(fov, aspectRatio, nearDistance, farDistance);
        m_NeedRecompute = true;
    }


    void PerspectiveCamera::SetView(const Vector3& position, const Vector3& aimDirection, const Vector3& upDirection)
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
