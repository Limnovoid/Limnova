#pragma once

#include "Renderer.h"

#include "Camera.h"
#include "Texture.h"
#include "SubTexture.h"


namespace Limnova
{

    class Renderer2D
    {
    public:
        static void Init(const Ref<UniformBuffer>& sceneUniformBuffer);
        static void Shutdown();

        static void BeginScene(Camera& camera);
        static void EndScene();
        static void Flush();

        static void DrawQuad(const Vector3& positionCentre, const Vector2& size, const Vector4& color);
        static void DrawQuad(const Vector2& positionCentre, const Vector2& size, const Vector4& color);
        static void DrawQuad(const Vector3& positionCentre, const Vector2& size, const Ref<Texture2D>& texture, const Vector4& tint = { 1.f }, const Vector2& textureScale = { 1.f });
        static void DrawQuad(const Vector2& positionCentre, const Vector2& size, const Ref<Texture2D>& texture, const Vector4& tint = { 1.f }, const Vector2& textureScale = { 1.f });
        static void DrawQuad(const Vector3& positionCentre, const Vector2& size, const Ref<SubTexture2D>& subTexture, const Vector4& tint = { 1.f }, const Vector2& textureScale = { 1.f });
        static void DrawQuad(const Vector2& positionCentre, const Vector2& size, const Ref<SubTexture2D>& subTexture, const Vector4& tint = { 1.f }, const Vector2& textureScale = { 1.f });

        static void DrawRotatedQuad(const Vector3& positionCentre, const Vector2& size, const float rotation, const Vector4& color);
        static void DrawRotatedQuad(const Vector2& positionCentre, const Vector2& size, const float rotation, const Vector4& color);
        static void DrawRotatedQuad(const Vector3& positionCentre, const Vector2& size, const float rotation, const Ref<Texture2D>& texture, const Vector4& tint = { 1.f }, const Vector2& textureScale = { 1.f });
        static void DrawRotatedQuad(const Vector2& positionCentre, const Vector2& size, const float rotation, const Ref<Texture2D>& texture, const Vector4& tint = { 1.f }, const Vector2& textureScale = { 1.f });
        static void DrawRotatedQuad(const Vector3& positionCentre, const Vector2& size, const float rotation, const Ref<SubTexture2D>& subTexture, const Vector4& tint = { 1.f }, const Vector2& textureScale = { 1.f });
        static void DrawRotatedQuad(const Vector2& positionCentre, const Vector2& size, const float rotation, const Ref<SubTexture2D>& subTexture, const Vector4& tint = { 1.f }, const Vector2& textureScale = { 1.f });

        static void DrawLine(const Vector2& start, const Vector2& end, const float thickness, const Vector4& color, int layer = 0);

        static void DrawEllipse(const Vector2& centre, const float rotation, const float semiMajorAxis, const float semiMinorAxis, const Vector2& escapePointFromCentre, const float thickness, const Vector4& color, int layer = 0);
        static void DrawHyperbola(const Vector2& centre, const float rotation, const float semiMajorAxis, const float semiMinorAxis, const Vector2& escapePointFromCentre, const float thickness, const Vector4& color, int layer = 0);

        static void TEMP_BeginEllipses();
        static void TEMP_BeginHyperbolae();
    private:
        static Ref<UniformBuffer> s_SceneUniformBuffer;
        static Ref<UniformBuffer> s_HyperbolaUniformBuffer;
    private:
        static void ResetBatch();

        // stats
    public:
        struct Statistics
        {
            uint32_t DrawCalls;
            uint32_t QuadCount;

            uint32_t GetNumVertices() { return QuadCount * 4; }
            uint32_t GetNumIndices() { return QuadCount * 6; }
        };
        static Statistics& GetStatistics();
        static void ResetStatistics();
    };

}
