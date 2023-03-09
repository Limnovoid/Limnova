#pragma once

#include <Math/Math.h>
#include <Renderer/PerspectiveCamera.h>
#include <Renderer/OrthographicCamera.h>

namespace Limnova
{

    struct TagComponent
    {
        std::string Tag;

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;
        TagComponent(const std::string& tag)
            : Tag(tag) {}
    };


    struct TransformComponent
    {
        glm::mat4 Transform{ 1.f };

        Vector3 Position;
        Quaternion Orientation;

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::mat4& transform)
            : Transform(transform) {}

        operator glm::mat4& () { return Transform; }
        operator const glm::mat4& () const { return Transform; }
    };


    struct SpriteRendererComponent
    {
        Vector4 Color{ 1.f, 0.f, 1.f, 1.f };

        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent&) = default;
        SpriteRendererComponent(const Vector4& color)
            : Color(color) {}
    };


    struct PerspectiveCameraComponent
    {
    private:
        float Fov{ glm::radians(60.f) };
        float AspectRatio{ 16.f / 9.f };
        float NearClip{ 0.1f }, FarClip{ 100.f };
    public:
        PerspectiveCamera Camera{ Fov, AspectRatio, NearClip, FarClip, {0.f}, {0.f,0.f,-1.f}, {0.f,1.f,0.f} };
        bool TieAspectToView = true;

        PerspectiveCameraComponent() = default;
        PerspectiveCameraComponent(const PerspectiveCameraComponent&) = default;
        PerspectiveCameraComponent(float fov, float aspectRatio, float nearClip, float farClip,
            const Vector3& position, const Vector3& aimDirection, const Vector3& upDirection)
            :
            Camera(fov, aspectRatio, nearClip, farClip, position, aimDirection, upDirection),
            Fov(fov), AspectRatio(aspectRatio), NearClip(nearClip), FarClip(farClip)
        {
        }

        void SetFov(float fov) { Fov = fov; UpdateProjection(); }
        void SetAspect(float aspect) { AspectRatio = aspect; UpdateProjection(); }
        void SetClip(float nearClip, float farClip) { NearClip = nearClip; FarClip = farClip; UpdateProjection(); }

        float GetFov() { return Fov; }
        float GetAspect() { return AspectRatio; }
        std::pair<float, float> GetClip() { return { NearClip, FarClip }; }
    private:
        void UpdateProjection()
        {
            Camera.SetProjection(Fov, AspectRatio, NearClip, FarClip);
        }
    };

}

