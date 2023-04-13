#pragma once

#include "Renderer.h"

#include "Camera.h"
#include "Texture.h"
#include "SubTexture.h"

#include "Scene/Components.h"


namespace Limnova
{

    class Renderer2D
    {
    public:
        static void Init();
        static void Shutdown();

        static void BeginScene(Camera& camera);
        static void EndScene();

        // Quads //
    private:
        static void DrawBatchedQuad(const Matrix4& transform, const Vector4& color, const Vector2* textureCoords, const Vector2& textureScale, const float textureIndex, int entityId = -1);
    public:
        static void DrawQuad(const Matrix4& transform, const Vector4& color, int entityId = -1);
        static void DrawQuad(const Matrix4& transform, const Ref<Texture2D>& texture, const Vector4& tint = { 1.f }, const Vector2& textureScale = { 1.f });
        static void DrawQuad(const Matrix4& transform, const Ref<SubTexture2D>& subTexture, const Vector4& tint = { 1.f }, const Vector2& textureScale = { 1.f });

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

        // Circles //
    public:
        static void DrawCircle(const Matrix4& transform, const Vector4& color, float thickness = 1.f, float fade = 0.005f, int entityId = -1);
        static void DrawCircle(const Vector3& origin, float radius, const Vector4& color, float thickness = 1.f, float fade = 0.005f, int entityId = -1);

        // Ellipses //
    private:
        static void DrawBatchedEllipse(const Matrix4& transform, float majorMinorAxisRatio, const Vector4& color, float thickness = 1.f, float fade = 0.005f, int entityId = -1);
    public:
        static void DrawEllipse(const Matrix4& transform, float majorMinorAxisRatio, const Vector4& color, float thickness = 1.f, float fade = 0.005f, int entityId = -1);
        static void DrawEllipse(const Vector3& centre, const Quaternion& orientation, float semiMajorAxis, float semiMinorAxis, const Vector4& color, float thickness = 1.f, float fade = 0.005f, int entityId = -1);

        // Lines //
    private:
        static void DrawBatchedLine(const Matrix4& transform, const Vector4& color, float length, float thickness, int entityId = -1);
    public:
        static void DrawLine(const Vector2& start, const Vector2& end, float width, const Vector4& color, int layer = 0);
        static void DrawLine(const Vector3& start, const Vector3& end, const Quaternion& orientation, const Vector4& color, float thickness, int entityId = -1);

        // Orbital - OLD/SUPERSEDED //
        static void DrawEllipse(const Vector2& centre, const float rotation, const float semiMajorAxis, const float semiMinorAxis, const Vector2& escapePointFromCentre, const float thickness, const Vector4& color, int layer = 0);
        static void DrawHyperbola(const Vector2& centre, const float rotation, const float semiMajorAxis, const float semiMinorAxis, const Vector2& escapePointFromCentre, const float thickness, const Vector4& color, int layer = 0);
    private:
        static Ref<UniformBuffer> s_HyperbolaUniformBuffer;
    private:
        static void FlushQuads();
        static void ResetQuadBatch();

        static void FlushCircles();
        static void ResetCircleBatch();

        static void FlushEllipses();
        static void ResetEllipseBatch();

        static void FlushLines();
        static void ResetLineBatch();

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
