#include "Renderer2D.h"

#include "VertexArray.h"
#include "Shader.h"


namespace Limnova
{

    Ref<UniformBuffer> Renderer2D::s_SceneUniformBuffer = nullptr;


    struct QuadVertex
    {
        Vector3 Position;
        Vector4 Color;
        Vector2 TexCoord;
        Vector2 TexScale;
        float TexIndex;
    };


    struct CircleVertex
    {
        Vector3 WorldPosition;
        Vector2 LocalPosition;
        Vector4 Color;
        float Thickness;
        float Fade;
    };


    struct Renderer2DData
    {
        const uint32_t MaxQuads = 10000;
        const uint32_t MaxQuadVertices = MaxQuads * 4;
        const uint32_t MaxQuadIndices = MaxQuads * 6;
        static const uint32_t MaxTextureSlots = 32; // TODO : set with render capabilities

        Ref<VertexArray> QuadVertexArray;
        Ref<VertexBuffer> QuadVertexBuffer;
        Ref<Shader> QuadShader;
        Ref<Texture2D> WhiteTexture;

        uint32_t QuadIndexCount = 0;
        QuadVertex* QuadVertexBufferBase = nullptr;
        QuadVertex* QuadVertexBufferPtr = nullptr;

        std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
        uint32_t TextureSlotIndex = 1; // 0 = white texture

        Vector4 QuadVertexPositions[4];

        Renderer2D::Statistics Stats;

        // Circles
        const uint32_t MaxCircles = 10000;
        const uint32_t MaxCircleVertices = MaxCircles * 4;
        const uint32_t MaxCircleIndices = MaxCircles * 6;

        Ref<VertexArray> CircleVertexArray;
        Ref<VertexBuffer> CircleVertexBuffer;
        Ref<Shader> CircleShader;

        uint32_t CircleIndexCount = 0;
        CircleVertex* CircleVertexBufferBase = nullptr;
        CircleVertex* CircleVertexBufferPtr = nullptr;

        // Orbit resources
        Ref<VertexArray> HyperbolaVertexArray;
        Ref<Shader> HyperbolaShader;
        Ref<VertexArray> EllipseVertexArray;
        Ref<Shader> EllipseShader;
        Ref<UniformBuffer> OrbitUniformBuffer;
    };

    static Renderer2DData s_Data;


    struct OrbitData
    {
        float XOffset;
        float XLimit;
        float YLimit;
        float XEscape;
        float YEscape;
        float SemiMajorAxis;
        float SemiMinorAxis;
        float DrawRadius;
        float XEscapeTangent;
        /*pad 3 bytes --------------------*/private: float pad0[3]; public:
    };

    static OrbitData* s_OrbitData;


    void Renderer2D::Init(const Ref<UniformBuffer>& sceneUniformBuffer)
    {
        LV_PROFILE_FUNCTION();

        s_SceneUniformBuffer = sceneUniformBuffer;

        // Quads
        s_Data.QuadVertexArray = VertexArray::Create();

        s_Data.QuadVertexBuffer = VertexBuffer::Create(s_Data.MaxQuadVertices * sizeof(QuadVertex));
        s_Data.QuadVertexBuffer->SetLayout({
            { ShaderDataType::Float3,   "a_Position" },
            { ShaderDataType::Float4,   "a_Color"    },
            { ShaderDataType::Float2,   "a_TexCoord" },
            { ShaderDataType::Float2,   "a_TexScale" },
            { ShaderDataType::Float,    "a_TexIndex" }
        });
        s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);

        s_Data.QuadVertexBufferBase = new QuadVertex[s_Data.MaxQuadVertices];

        uint32_t* quadIndices = new uint32_t[s_Data.MaxQuadIndices];
        uint32_t offset = 0;
        for (uint32_t i = 0; i < s_Data.MaxQuadIndices; i += 6)
        {
            quadIndices[i + 0] = offset + 0;
            quadIndices[i + 1] = offset + 1;
            quadIndices[i + 2] = offset + 2;

            quadIndices[i + 3] = offset + 0;
            quadIndices[i + 4] = offset + 2;
            quadIndices[i + 5] = offset + 3;

            offset += 4;
        }
        Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_Data.MaxQuadIndices);
        s_Data.QuadVertexArray->SetIndexBuffer(quadIB);
        delete[] quadIndices;

        s_Data.WhiteTexture = Texture2D::Create(1, 1);
        uint32_t whiteTextureData = 0xffffffff;
        s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

        int samplers[s_Data.MaxTextureSlots];
        for (uint32_t i = 0; i < s_Data.MaxTextureSlots; i++)
        {
            samplers[i] = i;
        }

        s_Data.QuadShader = Shader::Create(LV_ASSET_DIR"/shaders/Renderer2D_Quad.lvglsl");
        s_Data.QuadShader->BindUniformBuffer(Renderer::GetSceneUniformBufferId(), "CameraUniform");
        s_Data.QuadShader->Bind();
        s_Data.QuadShader->SetIntArray("u_Textures", samplers, s_Data.MaxTextureSlots);

        s_Data.TextureSlots[0] = s_Data.WhiteTexture;

        s_Data.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.f, 1.f };
        s_Data.QuadVertexPositions[1] = {  0.5f, -0.5f, 0.f, 1.f };
        s_Data.QuadVertexPositions[2] = {  0.5f,  0.5f, 0.f, 1.f };
        s_Data.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.f, 1.f };

        // Circles
        s_Data.CircleVertexArray = VertexArray::Create();

        s_Data.CircleVertexBuffer = VertexBuffer::Create(s_Data.MaxCircleVertices * sizeof(CircleVertex));
        s_Data.CircleVertexBuffer->SetLayout({
            { ShaderDataType::Float3,   "a_WorldPosition" },
            { ShaderDataType::Float2,   "a_LocalPosition" },
            { ShaderDataType::Float4,   "a_Color"         },
            { ShaderDataType::Float,    "a_Thickness"     },
            { ShaderDataType::Float,    "a_Fade"          }
        });
        s_Data.CircleVertexArray->AddVertexBuffer(s_Data.CircleVertexBuffer);
        s_Data.CircleVertexArray->SetIndexBuffer(quadIB); /* Reuse quad indexes (same geometry) */
        s_Data.CircleVertexBufferBase = new CircleVertex[s_Data.MaxCircleVertices];

        s_Data.CircleShader = Shader::Create(LV_ASSET_DIR"/shaders/Renderer2D_Circle.lvglsl");
        s_Data.CircleShader->BindUniformBuffer(Renderer::GetSceneUniformBufferId(), "CameraUniform");

        // Color hyperbola
        s_Data.HyperbolaVertexArray = VertexArray::Create();

        float hyperbolaVertices[3 * 3] = {
            0.f,  0.f,  0.f,
           -1.f,  1.f,  0.f,
           -1.f, -1.f,  0.f
        };
        Ref<VertexBuffer> hyperbolaVB = VertexBuffer::Create(hyperbolaVertices, sizeof(hyperbolaVertices));
        hyperbolaVB->SetLayout({
            { ShaderDataType::Float3, "a_Position" }
            });
        s_Data.HyperbolaVertexArray->AddVertexBuffer(hyperbolaVB);

        uint32_t hyperbolaIndices[3] = { 0, 1, 2 };
        Ref<IndexBuffer> hyperbolaIB = IndexBuffer::Create(hyperbolaIndices, std::size(hyperbolaIndices));
        s_Data.HyperbolaVertexArray->SetIndexBuffer(hyperbolaIB);

        s_Data.HyperbolaShader = Shader::Create(LV_ASSET_DIR"/shaders/Hyperbola.lvglsl");
        s_Data.HyperbolaShader->BindUniformBuffer(Renderer::GetSceneUniformBufferId(), "CameraUniform");

        s_OrbitData = new OrbitData();
        s_Data.OrbitUniformBuffer = UniformBuffer::Create((void*)&s_OrbitData, sizeof(OrbitData));
        s_Data.HyperbolaShader->BindUniformBuffer(s_Data.OrbitUniformBuffer->GetRendererId(), "OrbitUniform");

        // Color ellipse
        s_Data.EllipseVertexArray = VertexArray::Create();

        float ellipseVertices[3 * 4] = {
           -0.5f, -0.5f,  0.f,
            0.5f, -0.5f,  0.f,
            0.5f,  0.5f,  0.f,
           -0.5f,  0.5f,  0.f
        };
        Ref<VertexBuffer> ellipseVB = VertexBuffer::Create(ellipseVertices, sizeof(ellipseVertices));
        ellipseVB->SetLayout({
            { ShaderDataType::Float3, "a_Position" }
            });
        s_Data.EllipseVertexArray->AddVertexBuffer(ellipseVB);

        uint32_t ellipseIndices[6] = { 0, 1, 2, 0, 2, 3 };
        Ref<IndexBuffer> ellipseIB = IndexBuffer::Create(ellipseIndices, std::size(ellipseIndices));
        s_Data.EllipseVertexArray->SetIndexBuffer(ellipseIB);

        s_Data.EllipseShader = Shader::Create(LV_ASSET_DIR"/shaders/Ellipse.lvglsl");
        s_Data.EllipseShader->BindUniformBuffer(Renderer::GetSceneUniformBufferId(), "CameraUniform");
        s_Data.EllipseShader->BindUniformBuffer(s_Data.OrbitUniformBuffer->GetRendererId(), "OrbitUniform");
    }


    void Renderer2D::Shutdown()
    {
        LV_PROFILE_FUNCTION();
    }


    void Renderer2D::BeginScene(Camera& camera)
    {
        LV_PROFILE_FUNCTION();

        s_SceneUniformBuffer->UpdateData((void*)camera.GetData(), offsetof(Renderer::SceneData, Renderer::SceneData::CameraData), sizeof(Camera::Data));

        ResetQuadBatch();
        ResetCircleBatch();
    }


    void Renderer2D::EndScene()
    {
        LV_PROFILE_FUNCTION();

        FlushQuads();
        FlushCircles();
    }


    void Renderer2D::FlushQuads()
    {
        LV_PROFILE_FUNCTION();

        if (s_Data.QuadIndexCount == 0) {
            return;
        }

        s_Data.QuadShader->Bind();

        for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
        {
            s_Data.TextureSlots[i]->Bind(i);
        }

        uint32_t dataSize = (uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase;
        s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

        s_Data.QuadVertexArray->Bind(); // TEMPORARY - necessary because DrawEllipse and DrawHyperbola bind their different vertex buffers
        RenderCommand::DrawIndexed(s_Data.QuadVertexArray, s_Data.QuadIndexCount);


        s_Data.Stats.DrawCalls++;
    }


    void Renderer2D::ResetQuadBatch()
    {
        LV_PROFILE_FUNCTION();

        s_Data.QuadIndexCount = 0;
        s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

        s_Data.TextureSlotIndex = 1;
    }


    void Renderer2D::DrawBatchedQuad(const glm::mat4& transform, const Vector4& color, const Vector2* textureCoords, const Vector2& textureScale, const float textureIndex)
    {
        LV_PROFILE_FUNCTION();

        if (s_Data.QuadIndexCount >= s_Data.MaxQuadIndices)
        {
            FlushQuads();
            ResetQuadBatch();
        }

        for (uint32_t i = 0; i < 4; i++)
        {
            s_Data.QuadVertexBufferPtr->Position = glm::vec3(transform * s_Data.QuadVertexPositions[i].glm_vec4()); // TODO : Limnova::Math matrices
            s_Data.QuadVertexBufferPtr->Color = color;
            s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
            s_Data.QuadVertexBufferPtr->TexScale = textureScale;
            s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
            s_Data.QuadVertexBufferPtr++;
        }
        s_Data.QuadIndexCount += 6;


        s_Data.Stats.QuadCount++;
    }


    void Renderer2D::DrawQuad(const glm::mat4& transform, const Vector4& color)
    {
        LV_PROFILE_FUNCTION();

        const Vector2 textureCoords[4]{ { 0.f, 0.f }, { 1.f, 0.f }, { 1.f, 1.f }, { 0.f, 1.f } };
        const Vector2 textureScale{ 1.f, 1.f };
        const float textureIndex = 0.f;

        DrawBatchedQuad(transform, color, textureCoords, textureScale, textureIndex);
    }


    void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, const Vector4& tint, const Vector2& textureScale)
    {
        const Vector2 textureCoords[4]{ { 0.f, 0.f }, { 1.f, 0.f }, { 1.f, 1.f }, { 0.f, 1.f } };
        float textureIndex = 0.f;
        for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
        {
            if (*s_Data.TextureSlots[i].get() == *texture.get())
            {
                textureIndex = (float)i;
            }
        }
        if (textureIndex == 0.f)
        {
            textureIndex = (float)s_Data.TextureSlotIndex;
            s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
            s_Data.TextureSlotIndex++;
        }

        DrawBatchedQuad(transform, tint, textureCoords, textureScale, textureIndex);
    }


    void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<SubTexture2D>& subTexture, const Vector4& tint, const Vector2& textureScale)
    {
        const Ref<Texture2D>& texture = subTexture->GetTexture();
        const Vector2* textureCoords = subTexture->GetTexCoords();
        float textureIndex = 0.f;
        for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
        {
            if (*s_Data.TextureSlots[i].get() == *texture.get())
            {
                textureIndex = (float)i;
            }
        }
        if (textureIndex == 0.f)
        {
            textureIndex = (float)s_Data.TextureSlotIndex;
            s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
            s_Data.TextureSlotIndex++;
        }

        DrawBatchedQuad(transform, tint, textureCoords, textureScale, textureIndex);
    }


    void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, const Vector4& color)
    {
        LV_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.f), (glm::vec3)position);
        transform = glm::scale(transform, glm::vec3((glm::vec2)size, 1.f));

        DrawQuad(transform, color);
    }


    void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Vector4& color)
    {
        DrawQuad({ position.x, position.y, 0.f }, size, color);
    }


    void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, const Ref<Texture2D>& texture, const Vector4& tint, const Vector2& textureScale)
    {
        LV_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.f), (glm::vec3)position);
        transform = glm::scale(transform, glm::vec3((glm::vec2)size, 1.f));

        DrawQuad(transform, texture, tint, textureScale);
    }


    void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Ref<Texture2D>& texture, const Vector4& tint, const Vector2& textureScale)
    {
        DrawQuad({ position.x, position.y, 0.f }, size, texture, tint, textureScale);
    }


    void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, const Ref<SubTexture2D>& subTexture, const Vector4& tint, const Vector2& textureScale)
    {
        LV_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.f), (glm::vec3)position);
        transform = glm::scale(transform, glm::vec3((glm::vec2)size, 1.f));

        DrawQuad(transform, subTexture, tint, textureScale);
    }


    void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Ref<SubTexture2D>& subTexture, const Vector4& tint, const Vector2& textureScale)
    {
        DrawQuad({ position.x, position.y, 0.f }, size, subTexture, tint, textureScale);
    }


    void Renderer2D::DrawRotatedQuad(const Vector3& position, const Vector2& size, const float rotation, const Vector4& color)
    {
        LV_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.f), (glm::vec3)position);
        transform = glm::rotate(transform, rotation, { 0.f, 0.f, 1.f });
        transform = glm::scale(transform, glm::vec3((glm::vec2)size, 1.f));

        const Vector2 textureCoords[4]{ { 0.f, 0.f }, { 1.f, 0.f }, { 1.f, 1.f }, { 0.f, 1.f } };
        const Vector2 textureScale{ 1.f, 1.f };
        const float textureIndex = 0.f;

        DrawBatchedQuad(transform, color, textureCoords, textureScale, textureIndex);
    }


    void Renderer2D::DrawRotatedQuad(const Vector2& position, const Vector2& size, const float rotation, const Vector4& color)
    {
        DrawRotatedQuad({ position.x, position.y, 0.f }, size, rotation, color);
    }


    void Renderer2D::DrawRotatedQuad(const Vector3& position, const Vector2& size, const float rotation, const Ref<Texture2D>& texture, const Vector4& tint, const Vector2& textureScale)
    {
        LV_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.f), (glm::vec3)position);
        transform = glm::rotate(transform, rotation, { 0.f, 0.f, 1.f });
        transform = glm::scale(transform, glm::vec3((glm::vec2)size, 1.f));

        const Vector2 textureCoords[4]{ { 0.f, 0.f }, { 1.f, 0.f }, { 1.f, 1.f }, { 0.f, 1.f } };
        float textureIndex = 0.f;
        for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
        {
            if (*s_Data.TextureSlots[i].get() == *texture.get())
            {
                textureIndex = (float)i;
            }
        }
        if (textureIndex == 0.f)
        {
            textureIndex = (float)s_Data.TextureSlotIndex;
            s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
            s_Data.TextureSlotIndex++;
        }

        DrawBatchedQuad(transform, tint, textureCoords, textureScale, textureIndex);
    }


    void Renderer2D::DrawRotatedQuad(const Vector2& position, const Vector2& size, const float rotation, const Ref<Texture2D>& texture, const Vector4& tint, const Vector2& textureScale)
    {
        DrawRotatedQuad({ position.x, position.y, 0.f }, size, rotation, texture, tint, textureScale);
    }


    void Renderer2D::DrawRotatedQuad(const Vector3& position, const Vector2& size, const float rotation, const Ref<SubTexture2D>& subTexture, const Vector4& tint, const Vector2& textureScale)
    {
        LV_PROFILE_FUNCTION();

        const Ref<Texture2D>& texture = subTexture->GetTexture();

        glm::mat4 transform = glm::translate(glm::mat4(1.f), (glm::vec3)position);
        transform = glm::rotate(transform, rotation, { 0.f, 0.f, 1.f });
        transform = glm::scale(transform, glm::vec3((glm::vec2)size, 1.f));

        const Vector2* textureCoords = subTexture->GetTexCoords();
        float textureIndex = 0.f;
        for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
        {
            if (*s_Data.TextureSlots[i].get() == *texture.get())
            {
                textureIndex = (float)i;
            }
        }
        if (textureIndex == 0.f)
        {
            textureIndex = (float)s_Data.TextureSlotIndex;
            s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
            s_Data.TextureSlotIndex++;
        }

        DrawBatchedQuad(transform, tint, textureCoords, textureScale, textureIndex);
    }


    void Renderer2D::DrawRotatedQuad(const Vector2& position, const Vector2& size, const float rotation, const Ref<SubTexture2D>& subTexture, const Vector4& tint, const Vector2& textureScale)
    {
        DrawRotatedQuad({ position.x, position.y, 0.f }, size, rotation, subTexture, tint, textureScale);
    }


    void Renderer2D::FlushCircles()
    {
        LV_PROFILE_FUNCTION();

        if (s_Data.CircleIndexCount == 0) {
            return;
        }

        s_Data.CircleShader->Bind();

        uint32_t dataSize = (uint8_t*)s_Data.CircleVertexBufferPtr - (uint8_t*)s_Data.CircleVertexBufferBase;
        s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, dataSize);

        s_Data.CircleVertexArray->Bind(); // TEMPORARY - necessary because DrawEllipse and DrawHyperbola bind their different vertex buffers
        RenderCommand::DrawIndexed(s_Data.CircleVertexArray, s_Data.CircleIndexCount);


        s_Data.Stats.DrawCalls++;
    }


    void Renderer2D::ResetCircleBatch()
    {
        LV_PROFILE_FUNCTION();

        s_Data.CircleIndexCount = 0;
        s_Data.CircleVertexBufferPtr = s_Data.CircleVertexBufferBase;
    }


    void Renderer2D::DrawCircle(const Matrix4& transform, const Vector4& color, float thickness, float fade)
    {
        LV_PROFILE_FUNCTION();

        if (s_Data.CircleIndexCount >= s_Data.MaxCircleIndices)
        {
            FlushCircles();
            ResetCircleBatch();
        }

        for (uint32_t i = 0; i < 4; i++)
        {
            s_Data.CircleVertexBufferPtr->WorldPosition = (transform * s_Data.QuadVertexPositions[i]).XYZ();
            s_Data.CircleVertexBufferPtr->LocalPosition.x = 2.f * s_Data.QuadVertexPositions[i].x;
            s_Data.CircleVertexBufferPtr->LocalPosition.y = 2.f * s_Data.QuadVertexPositions[i].y;
            s_Data.CircleVertexBufferPtr->Color = color;
            s_Data.CircleVertexBufferPtr->Thickness = thickness;
            s_Data.CircleVertexBufferPtr->Fade = fade;
            s_Data.CircleVertexBufferPtr++;
        }
        s_Data.CircleIndexCount += 6;


        s_Data.Stats.QuadCount++;
    }


    void Renderer2D::DrawCircle(const Vector3& origin, float radius, const Vector4& color, float thickness, float fade)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.f), (glm::vec3)origin);
        transform = glm::scale(transform, glm::vec3(glm::vec2{2.f * radius}, 1.f));

        DrawCircle(transform, color, thickness, fade);
    }


    void Renderer2D::DrawLine(const Vector2& start, const Vector2& end, const float thickness, const Vector4& color, int layer)
    {
        LV_PROFILE_FUNCTION();

        auto line = end - start;
        auto midpoint = start + (0.5f * line);
        Vector2 dimensions = { sqrt(line.SqrMagnitude()) + thickness, thickness };
        float rotation = atanf(line.y / line.x);

        DrawRotatedQuad({ midpoint, (float)layer }, dimensions, rotation, color);
    }


    void Renderer2D::DrawEllipse(const Vector2& centre, const float rotation, const float semiMajorAxis, const float semiMinorAxis, const Vector2& escapePointFromCentre, const float thickness, const Vector4& color, int layer)
    {
        LV_PROFILE_FUNCTION();

        bool escapes = !(escapePointFromCentre.y == 0);
        s_OrbitData->XOffset = escapes ? 0.5f * (semiMajorAxis + escapePointFromCentre.x) : 0.f;
        s_OrbitData->XEscapeTangent = -abs(escapePointFromCentre.y * semiMajorAxis * semiMajorAxis / (semiMinorAxis * semiMinorAxis * escapePointFromCentre.x));
        float drawRadius = thickness / 2.f;
        float xLimit = escapes ? semiMajorAxis - escapePointFromCentre.x + thickness : 2.f * semiMajorAxis + thickness;
        float yLimit = escapes && escapePointFromCentre.x > 0 ? 2.f * escapePointFromCentre.y + thickness : 2.f * semiMinorAxis + thickness;

        glm::mat4 triangleTransform = glm::translate(glm::mat4(1.f), { (glm::vec2)centre, (float)layer });
        triangleTransform = glm::rotate(triangleTransform, rotation, { 0.f, 0.f, 1.f });
        triangleTransform = glm::translate(triangleTransform, { s_OrbitData->XOffset, 0.f, 0.f });
        triangleTransform = glm::scale(triangleTransform, glm::vec3(xLimit, yLimit, 1.f));
        s_Data.EllipseShader->SetMat4("u_Transform", triangleTransform);

        s_OrbitData->XLimit = xLimit;
        s_OrbitData->YLimit = yLimit;
        s_OrbitData->XEscape = escapePointFromCentre.x;
        s_OrbitData->YEscape = escapePointFromCentre.y;
        s_OrbitData->SemiMajorAxis = semiMajorAxis;
        s_OrbitData->SemiMinorAxis = semiMinorAxis;
        s_OrbitData->DrawRadius = drawRadius;
        s_Data.OrbitUniformBuffer->UpdateData(s_OrbitData, 0, sizeof(OrbitData));

        s_Data.EllipseShader->SetVec4("u_Color", color);

        s_Data.EllipseVertexArray->Bind();
        RenderCommand::DrawIndexed(s_Data.EllipseVertexArray);
    }


    void Renderer2D::DrawHyperbola(const Vector2& centre, const float rotation, const float semiMajorAxis, const float semiMinorAxis, const Vector2& escapePointFromCentre,
        const float thickness, const Vector4& color, int layer)
    {
        LV_PROFILE_FUNCTION();

        s_OrbitData->XOffset = 0;

        // Determine the coordinates of the top-left corner of the triangle, measured in the hyperbola's coordinate system.
        s_OrbitData->XEscapeTangent = escapePointFromCentre.y * semiMajorAxis * semiMajorAxis / (semiMinorAxis * semiMinorAxis * escapePointFromCentre.x);
        float xEscapeTanNormalised = s_OrbitData->XEscapeTangent / sqrt(s_OrbitData->XEscapeTangent * s_OrbitData->XEscapeTangent + 1.f);
        float drawRadius = thickness / 2.f;
        float xLimit = escapePointFromCentre.x + drawRadius;
        float yLimit = (escapePointFromCentre.y + drawRadius / xEscapeTanNormalised) * xLimit / escapePointFromCentre.x;

        glm::mat4 triangleTransform = glm::translate(glm::mat4(1.f), { (glm::vec2)centre, (float)layer });
        triangleTransform = glm::rotate(triangleTransform, rotation, { 0.f, 0.f, 1.f });
        triangleTransform = glm::scale(triangleTransform, glm::vec3(xLimit, yLimit, 1.f));
        s_Data.HyperbolaShader->SetMat4("u_Transform", triangleTransform);

        s_OrbitData->XLimit = xLimit;
        s_OrbitData->YLimit = yLimit;
        s_OrbitData->XEscape = escapePointFromCentre.x;
        s_OrbitData->YEscape = escapePointFromCentre.y;
        s_OrbitData->SemiMajorAxis = semiMajorAxis;
        s_OrbitData->SemiMinorAxis = semiMinorAxis;
        s_OrbitData->DrawRadius = drawRadius;
        s_Data.OrbitUniformBuffer->UpdateData(s_OrbitData, 0, sizeof(OrbitData));

        s_Data.HyperbolaShader->SetVec4("u_Color", color);

        s_Data.HyperbolaVertexArray->Bind();
        RenderCommand::DrawIndexed(s_Data.HyperbolaVertexArray);
    }


    void Renderer2D::TEMP_BeginEllipses()
    {
        s_Data.EllipseShader->Bind();
    }


    void Renderer2D::TEMP_BeginHyperbolae()
    {
        s_Data.HyperbolaShader->Bind();
    }


    Renderer2D::Statistics& Renderer2D::GetStatistics()
    {
        return s_Data.Stats;
    }

    void Renderer2D::ResetStatistics()
    {
        memset(&s_Data.Stats, 0, sizeof(Statistics));
    }
}
