#pragma once

#include "Entity.h"
#include "Script.h"

#include <Math/Math.h>
#include <Renderer/Camera.h>
#include <Orbital/OrbitalPhysics.h>

namespace Limnova
{

    struct TagComponent
    {
        //LV_REFLECT(TagComponent)

        std::string Tag;

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;
        TagComponent(const std::string& tag)
            : Tag(tag) {}
    };


    struct TransformComponent
    {
        //LV_REFLECT(TransformComponent)
        friend class SceneHierarchyPanel;
    private:
        Matrix4 Transform = Matrix4::Identity();
        bool NeedCompute = true;

        Vector3 Position = { 0.f };
        Quaternion Orientation = Quaternion::Unit();
        Vector3 EulerAngles = { 0.f };
        Vector3 Scale = { 1.f };
    public:
        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const Vector3& scale, const Vector3& position)
            : Scale(scale), Position(position), NeedCompute(true) {}

        void Set(const Vector3& position, const Quaternion& orientation, const Vector3& scale)
        {
            Orientation = orientation;
            Position = position;
            Scale = scale;
            NeedCompute = true;
        }
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

        const Matrix4& GetTransform() { if (NeedCompute) Compute(); return Transform; }
        operator const Matrix4& () { if (NeedCompute) Compute(); return Transform; }
    private:
        void Compute()
        {
            Transform = glm::translate(glm::mat4(1.f), (glm::vec3)Position);
            Transform = Transform * Matrix4(Orientation);
            Transform = glm::scale((glm::mat4)Transform, (glm::vec3)Scale);
            NeedCompute = false;
        }
    };


    struct HierarchyComponent
    {
        //LV_REFLECT(HierarchyComponent)
        friend class Scene;
        friend class OrbitalScene;
        friend class SceneSerializer;
    private:
        /* TODO - use UUIDs */
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
        //LV_REFLECT(CameraComponent)
        friend class SceneSerializer;
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

        void SetIsOrthographic(bool isOrthographic) { IsOrthographic = isOrthographic; UpdateProjection(); }
        bool GetIsOrthographic() { return IsOrthographic; }

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
        //LV_REFLECT(NativeScriptComponent)

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
        //LV_REFLECT(SpriteRendererComponent);

        Vector4 Color{ 1.f, 0.f, 1.f, 1.f };

        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent&) = default;
        SpriteRendererComponent(const Vector4& color)
            : Color(color) {}
    };


    struct BillboardSpriteRendererComponent
    {
        //LV_REFLECT(BillboardSpriteRendererComponent);

        Vector4 Color{ 1.f, 0.f, 1.f, 1.f };

        BillboardSpriteRendererComponent() = default;
        BillboardSpriteRendererComponent(const BillboardSpriteRendererComponent&) = default;
        BillboardSpriteRendererComponent(const Vector4& color)
            : Color(color) {}
    };


    struct CircleRendererComponent
    {
        //LV_REFLECT(CircleRendererComponent);

        Vector4 Color = { 1.f, 0.f, 1.f, 1.f };
        float Thickness = 0.5f;
        float Fade = 0.005f;

        CircleRendererComponent() = default;
        CircleRendererComponent(const CircleRendererComponent&) = default;
    };


    struct BillboardCircleRendererComponent
    {
        //LV_REFLECT(BillboardCircleRendererComponent);

        Vector4 Color = { 1.f, 0.f, 1.f, 1.f };
        float Thickness = 0.5f;
        float Fade = 0.005f;

        BillboardCircleRendererComponent() = default;
        BillboardCircleRendererComponent(const BillboardCircleRendererComponent&) = default;
    };


    struct EllipseRendererComponent
    {
        //LV_REFLECT(EllipseRendererComponent);

        Vector4 Color = { 1.f, 0.f, 1.f, 1.f };
        float Thickness = 0.04f;
        float Fade = 0.f;

        EllipseRendererComponent() = default;
        EllipseRendererComponent(const EllipseRendererComponent&) = default;
    };


    using Physics = OrbitalPhysics<entt::entity>;
    struct OrbitalComponent
    {
        //LV_REFLECT(OrbitalComponent);

        friend class OrbitalScene;
    private:
        Physics::TObjectId PhysicsObjectId = Physics::Null;
        Physics* Physics = nullptr;
    public:
        Vector3 LocalScale = { 1.f };
        Vector3 UIColor = { 1.f };
        float Albedo; /* Surface reflectivity of orbital object - determines object's brightness as a star-like object when viewed from far away */

        bool ShowMajorMinorAxes = false;
        bool ShowNormal = false;

        OrbitalComponent() = default;
        //OrbitalComponent(const OrbitalComponent&) = default;
        //OrbitalComponent(const OrbitalPhysics<entt::entity>::TObjectId& physicsObjectId)
        //    : PhysicsObjectId(physicsObjectId) {}

        Physics::Validity GetValidity() { return Physics->GetValidity(PhysicsObjectId); }

        bool IsInfluencing() { return Physics->IsInfluencing(PhysicsObjectId); }
        void SetLocalSpaceRadius(float radius)
        {
            float oldRadius = Physics->GetLocalSpaceRadius(PhysicsObjectId);
            if (Physics->SetLocalSpaceRadius(PhysicsObjectId, radius))
            {
                LocalScale *= oldRadius / radius;
            }
        }
        float GetLocalSpaceRadius() { return Physics->GetLocalSpaceRadius(PhysicsObjectId); }

        void SetMass(double mass) { Physics->SetMass(PhysicsObjectId, mass); }
        void SetPosition(const Vector3& position) { Physics->SetPosition(PhysicsObjectId, position); }
        void SetVelocity(const Vector3d& velocity) { Physics->SetVelocity(PhysicsObjectId, velocity); }

        void SetCircular(bool reverse = false)
        {
            auto v = Physics->GetDefaultOrbitVelocity(PhysicsObjectId);
            if (reverse) v = -v;
            Physics->SetVelocity(PhysicsObjectId, v);
        }

        double GetMass() { return Physics->GetMass(PhysicsObjectId); }
        Vector3 GetPosition() { return Physics->GetPosition(PhysicsObjectId); }
        Vector3d GetVelocity() { return Physics->GetVelocity(PhysicsObjectId); }

        const Physics::Elements& GetElements() const { return Physics->GetElements(PhysicsObjectId); }
        const Physics::Dynamics& GetDynamics() const { return Physics->GetDynamics(PhysicsObjectId); }

        bool IsDynamic() const { return Physics->IsDynamic(PhysicsObjectId); }
        void SetDynamic(bool isDynamic = true) { Physics->SetDynamic(PhysicsObjectId, isDynamic); }
    };

}
