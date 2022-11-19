#include "PointCamera.h"

#include "Application.h"


namespace Limnova
{

    PointCamera::PointCamera(const float fov, const float aspectRatio, const float nearDistance, const float farDistance)
        : m_IsActive(false),
        m_Fov(fov), m_Aspect(aspectRatio), m_Near(nearDistance), m_Far(farDistance),
        m_Position(0.f), m_AimDirection(0.f, 0.f, -1.f), m_UpDirection(0.f, 1.f, 0.f)
    {
        RecomputeData();
    }


    PointCamera::~PointCamera()
    {
    }


    void PointCamera::RecomputeData()
    {
        glm::mat4 view = glm::lookAtRH(m_Position, m_Position + m_AimDirection, m_UpDirection);
        glm::mat4 proj = glm::perspectiveRH_ZO(m_Fov, m_Aspect, m_Near, m_Far);
        m_Data.ViewProj = proj * view;
        m_Data.Position = glm::vec4(m_Position, 1.f);
        m_Data.AimDirection = glm::vec4(m_AimDirection, 1.f);
        m_NeedRecompute = false;
    }


    Camera::BufferData const* PointCamera::GetData()
    {
        if (m_NeedRecompute)
        {
            RecomputeData();
        }
        return &m_Data;
    }


    void PointCamera::SetFov(const float fov)
    {
        m_Fov = fov;
        m_NeedRecompute = true;
    }


    void PointCamera::SetAspect(const float aspect)
    {
        m_Aspect = aspect;
        m_NeedRecompute = true;
    }


    void PointCamera::SetNearFar(const float nearDistance, const float farDistance)
    {
        m_Near = nearDistance;
        m_Far = farDistance;
        m_NeedRecompute = true;
    }


    void PointCamera::SetPosition(const glm::vec3& position)
    {
        m_Position = position;
        m_NeedRecompute = true;
    }


    void PointCamera::SetAimDirection(const glm::vec3& aimDirection)
    {
        m_AimDirection = aimDirection;
        m_NeedRecompute = true;
    }


    void PointCamera::SetUpDirection(const glm::vec3& upDirection)
    {
        m_UpDirection = upDirection;
        m_NeedRecompute = true;
    }


    void PointCamera::EnableMouseAim()
    {
        Limnova::Application::Get().GetWindow().DisableCursor();
    }


    void PointCamera::DisableMouseAim()
    {
        Application::Get().GetWindow().EnableCursor();
    }


    void PointCamera::UpdateMouseAim(float deltaT)
    {

    }

}
