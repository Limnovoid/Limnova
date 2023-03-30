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
    private:
        static void DrawBatchedQuad(const glm::mat4& transform, const Vector4& color, const Vector2* textureCoords, const Vector2& textureScale, const float textureIndex);
    public:
        static void DrawQuad(const glm::mat4& transform, const Vector4& color);
        static void DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, const Vector4& tint = { 1.f }, const Vector2& textureScale = { 1.f });
        static void DrawQuad(const glm::mat4& transform, const Ref<SubTexture2D>& subTexture, const Vector4& tint = { 1.f }, const Vector2& textureScale = { 1.f });

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

        static void DrawCircle(const Matrix4& transform, const Vector4& color, float thickness = 1.f, float fade = 0.005f);
        static void DrawCircle(const Vector3& origin, float radius, const Vector4& color, float thickness = 1.f, float fade = 0.005f);

        static void DrawEllipse(const Matrix4& transform, float majorMinorAxisRatio, const Vector4& color, float thickness = 1.f, float fade = 0.005f);
        static void DrawEllipse(const Vector3& centre, const Quaternion& orientation, float semiMajorAxis, float semiMinorAxis, const Vector4& color, float thickness = 1.f, float fade = 0.005f);

        static void DrawLine(const Vector2& start, const Vector2& end, const float width, const Vector4& color, int layer = 0);

        static void DrawEllipse(const Vector2& centre, const float rotation, const float semiMajorAxis, const float semiMinorAxis, const Vector2& escapePointFromCentre, const float thickness, const Vector4& color, int layer = 0);
        static void DrawHyperbola(const Vector2& centre, const float rotation, const float semiMajorAxis, const float semiMinorAxis, const Vector2& escapePointFromCentre, const float thickness, const Vector4& color, int layer = 0);

        static void TEMP_BeginEllipses();
        static void TEMP_BeginHyperbolae();
    private:
        static Ref<UniformBuffer> s_SceneUniformBuffer;
        static Ref<UniformBuffer> s_HyperbolaUniformBuffer;
    private:
        static void FlushQuads();
        static void ResetQuadBatch();

        static void FlushCircles();
        static void ResetCircleBatch();

        static void FlushEllipses();
        static void ResetEllipseBatch();

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
