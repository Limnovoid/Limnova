#include "PerspectiveCamera.h"

#include "Application.h"


namespace Limnova
{

    PerspectiveCamera::PerspectiveCamera(const float fov, const float aspectRatio, const float nearDistance, const float farDistance)
        : m_Projection(glm::perspectiveRH_ZO(fov, aspectRatio, nearDistance, farDistance)),
        m_View(glm::lookAtRH(glm::vec3(0.f), glm::vec3(0.f, 0.f,-1.f), glm::vec3(0.f, 1.f, 0.f ))),
        m_Data(m_Projection * m_View, glm::vec4(0.f), glm::vec4(0.f, 0.f,-1.f, 0.f))
    {
    }


    PerspectiveCamera::~PerspectiveCamera()
    {
    }


    Camera::BufferData const* PerspectiveCamera::GetData()
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
        m_Data.Position = glm::vec4((glm::vec3)position, 1.f);
        m_Data.AimDirection = glm::vec4((glm::vec3)aimDirection, 1.f);
        m_View = glm::lookAtRH((glm::vec3)position,
            (glm::vec3)position + (glm::vec3)aimDirection,
            (glm::vec3)upDirection
        );
        m_NeedRecompute = true;
    }

}
