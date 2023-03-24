#pragma once

#include "Entity.h"
#include "Script.h"

#include <Math/Math.h>
#include <Renderer/Camera.h>

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
        friend class SceneHierarchyPanel;
    private:
        glm::mat4 Transform = glm::identity<glm::mat4>();
        bool NeedCompute = true;

        Vector3 Scale = { 1.f };
        Vector3 Position = { 0.f };
        Quaternion Orientation = Quaternion::Unit();
        Vector3 EulerAngles = { 0.f };
    public:
        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const Vector3& scale, const Vector3& position)
            : Scale(scale), Position(position), NeedCompute(true) {}

        void Set(const Vector3& scale, const Vector3& position) { Scale = scale; Position = position; NeedCompute = true; }
        void SetScale(const Vector3& scale) { Scale = scale; NeedCompute = true; }
        void SetPosition(const Vector3& position) { Position = position; NeedCompute = true; }
        void SetOrientation(const Quaternion& orientation)
        {
            Orientation = orientation;
            EulerAngles = Orientation.ToEulerAngles();
            NeedCompute = true;
        }
        void SetEulerAngles(const Vector3& eulerAngles)
        {
            EulerAngles = eulerAngles;
            Orientation = Quaternion(Vector3{ 1.f,0.f,0.f }, EulerAngles.x)
                * Quaternion(Vector3{ 0.f,1.f,0.f }, EulerAngles.y)
                * Quaternion(Vector3{ 0.f,0.f,1.f }, EulerAngles.z);
            NeedCompute = true;
        }

        const Vector3& GetScale() { return Scale; }
        const Vector3& GetPosition() { return Position; }
        const Quaternion& GetOrientation() { return Orientation; }
        const Vector3& GetEulerAngles() { return EulerAngles; }

        const glm::mat4& GetTransform() { if (NeedCompute) Compute(); return Transform; }
        operator const glm::mat4& () { if (NeedCompute) Compute(); return Transform; }
    private:
        void Compute()
        {
            Transform = glm::translate(glm::mat4(1.f), (glm::vec3)Position);
            Transform = Transform * Matrix4(Orientation).mat;
            Transform = glm::scale(Transform, (glm::vec3)Scale);
            NeedCompute = false;
        }
    };


    struct HierarchyComponent
    {
        friend class Scene;
        friend class OrbitalScene;
    private:
        Entity Parent;
        Entity NextSibling;
        Entity PrevSibling;
        Entity FirstChild;
    public:
        HierarchyComponent() = default;
        HierarchyComponent(const HierarchyComponent&) = default;
    };


    struct CameraComponent
    {
    private:
        float VerticalFov = { Radiansf(60.f) };
        float OrthographicHeight = 1.f;
        float AspectRatio = { 16.f / 9.f };
        float OrthoNearClip = { -5.f }, OrthoFarClip{ 5.f };
        float PerspNearClip = { 0.01f }, PerspFarClip{ 1000.f };
    public:
        Camera Camera = { {0.f}, Vector3::Forward(), Vector3::Up()};
        bool TieAspectToView = true;
        bool IsOrthographic = false;

        CameraComponent() = default;
        CameraComponent(const CameraComponent&) = default;
        CameraComponent(const Vector3& position, const Vector3& aimDirection, const Vector3& upDirection)
            : Camera(position, aimDirection, upDirection)
        {
            UpdateProjection();
        }

        void SetOrthographicProjection(const float aspectRatio, const float height, const float nearClip, const float farClip)
        {
            OrthographicHeight = height;
            AspectRatio = aspectRatio;
            OrthoNearClip = nearClip;
            OrthoFarClip = farClip;

            IsOrthographic = true;
            UpdateProjection();
        }

        void SetPerspectiveProjection(const float verticalFov, const float aspectRatio, const float nearClip, const float farClip)
        {
            VerticalFov = verticalFov;
            AspectRatio = aspectRatio;
            PerspNearClip = nearClip;
            PerspFarClip = farClip;

            IsOrthographic = false;
            UpdateProjection();
        }

        void SetOrthographic(bool isOrthographic) { IsOrthographic = isOrthographic; UpdateProjection(); }
        bool GetOrthographic() { return IsOrthographic; }

        void SetOrthographicHeight(float orthographicHeight) { OrthographicHeight = orthographicHeight; UpdateProjection(); }
        float GetOrthographicHeight() { return OrthographicHeight; }

        void SetPerspectiveFov(float verticalFov) { VerticalFov = verticalFov; UpdateProjection(); }
        float GetPerspectiveFov() { return VerticalFov; }

        void SetOrthographicClip(float nearClip, float farClip) { OrthoNearClip = nearClip; OrthoFarClip = farClip; UpdateProjection(); }
        std::pair<float, float> GetOrthographicClip() { return { OrthoNearClip, OrthoFarClip }; }

        void SetPerspectiveClip(float nearClip, float farClip) { PerspNearClip = nearClip; PerspFarClip = farClip; UpdateProjection(); }
        std::pair<float, float> GetPerspectiveClip() { return { PerspNearClip, PerspFarClip }; }

        void SetAspectRatio(float aspectRatio) { AspectRatio = aspectRatio; UpdateProjection(); }
        float GetAspectRatio() { return AspectRatio; }
    private:
        void UpdateProjection()
        {
            if (IsOrthographic) {
                Camera.SetOrthographicProjection(AspectRatio, OrthographicHeight, OrthoNearClip, OrthoFarClip);
            }
            else {
                Camera.SetPerspectiveProjection(VerticalFov, AspectRatio, PerspNearClip, PerspFarClip);
            }
        }
    };


    struct NativeScriptComponent
    {
        friend class Scene;
    private:
        NativeScript* Instance = nullptr;

        void (*InstantiateScript)(NativeScript**);
        void (*DeleteScript)(NativeScript**);
    public:
        template<typename T>
        void Bind()
        {
            if (Instance) DeleteScript(&Instance); /* Deletes existing script - NOTE: potentially unnecessary as scripts are not meant to be instantiated outside scene playtime */

            InstantiateScript = [](NativeScript** instance) { *instance = static_cast<NativeScript*>(new T()); };
            DeleteScript = [](NativeScript** instance) { delete *instance; *instance = nullptr; };
        }
    };


    struct SpriteRendererComponent
    {
        Vector4 Color{ 1.f, 0.f, 1.f, 1.f };

        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent&) = default;
        SpriteRendererComponent(const Vector4& color)
            : Color(color) {}
    };


    struct CircleRendererComponent
    {
        Vector4 Color = { 1.f, 0.f, 1.f, 1.f };
        float Thickness = 0.5f;
        float Fade = 0.005f;

        CircleRendererComponent() = default;
        CircleRendererComponent(const CircleRendererComponent&) = default;
    };

}
