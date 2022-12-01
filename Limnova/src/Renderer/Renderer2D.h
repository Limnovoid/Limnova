#pragma once

#include "Renderer.h"

#include "Camera.h"


namespace Limnova
{

    class Renderer2D
    {
    public:
        static void Init(const Ref<UniformBuffer>& sceneUniformBuffer);
        static void Shutdown();

        static void BeginScene(Camera& camera);
        static void EndScene();

        static void DrawQuad(const Vector3& positionBottomLeft, const Vector2& size, const Vector4& color);
        static void DrawQuad(const Vector2& positionBottomLeft, const Vector2& size, const Vector4& color);
    private:
        static Ref<UniformBuffer> m_SceneUniformBuffer;
    };

}
