#include "Renderer2D.h"

#include "VertexArray.h"
#include "Shader.h"


namespace Limnova
{

    struct QuadVertex
    {
        Vector3 Position;
        Vector4 Color;
        Vector2 TexCoord;
        Vector2 TexScale;
        float TexIndex;

        // Editor only
        int EntityId;
    };


    struct CircleVertex
    {
        Vector3 WorldPosition;
        Vector2 LocalPosition;
        Vector4 Color;
        float Thickness;
        float Fade;

        // Editor only
        int EntityId;
    };


    struct EllipseVertex
    {
        Vector3 WorldPosition;
        Vector2 LocalPosition;
        Vector4 Color;
        float SemiMajorAxis;
        float SemiMinorAxis;
        Vector2 CutoffPoint;
        Vector2 CutoffNormal;
        float Thickness;
        float Fade;

        // Editor only
        int EntityId;
    };


    struct HyperbolaVertex
    {
        Vector3 WorldPosition;
        Vector2 LocalPosition;
        Vector4 Color;
        float SemiMajorAxis;
        float SemiMinorAxis;
        Vector2 CutoffPoint;
        Vector2 CutoffNormal;
        float Thickness;
        float Fade;

        // Editor only
        int EntityId;
    };


    struct LineVertex
    {
        Vector3 WorldPosition;
        Vector2 LocalPosition;
        Vector4 Color;
        float Length;
        float Thickness;
        float DashLength;
        float GapLength;

        // Editor only
        int EntityId;
    };


    struct Renderer2DData
    {
        // Camera
        const Camera::Data* CameraData = nullptr;
        Ref<UniformBuffer> SceneUniformBuffer;

        // Quads
        const uint32_t MaxQuads = 4096;
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
        const uint32_t MaxCircles = 4096;
        const uint32_t MaxCircleVertices = MaxCircles * 4;
        const uint32_t MaxCircleIndices = MaxCircles * 6;

        Ref<VertexArray> CircleVertexArray;
        Ref<VertexBuffer> CircleVertexBuffer;
        Ref<Shader> CircleShader;

        uint32_t CircleIndexCount = 0;
        CircleVertex* CircleVertexBufferBase = nullptr;
        CircleVertex* CircleVertexBufferPtr = nullptr;

        // Ellipses
        const uint32_t MaxEllipses = 1024;
        const uint32_t MaxEllipseVertices = MaxEllipses * 4;
        const uint32_t MaxEllipseIndices = MaxEllipses * 6;

        Ref<VertexArray> EllipseVertexArray;
        Ref<VertexBuffer> EllipseVertexBuffer;
        Ref<Shader> EllipseShader;

        uint32_t EllipseIndexCount = 0;
        EllipseVertex* EllipseVertexBufferBase = nullptr;
        EllipseVertex* EllipseVertexBufferPtr = nullptr;

        // Hyperbolas
        const uint32_t MaxHyperbolas = 1024;
        const uint32_t MaxHyperbolaVertices = MaxHyperbolas * 3;
        const uint32_t MaxHyperbolaIndices = MaxHyperbolas * 3;

        Ref<VertexArray> HyperbolaVertexArray;
        Ref<VertexBuffer> HyperbolaVertexBuffer;
        Ref<Shader> HyperbolaShader;

        uint32_t HyperbolaIndexCount = 0;
        HyperbolaVertex* HyperbolaVertexBufferBase = nullptr;
        HyperbolaVertex* HyperbolaVertexBufferPtr = nullptr;

        Vector4 HyperbolaVertexPositions[3];

        // Lines
        const uint32_t MaxLines = 1024;
        const uint32_t MaxLineVertices = MaxLines * 4;
        const uint32_t MaxLineIndices = MaxLines * 6;

        Ref<VertexArray> LineVertexArray;
        Ref<VertexBuffer> LineVertexBuffer;
        Ref<Shader> LineShader;

        uint32_t LineIndexCount = 0;
        LineVertex* LineVertexBufferBase = nullptr;
        LineVertex* LineVertexBufferPtr = nullptr;

        // Orbit resources
        //Ref<VertexArray> HyperbolaVertexArray2;
        //Ref<Shader> HyperbolaShader2;
        //Ref<VertexArray> EllipseVertexArray2;
        //Ref<Shader> EllipseShader2;
        //Ref<UniformBuffer> OrbitUniformBuffer2;
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


    void Renderer2D::Init()
    {
        LV_PROFILE_FUNCTION();

        s_Data.SceneUniformBuffer = UniformBuffer::Create(0, sizeof(Camera::Data));

        // Quads
        s_Data.QuadVertexArray = VertexArray::Create();

        s_Data.QuadVertexBuffer = VertexBuffer::Create(s_Data.MaxQuadVertices * sizeof(QuadVertex));
        s_Data.QuadVertexBuffer->SetLayout({
            { ShaderDataType::Float3,   "a_Position" },
            { ShaderDataType::Float4,   "a_Color"    },
            { ShaderDataType::Float2,   "a_TexCoord" },
            { ShaderDataType::Float2,   "a_TexScale" },
            { ShaderDataType::Float,    "a_TexIndex" },
            { ShaderDataType::Int,      "a_EntityId" }
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

        s_Data.TextureSlots[0] = s_Data.WhiteTexture;

        s_Data.QuadVertexPositions[0] = {-0.5f,-0.5f, 0.f, 1.f  };
        s_Data.QuadVertexPositions[1] = { 0.5f,-0.5f, 0.f, 1.f  };
        s_Data.QuadVertexPositions[2] = { 0.5f, 0.5f, 0.f, 1.f  };
        s_Data.QuadVertexPositions[3] = {-0.5f, 0.5f, 0.f, 1.f  };

        // Circles
        s_Data.CircleVertexArray = VertexArray::Create();

        s_Data.CircleVertexBuffer = VertexBuffer::Create(s_Data.MaxCircleVertices * sizeof(CircleVertex));
        s_Data.CircleVertexBuffer->SetLayout({
            { ShaderDataType::Float3,   "a_WorldPosition" },
            { ShaderDataType::Float2,   "a_LocalPosition" },
            { ShaderDataType::Float4,   "a_Color"         },
            { ShaderDataType::Float,    "a_Thickness"     },
            { ShaderDataType::Float,    "a_Fade"          },
            { ShaderDataType::Int,      "a_EntityId"      }
        });
        s_Data.CircleVertexArray->AddVertexBuffer(s_Data.CircleVertexBuffer);
        s_Data.CircleVertexArray->SetIndexBuffer(quadIB); /* Reuse quad indexes (same geometry) */
        s_Data.CircleVertexBufferBase = new CircleVertex[s_Data.MaxCircleVertices];

        s_Data.CircleShader = Shader::Create(LV_ASSET_DIR"/shaders/Renderer2D_Circle.lvglsl");

        // Ellipses
        s_Data.EllipseVertexArray = VertexArray::Create();

        s_Data.EllipseVertexBuffer = VertexBuffer::Create(s_Data.MaxEllipseVertices * sizeof(EllipseVertex));
        s_Data.EllipseVertexBuffer->SetLayout({
            { ShaderDataType::Float3,   "a_WorldPosition"   },
            { ShaderDataType::Float2,   "a_LocalPosition"   },
            { ShaderDataType::Float4,   "a_Color"           },
            { ShaderDataType::Float,    "a_SemiMajorAxis"   },
            { ShaderDataType::Float,    "a_SemiMinorAxis"   },
            { ShaderDataType::Float2,   "a_CutoffPoint"     },
            { ShaderDataType::Float2,   "a_CutoffNormal"    },
            { ShaderDataType::Float,    "a_Thickness"       },
            { ShaderDataType::Float,    "a_Fade"            },
            { ShaderDataType::Int,      "a_EntityId"        }
        });
        s_Data.EllipseVertexArray->AddVertexBuffer(s_Data.EllipseVertexBuffer);
        s_Data.EllipseVertexArray->SetIndexBuffer(quadIB); /* Reuse quad indexes (same geometry) */
        s_Data.EllipseVertexBufferBase = new EllipseVertex[s_Data.MaxEllipseVertices];

        //s_Data.EllipseShader = Shader::Create(LV_ASSET_DIR"/shaders/Renderer2D_Ellipse.lvglsl");
        s_Data.EllipseShader = Shader::Create(LV_ASSET_DIR"/shaders/Orbital_Ellipse.lvglsl");

        // Hyperbolas
        s_Data.HyperbolaVertexArray = VertexArray::Create();

        s_Data.HyperbolaVertexBuffer = VertexBuffer::Create(s_Data.MaxHyperbolaVertices * sizeof(HyperbolaVertex));
        s_Data.HyperbolaVertexBuffer->SetLayout({
            { ShaderDataType::Float3,   "a_WorldPosition"   },
            { ShaderDataType::Float2,   "a_LocalPosition"   },
            { ShaderDataType::Float4,   "a_Color"           },
            { ShaderDataType::Float,    "a_SemiMajorAxis"   },
            { ShaderDataType::Float,    "a_SemiMinorAxis"   },
            { ShaderDataType::Float2,   "a_CutoffPoint"     },
            { ShaderDataType::Float2,   "a_CutoffNormal"    },
            { ShaderDataType::Float,    "a_Thickness"       },
            { ShaderDataType::Float,    "a_Fade"            },
            { ShaderDataType::Int,      "a_EntityId"        }
            });
        s_Data.HyperbolaVertexArray->AddVertexBuffer(s_Data.HyperbolaVertexBuffer);

        uint32_t* hyperbolaIndices = new uint32_t[s_Data.MaxHyperbolaIndices];
        for (uint32_t i = 0; i < s_Data.MaxHyperbolaIndices; i++) {
            hyperbolaIndices[i] = i;
        }
        Ref<IndexBuffer> hyperbolaIB = IndexBuffer::Create(hyperbolaIndices, s_Data.MaxHyperbolaIndices);
        s_Data.HyperbolaVertexArray->SetIndexBuffer(hyperbolaIB);
        delete[] hyperbolaIndices;

        s_Data.HyperbolaVertexArray->SetIndexBuffer(hyperbolaIB);
        s_Data.HyperbolaVertexBufferBase = new HyperbolaVertex[s_Data.MaxHyperbolaVertices];
        s_Data.HyperbolaShader = Shader::Create(LV_ASSET_DIR"/shaders/Orbital_Hyperbola.lvglsl");

        s_Data.HyperbolaVertexPositions[0] = { 0.f,  0.f,  0.f,  1.f  };
        s_Data.HyperbolaVertexPositions[1] = {-1.f,  1.f,  0.f,  1.f  };
        s_Data.HyperbolaVertexPositions[2] = {-1.f, -1.f,  0.f,  1.f  };

        // Lines
        s_Data.LineVertexArray = VertexArray::Create();

        s_Data.LineVertexBuffer = VertexBuffer::Create(s_Data.MaxLineVertices * sizeof(LineVertex));
        s_Data.LineVertexBuffer->SetLayout({
            { ShaderDataType::Float3,   "a_WorldPosition" },
            { ShaderDataType::Float2,   "a_LocalPosition" },
            { ShaderDataType::Float4,   "a_Color"         },
            { ShaderDataType::Float,    "a_Length"        },
            { ShaderDataType::Float,    "a_Thickness"     },
            { ShaderDataType::Float,    "a_DashLength"    },
            { ShaderDataType::Float,    "a_GapLength"     },
            { ShaderDataType::Int,      "a_EntityId"      }
        });
        s_Data.LineVertexArray->AddVertexBuffer(s_Data.LineVertexBuffer);
        s_Data.LineVertexArray->SetIndexBuffer(quadIB); /* Reuse quad indexes (same geometry) */
        s_Data.LineVertexBufferBase = new LineVertex[s_Data.MaxLineVertices];

        s_Data.LineShader = Shader::Create(LV_ASSET_DIR"/shaders/Renderer2D_Line.lvglsl");

#ifdef EXCLUDED
        // Color hyperbola
        s_Data.HyperbolaVertexArray2 = VertexArray::Create();
        
        float hyperbolaVertices[3 * 3] = {
            0.f,  0.f,  0.f,
           -1.f,  1.f,  0.f,
           -1.f, -1.f,  0.f
        };
        Ref<VertexBuffer> hyperbolaVB = VertexBuffer::Create(hyperbolaVertices, sizeof(hyperbolaVertices));
        hyperbolaVB->SetLayout({
            { ShaderDataType::Float3, "a_Position" }
            });
        s_Data.HyperbolaVertexArray2->AddVertexBuffer(hyperbolaVB);
        
        uint32_t hyperbolaIndices[3] = { 0, 1, 2 };
        Ref<IndexBuffer> hyperbolaIB = IndexBuffer::Create(hyperbolaIndices, std::size(hyperbolaIndices));
        s_Data.HyperbolaVertexArray2->SetIndexBuffer(hyperbolaIB);
        
        s_Data.HyperbolaShader2 = Shader::Create(LV_ASSET_DIR"/shaders/Hyperbola.lvglsl");
        
        s_OrbitData = new OrbitData();
        s_Data.OrbitUniformBuffer2 = UniformBuffer::Create(1, sizeof(OrbitData));
        
        // Color ellipse
        s_Data.EllipseVertexArray2 = VertexArray::Create();
        
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
        s_Data.EllipseVertexArray2->AddVertexBuffer(ellipseVB);
        
        uint32_t ellipseIndices[6] = { 0, 1, 2, 0, 2, 3 };
        Ref<IndexBuffer> ellipseIB = IndexBuffer::Create(ellipseIndices, std::size(ellipseIndices));
        s_Data.EllipseVertexArray2->SetIndexBuffer(ellipseIB);
        
        s_Data.EllipseShader2 = Shader::Create(LV_ASSET_DIR"/shaders/Ellipse.lvglsl");
#endif
    }


    void Renderer2D::Shutdown()
    {
        LV_PROFILE_FUNCTION();
    }


    void Renderer2D::BeginScene(Camera& camera)
    {
        LV_PROFILE_FUNCTION();

        s_Data.SceneUniformBuffer->UpdateData((void*)camera.GetData(), offsetof(Renderer::SceneData, Renderer::SceneData::CameraData), sizeof(Camera::Data));
        s_Data.CameraData = camera.GetData();

        ResetQuadBatch();
        ResetCircleBatch();
        ResetEllipseBatch();
        ResetHyperbolaBatch();
        ResetLineBatch();
    }


    void Renderer2D::EndScene()
    {
        LV_PROFILE_FUNCTION();

        FlushQuads();
        FlushCircles();
        FlushEllipses();
        FlushHyperbolas();
        FlushLines();
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


    void Renderer2D::DrawBatchedQuad(const Matrix4& transform, const Vector4& color, const Vector2* textureCoords, const Vector2& textureScale, const float textureIndex, int entityId)
    {
        LV_PROFILE_FUNCTION();

        if (s_Data.QuadIndexCount >= s_Data.MaxQuadIndices)
        {
            FlushQuads();
            ResetQuadBatch();
        }

        for (uint32_t i = 0; i < 4; i++)
        {
            s_Data.QuadVertexBufferPtr->Position = (transform * s_Data.QuadVertexPositions[i]).XYZ();
            s_Data.QuadVertexBufferPtr->Color = color;
            s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
            s_Data.QuadVertexBufferPtr->TexScale = textureScale;
            s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
            s_Data.QuadVertexBufferPtr->EntityId = entityId;
            s_Data.QuadVertexBufferPtr++;
        }
        s_Data.QuadIndexCount += 6;


        s_Data.Stats.QuadCount++;
    }


    void Renderer2D::DrawQuad(const Matrix4& transform, const Vector4& color, int entityId)
    {
        LV_PROFILE_FUNCTION();

        const Vector2 textureCoords[4]{ { 0.f, 0.f }, { 1.f, 0.f }, { 1.f, 1.f }, { 0.f, 1.f } };
        const Vector2 textureScale{ 1.f, 1.f };
        const float textureIndex = 0;

        DrawBatchedQuad(transform, color, textureCoords, textureScale, textureIndex, entityId);
    }


    void Renderer2D::DrawQuad(const Matrix4& transform, const Ref<Texture2D>& texture, const Vector4& tint, const Vector2& textureScale)
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


    void Renderer2D::DrawQuad(const Matrix4& transform, const Ref<SubTexture2D>& subTexture, const Vector4& tint, const Vector2& textureScale)
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
        const float textureIndex = 0;

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


    // Circles /////////////////////////////

    void Renderer2D::FlushCircles()
    {
        LV_PROFILE_FUNCTION();

        if (s_Data.CircleIndexCount == 0) {
            return;
        }

        s_Data.CircleShader->Bind();

        uint32_t dataSize = (uint8_t*)s_Data.CircleVertexBufferPtr - (uint8_t*)s_Data.CircleVertexBufferBase;
        s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, dataSize);

        RenderCommand::DrawIndexed(s_Data.CircleVertexArray, s_Data.CircleIndexCount);


        s_Data.Stats.DrawCalls++;
    }


    void Renderer2D::ResetCircleBatch()
    {
        LV_PROFILE_FUNCTION();

        s_Data.CircleIndexCount = 0;
        s_Data.CircleVertexBufferPtr = s_Data.CircleVertexBufferBase;
    }


    void Renderer2D::DrawCircle(const Matrix4& transform, const Vector4& color, float thickness, float fade, int entityId)
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
            s_Data.CircleVertexBufferPtr->EntityId = entityId;
            s_Data.CircleVertexBufferPtr++;
        }
        s_Data.CircleIndexCount += 6;


        s_Data.Stats.QuadCount++;
    }


    void Renderer2D::DrawCircle(const Vector3& origin, float radius, const Vector4& color, float thickness, float fade, int entityId)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.f), (glm::vec3)origin);
        transform = glm::scale(transform, glm::vec3(glm::vec2{2.f * radius}, 1.f));

        DrawCircle(transform, color, thickness, fade, entityId);
    }


    // Ellipses ////////////////////////////

    void Renderer2D::FlushEllipses()
    {
        LV_PROFILE_FUNCTION();

        if (s_Data.EllipseIndexCount == 0) {
            return;
        }

        s_Data.EllipseShader->Bind();

        uint32_t dataSize = (uint8_t*)s_Data.EllipseVertexBufferPtr - (uint8_t*)s_Data.EllipseVertexBufferBase;
        s_Data.EllipseVertexBuffer->SetData(s_Data.EllipseVertexBufferBase, dataSize);

        RenderCommand::DrawIndexed(s_Data.EllipseVertexArray, s_Data.EllipseIndexCount);


        s_Data.Stats.DrawCalls++;
    }


    void Renderer2D::ResetEllipseBatch()
    {
        LV_PROFILE_FUNCTION();

        s_Data.EllipseIndexCount = 0;
        s_Data.EllipseVertexBufferPtr = s_Data.EllipseVertexBufferBase;
    }


    void Renderer2D::DrawBatchedEllipse(const Matrix4& transform, float majorMinorAxisRatio, Vector2 cutoffPoint, Vector2 cutoffNormal, const Vector4& color, float thickness, float fade, int entityId)
    {
        LV_PROFILE_FUNCTION();

        if (s_Data.EllipseIndexCount >= s_Data.MaxEllipseIndices)
        {
            FlushEllipses();
            ResetEllipseBatch();
        }

        for (uint32_t i = 0; i < 4; i++)
        {
            /* for Orbital_Ellipse.lvglsl */
            float vertexPadding = thickness / 2.f;
            Vector4 vertexPositionPadding = {
                (i == 0 || i == 3) ? -vertexPadding : vertexPadding,
                (i == 0 || i == 1) ? -vertexPadding : vertexPadding,
                0.f, 0.f
            };
            s_Data.EllipseVertexBufferPtr->WorldPosition = (transform * (s_Data.QuadVertexPositions[i] + vertexPositionPadding)).XYZ();
            s_Data.EllipseVertexBufferPtr->LocalPosition.x = 2.f * (s_Data.QuadVertexPositions[i].x + vertexPositionPadding.x) * majorMinorAxisRatio;
            s_Data.EllipseVertexBufferPtr->LocalPosition.y = 2.f * (s_Data.QuadVertexPositions[i].y + vertexPositionPadding.y);
            s_Data.EllipseVertexBufferPtr->Color = color;
            s_Data.EllipseVertexBufferPtr->SemiMajorAxis = majorMinorAxisRatio;
            s_Data.EllipseVertexBufferPtr->SemiMinorAxis = 1.0f;
            s_Data.EllipseVertexBufferPtr->CutoffPoint = cutoffPoint;
            s_Data.EllipseVertexBufferPtr->CutoffNormal = cutoffNormal;
            s_Data.EllipseVertexBufferPtr->Thickness = thickness;
            s_Data.EllipseVertexBufferPtr->Fade = fade;
            s_Data.EllipseVertexBufferPtr->EntityId = entityId;
            s_Data.EllipseVertexBufferPtr++;
        }
        s_Data.EllipseIndexCount += 6;


        s_Data.Stats.QuadCount++;
    }


    void Renderer2D::DrawEllipse(const Matrix4& transform, float majorMinorAxisRatio, const Vector4& color, float thickness, float fade, int entityId)
    {
        Vector2 cutoffPoint = { 0.f };
        Vector2 cutoffNormal = { 0.f };
        DrawBatchedEllipse(transform, majorMinorAxisRatio, cutoffPoint, cutoffNormal, color, thickness, fade, entityId);
    }


    void Renderer2D::DrawEllipse(const Vector3& centre, const Quaternion& orientation, float semiMajorAxis, float semiMinorAxis, const Vector4& color, float thickness, float fade, int entityId)
    {
        Matrix4 transform = glm::translate(glm::mat4(1.f), (glm::vec3)centre);
        transform = Matrix4(orientation) * transform;
        transform = glm::scale((glm::mat4)transform, glm::vec3(glm::vec2{ 2.f * semiMajorAxis, 2.f * semiMinorAxis }, 0.f));

        Vector2 cutoffPoint = { 0.f };
        Vector2 cutoffNormal = { 0.f };
        DrawBatchedEllipse(transform, semiMajorAxis / semiMinorAxis, cutoffPoint, cutoffNormal, color, thickness, fade, entityId);
    }


    void Renderer2D::DrawOrbitalEllipse(const Vector3& center, const Quaternion& orientation, const OrbitalComponent& component, const Vector4& color, float thickness, float fade, int entityId)
    {
        auto& orbit = component.Object.GetOrbit();
        auto& elems = orbit.Elements;

        Matrix4 transform = glm::translate(glm::mat4(1.f), (glm::vec3)center);
        transform = transform * Matrix4(orientation);
        transform = glm::scale((glm::mat4)transform, glm::vec3(glm::vec2{ 2.f * elems.SemiMajor + thickness, 2.f * elems.SemiMinor + thickness }, 0.f));

        Vector2 cutoffPoint = { 0.f }; /* the point on the orbit path above the x-axis (positive y-component) at which the orbit should stop being drawn */
        Vector2 cutoffNormal = { 0.f }; /* the normal to the the cutoff line, equal to the unit direction vector of the orbit velocity at the cutoff point */
        if (component.Object.IsDynamic())
        {
            if (orbit.TaExit < PI2f) // TEMP ! will 
            {
                cutoffPoint = { OrbitalPhysics::kLocalSpaceEscapeRadius * cosf(orbit.TaExit) - elems.C, /* subtract center's x-offset to convert x-component to the perifocal frame */
                    OrbitalPhysics::kLocalSpaceEscapeRadius * sinf(orbit.TaExit) };

                // Compute cutoff normal from the orbit velocity at the cutoff point
                cutoffNormal.x = -sinf(orbit.TaExit);
                cutoffNormal.y = elems.E + cosf(orbit.TaExit);
                cutoffNormal.Normalize();
            }
        }

        // Submit to batch
        if (s_Data.EllipseIndexCount >= s_Data.MaxEllipseIndices)
        {
            FlushEllipses();
            ResetEllipseBatch();
        }

        for (uint32_t i = 0; i < 4; i++)
        {
            float padX = (i == 0 || i == 3) ? -thickness / 2.f : thickness / 2.f;
            float padY = (i == 0 || i == 1) ? -thickness / 2.f : thickness / 2.f;

            s_Data.EllipseVertexBufferPtr->WorldPosition = (transform * s_Data.QuadVertexPositions[i]).XYZ();
            s_Data.EllipseVertexBufferPtr->LocalPosition.x = s_Data.QuadVertexPositions[i].x * 2.f * elems.SemiMajor + padX;
            s_Data.EllipseVertexBufferPtr->LocalPosition.y = s_Data.QuadVertexPositions[i].y * 2.f * elems.SemiMinor + padY;
            s_Data.EllipseVertexBufferPtr->Color = color;
            s_Data.EllipseVertexBufferPtr->SemiMajorAxis = elems.SemiMajor;
            s_Data.EllipseVertexBufferPtr->SemiMinorAxis = elems.SemiMinor;
            s_Data.EllipseVertexBufferPtr->CutoffPoint = cutoffPoint;
            s_Data.EllipseVertexBufferPtr->CutoffNormal = cutoffNormal;
            s_Data.EllipseVertexBufferPtr->Thickness = thickness;
            s_Data.EllipseVertexBufferPtr->Fade = fade;
            s_Data.EllipseVertexBufferPtr->EntityId = entityId;
            s_Data.EllipseVertexBufferPtr++;
        }
        s_Data.EllipseIndexCount += 6;


        s_Data.Stats.QuadCount++;
    }


    // Hyperbolas //

    void Renderer2D::FlushHyperbolas()
    {
        LV_PROFILE_FUNCTION();

        if (s_Data.HyperbolaIndexCount == 0) {
            return;
        }

        s_Data.HyperbolaShader->Bind();

        uint32_t dataSize = (uint8_t*)s_Data.HyperbolaVertexBufferPtr - (uint8_t*)s_Data.HyperbolaVertexBufferBase;
        s_Data.HyperbolaVertexBuffer->SetData(s_Data.HyperbolaVertexBufferBase, dataSize);

        RenderCommand::DrawIndexed(s_Data.HyperbolaVertexArray, s_Data.HyperbolaIndexCount);


        s_Data.Stats.DrawCalls++;
    }


    void Renderer2D::ResetHyperbolaBatch()
    {
        LV_PROFILE_FUNCTION();

        s_Data.HyperbolaIndexCount = 0;
        s_Data.HyperbolaVertexBufferPtr = s_Data.HyperbolaVertexBufferBase;
    }


    void Renderer2D::DrawOrbitalHyperbola(const Vector3& center, const Quaternion& orientation, const OrbitalComponent& component, const Vector4& color, float thickness, float fade, int entityId)
    {
        auto& orbit = component.Object.GetOrbit();
        auto& elems = orbit.Elements;
        LV_CORE_ASSERT(elems.Type == OrbitalPhysics::OrbitType::Hyperbola, "Orbit must be hyperbolic!");

        Vector2 cutoffPoint /* the point on the orbit path above the x-axis (positive y-component) at which the orbit should stop being drawn */
            { OrbitalPhysics::kLocalSpaceEscapeRadius * cosf(orbit.TaExit) - elems.C, /* subtract center's x-offset to convert x-component to the perifocal frame */
                OrbitalPhysics::kLocalSpaceEscapeRadius * sinf(orbit.TaExit)};
        Vector2 cutoffNormal = { 0.f }; /* the normal to the cutoff line, equal to the unit direction vector of the orbit velocity at the cutoff point */

        // Compute cutoff normal from the orbit velocity at the cutoff point
        cutoffNormal.x = -sinf(orbit.TaExit);
        cutoffNormal.y = elems.E + cosf(orbit.TaExit);
        cutoffNormal.Normalize();

        // Transform
        Matrix4 transform = glm::translate(glm::mat4(1.f), (glm::vec3)center);
        transform = transform * Matrix4(orientation);

        float triangleMaxX = abs(cutoffPoint.x) + thickness;
        float triangleMaxY = triangleMaxX * elems.SemiMinor / elems.SemiMajor; /* y-value of hyperbola's asymptote at x = triangleMaxX plus  */
        transform = glm::scale((glm::mat4)transform, glm::vec3{ glm::vec2{ triangleMaxX, triangleMaxY }, 0.f });

        // Submit to batch
        if (s_Data.HyperbolaIndexCount >= s_Data.MaxHyperbolaIndices)
        {
            FlushHyperbolas();
            ResetHyperbolaBatch();
        }

        for (uint32_t i = 0; i < 3; i++)
        {
            s_Data.HyperbolaVertexBufferPtr->WorldPosition = (transform * s_Data.HyperbolaVertexPositions[i]).XYZ();
            s_Data.HyperbolaVertexBufferPtr->LocalPosition.x = s_Data.HyperbolaVertexPositions[i].x * triangleMaxX;
            s_Data.HyperbolaVertexBufferPtr->LocalPosition.y = s_Data.HyperbolaVertexPositions[i].y * triangleMaxY;
            s_Data.HyperbolaVertexBufferPtr->Color = color;
            s_Data.HyperbolaVertexBufferPtr->SemiMajorAxis = elems.SemiMajor;
            s_Data.HyperbolaVertexBufferPtr->SemiMinorAxis = elems.SemiMinor;
            s_Data.HyperbolaVertexBufferPtr->CutoffPoint = cutoffPoint;
            s_Data.HyperbolaVertexBufferPtr->CutoffNormal = cutoffNormal;
            s_Data.HyperbolaVertexBufferPtr->Thickness = thickness;
            s_Data.HyperbolaVertexBufferPtr->Fade = fade;
            s_Data.HyperbolaVertexBufferPtr->EntityId = entityId;
            s_Data.HyperbolaVertexBufferPtr++;
        }
        s_Data.HyperbolaIndexCount += 6;
    }


    // Lines ///////////////////////////////

    void Renderer2D::FlushLines()
    {
        LV_PROFILE_FUNCTION();

        if (s_Data.LineIndexCount == 0) {
            return;
        }

        s_Data.LineShader->Bind();

        uint32_t dataSize = (uint8_t*)s_Data.LineVertexBufferPtr - (uint8_t*)s_Data.LineVertexBufferBase;
        s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, dataSize);

        RenderCommand::DrawIndexed(s_Data.LineVertexArray, s_Data.LineIndexCount);


        s_Data.Stats.DrawCalls++;
    }


    void Renderer2D::ResetLineBatch()
    {
        LV_PROFILE_FUNCTION();

        s_Data.LineIndexCount = 0;
        s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;
    }


    void Renderer2D::DrawBatchedLine(const Matrix4& transform, const Vector4& color, float length, float thickness, float dashLength, float gapLength, int entityId)
    {
        LV_PROFILE_FUNCTION();

        if (s_Data.LineIndexCount >= s_Data.MaxLineIndices)
        {
            FlushLines();
            ResetLineBatch();
        }

        for (uint32_t i = 0; i < 4; i++)
        {
            s_Data.LineVertexBufferPtr->WorldPosition = (transform * s_Data.QuadVertexPositions[i]).XYZ();
            s_Data.LineVertexBufferPtr->LocalPosition.x = s_Data.QuadVertexPositions[i].x + 0.5f;   // X range: [0, 1]
            s_Data.LineVertexBufferPtr->LocalPosition.y = s_Data.QuadVertexPositions[i].y;          // Y range: [-0.5, 0.5]
            s_Data.LineVertexBufferPtr->Color = color;
            s_Data.LineVertexBufferPtr->Length = length;
            s_Data.LineVertexBufferPtr->Thickness = thickness;
            s_Data.LineVertexBufferPtr->DashLength = dashLength;
            s_Data.LineVertexBufferPtr->GapLength = gapLength;
            s_Data.LineVertexBufferPtr->EntityId = entityId;
            s_Data.LineVertexBufferPtr++;
        }
        s_Data.LineIndexCount += 6;


        s_Data.Stats.QuadCount++;
    }


    void Renderer2D::DrawLine(const Vector3& start, const Vector3& end, const Vector4& color, float thickness, int entityId)
    {
        Vector3 direction = end - start;
        float length = sqrtf(direction.SqrMagnitude());
        direction = direction / length;
        Vector3 centre = 0.5f * (start + end);

        Matrix4 transform = glm::translate(glm::mat4(1.f), (glm::vec3)centre);

        // Rotations:
        // 
        // Rotate the quad such that the long edges are parallel with the direction vector
        Quaternion rotation = Rotation(Vector3::X(), direction);
        //
        // Rotate the quad about the direction vector such that the quad is 'facing' the camera as much as possible while
        // restricted to the direction axis
        Vector3 initialNormal = rotation.RotateVector(Vector3::Z());
        Vector3 cameraDirection = s_Data.CameraData->Position.Normalized();
        /* The final normal is the camera direction vector projected onto the normal's plane of rotation (the plane
         * perpendicular to the line direction). The rotation angle of the quad around the line direction is then the
         * angle between the initial normal and the final normal, measured counter-clockwise about the line direction.
         * Projection of vector u onto a plane with normal n (assuming all vectors are normalized):
         *  u_plane = u - (u DOT n) * n
         */
        Vector3 finalNormal = cameraDirection - (direction * (cameraDirection.Dot(direction)));
        finalNormal.Normalize();
        float normalRotationAngle = acosf(initialNormal.Dot(finalNormal));
        // Disambiguate quadrant
        if (initialNormal.Cross(finalNormal).Dot(direction) < 0.f) {
            normalRotationAngle = PI2f - normalRotationAngle;
        }
        Quaternion normalRotation{ direction, normalRotationAngle };
        //
        // Apply rotations
        transform = transform * Matrix4(normalRotation);
        transform = transform * Matrix4(rotation);

        transform = glm::scale((glm::mat4)transform, glm::vec3(glm::vec2{ length, thickness }, 0.f));

        float dashLength = length;
        float gapLength = 0.f;

        DrawBatchedLine(transform, color, length, thickness, dashLength, gapLength, entityId);
    }


    void Renderer2D::DrawDashedLine(const Vector3& start, const Vector3& end, const Vector4& color, float thickness, float dashFactor, float gapFactor, int entityId)
    {
        Vector3 direction = end - start;
        float length = sqrtf(direction.SqrMagnitude());
        direction = direction / length;
        Vector3 centre = 0.5f * (start + end);

        Matrix4 transform = glm::translate(glm::mat4(1.f), (glm::vec3)centre);

        // Rotations:
        // 
        // Rotate the quad such that the long edges are parallel with the direction vector
        Quaternion rotation = Rotation(Vector3::X(), direction);
        //
        // Rotate the quad about the direction vector such that the quad is 'facing' the camera as much as possible while
        // restricted to the direction axis
        Vector3 initialNormal = rotation.RotateVector(Vector3::Z());
        Vector3 cameraDirection = s_Data.CameraData->Position.Normalized();
        /* The final normal is the camera direction vector projected onto the normal's plane of rotation (the plane
         * perpendicular to the line direction). The rotation angle of the quad around the line direction is then the
         * angle between the initial normal and the final normal, measured counter-clockwise about the line direction.
         * Projection of vector u onto a plane with normal n (assuming all vectors are normalized):
         *  u_plane = u - (u DOT n) * n
         */
        Vector3 finalNormal = cameraDirection - (direction * (cameraDirection.Dot(direction)));
        finalNormal.Normalize();
        float normalRotationAngle = acosf(initialNormal.Dot(finalNormal));
        // Disambiguate quadrant
        if (initialNormal.Cross(finalNormal).Dot(direction) < 0.f) {
            normalRotationAngle = PI2f - normalRotationAngle;
        }
        Quaternion normalRotation{ direction, normalRotationAngle };
        //
        // Apply rotations
        transform = transform * Matrix4(normalRotation);
        transform = transform * Matrix4(rotation);

        transform = glm::scale((glm::mat4)transform, glm::vec3(glm::vec2{ length, thickness }, 0.f));

        float dashLength = dashFactor * thickness;
        float gapLength = gapFactor * thickness;

        DrawBatchedLine(transform, color, length, thickness, dashLength, gapLength, entityId);
    }


    void Renderer2D::DrawArrow(const Vector3& start, const Vector3& end, const Vector4& color, float thickness, float headSize, int entityId)
    {
        Vector3 direction = end - start;
        float length = sqrtf(direction.SqrMagnitude());
        direction = direction / length;

        static const float overRoot2 = 1.f / sqrtf(2.f);
        float stemLength = length - thickness * overRoot2;

        Vector3 stemCentre = start + 0.5f * stemLength * direction;

        Matrix4 stemTransform = glm::translate(glm::mat4(1.f), (glm::vec3)stemCentre);

        // Rotations:
        // 
        // Rotate the quad such that the long edges are parallel with the direction vector
        Quaternion rotation = Rotation(Vector3::X(), direction);
        //
        // Rotate the quad about the direction vector such that the quad is 'facing' the camera as much as possible while
        // restricted to the direction axis
        Vector3 initialNormal = rotation.RotateVector(Vector3::Z());
        Vector3 cameraDirection = s_Data.CameraData->Position.Normalized();
        /* The final normal is the camera direction vector projected onto the normal's plane of rotation (the plane
         * perpendicular to the line direction). The rotation angle of the quad around the line direction is then the
         * angle between the initial normal and the final normal, measured counter-clockwise about the line direction.
         * Projection of vector u onto a plane with normal n (assuming all vectors are normalized):
         *  u_plane = u - (u DOT n) * n
         */
        Vector3 finalNormal = cameraDirection - (direction * (cameraDirection.Dot(direction)));
        finalNormal.Normalize();
        float normalRotationAngle = acosf(initialNormal.Dot(finalNormal));
        // Disambiguate quadrant
        if (initialNormal.Cross(finalNormal).Dot(direction) < 0.f) {
            normalRotationAngle = PI2f - normalRotationAngle;
        }
        Quaternion normalRotation{ direction, normalRotationAngle };
        //
        // Apply rotations
        stemTransform = stemTransform * Matrix4(normalRotation);
        stemTransform = stemTransform * Matrix4(rotation);

        stemTransform = glm::scale((glm::mat4)stemTransform, glm::vec3(glm::vec2{ stemLength, thickness }, 0.f));

        float dashLength = length;
        float gapLength = 0.f;
        DrawBatchedLine(stemTransform, color, length, thickness, dashLength, gapLength, entityId);

        // Arrowhead arms
        Vector3 arm0Direction = Quaternion(finalNormal, 3.f * PIover4f).RotateVector(direction);
        Vector3 arm1Direction = Quaternion(finalNormal, -3.f * PIover4f).RotateVector(direction);

        Vector3 arm0WidthOffset = thickness / 2.f * arm1Direction;
        Vector3 arm0Start = end + arm0WidthOffset;
        Vector3 arm0End = end + arm0WidthOffset + headSize * arm0Direction;

        Vector3 arm1WidthOffset = thickness / 2.f * arm0Direction;
        Vector3 arm1Start = end + arm1WidthOffset;
        Vector3 arm1End = end + arm1WidthOffset + headSize * arm1Direction;

        DrawLine(arm0Start, arm0End, color, thickness, entityId);
        DrawLine(arm1Start, arm1End, color, thickness, entityId);
    }


    void Renderer2D::DrawDashedArrow(const Vector3& start, const Vector3& end, const Vector4& color, float thickness, float headSize, float dashFactor, float gapFactor, int entityId)
    {
        Vector3 direction = end - start;
        float length = sqrtf(direction.SqrMagnitude());
        direction = direction / length;

        static const float overRoot2 = 1.f / sqrtf(2.f);
        float stemLength = length - thickness * overRoot2;

        Vector3 stemCentre = start + 0.5f * stemLength * direction;

        Matrix4 stemTransform = glm::translate(glm::mat4(1.f), (glm::vec3)stemCentre);

        // Rotations:
        // 
        // Rotate the quad such that the long edges are parallel with the direction vector
        Quaternion rotation = Rotation(Vector3::X(), direction);
        //
        // Rotate the quad about the direction vector such that the quad is 'facing' the camera as much as possible while
        // restricted to the direction axis
        Vector3 initialNormal = rotation.RotateVector(Vector3::Z());
        Vector3 cameraDirection = s_Data.CameraData->Position.Normalized();
        /* The final normal is the camera direction vector projected onto the normal's plane of rotation (the plane
         * perpendicular to the line direction). The rotation angle of the quad around the line direction is then the
         * angle between the initial normal and the final normal, measured counter-clockwise about the line direction.
         * Projection of vector u onto a plane with normal n (assuming all vectors are normalized):
         *  u_plane = u - (u DOT n) * n
         */
        Vector3 finalNormal = cameraDirection - (direction * (cameraDirection.Dot(direction)));
        finalNormal.Normalize();
        float normalRotationAngle = acosf(initialNormal.Dot(finalNormal));
        // Disambiguate quadrant
        if (initialNormal.Cross(finalNormal).Dot(direction) < 0.f) {
            normalRotationAngle = PI2f - normalRotationAngle;
        }
        Quaternion normalRotation{ direction, normalRotationAngle };
        //
        // Apply rotations
        stemTransform = stemTransform * Matrix4(normalRotation);
        stemTransform = stemTransform * Matrix4(rotation);

        stemTransform = glm::scale((glm::mat4)stemTransform, glm::vec3(glm::vec2{ stemLength, thickness }, 0.f));

        float dashLength = dashFactor * thickness;
        float gapLength = gapFactor * thickness;
        DrawBatchedLine(stemTransform, color, length, thickness, dashLength, gapLength, entityId);

        // Arrowhead arms
        Vector3 arm0Direction = Quaternion(finalNormal, 3.f * PIover4f).RotateVector(direction);
        Vector3 arm1Direction = Quaternion(finalNormal,-3.f * PIover4f).RotateVector(direction);

        Vector3 arm0WidthOffset = thickness / 2.f * arm1Direction;
        Vector3 arm0Start = end + arm0WidthOffset;
        Vector3 arm0End = end + arm0WidthOffset + headSize * arm0Direction;

        Vector3 arm1WidthOffset = thickness / 2.f * arm0Direction;
        Vector3 arm1Start = end + arm1WidthOffset;
        Vector3 arm1End = end + arm1WidthOffset + headSize * arm1Direction;

        DrawDashedLine(arm0Start, arm0End, color, thickness, dashFactor, gapFactor, entityId);
        DrawDashedLine(arm1Start, arm1End, color, thickness, dashFactor, gapFactor, entityId);
    }


    void Renderer2D::DrawLine(const Vector2& start, const Vector2& end, float width, const Vector4& color, int layer)
    {
        LV_PROFILE_FUNCTION();

        auto line = end - start;
        auto midpoint = start + (0.5f * line);
        Vector2 dimensions = { sqrt(line.SqrMagnitude()) + width, width };
        float rotation = atanf(line.y / line.x);

        DrawRotatedQuad({ midpoint, (float)layer }, dimensions, rotation, color);
    }


    // Orbital /////////////////////////////

    void Renderer2D::DrawEllipse(const Vector2& centre, const float rotation, const float semiMajorAxis, const float semiMinorAxis, const Vector2& escapePointFromCentre, const float thickness, const Vector4& color, int layer)
    {
        LV_CORE_ASSERT(false, "Do not use!");

        LV_PROFILE_FUNCTION();
    
#ifdef EXCLUDED
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
        s_Data.EllipseShader2->SetMat4("u_Transform", triangleTransform);
    
        s_OrbitData->XLimit = xLimit;
        s_OrbitData->YLimit = yLimit;
        s_OrbitData->XEscape = escapePointFromCentre.x;
        s_OrbitData->YEscape = escapePointFromCentre.y;
        s_OrbitData->SemiMajorAxis = semiMajorAxis;
        s_OrbitData->SemiMinorAxis = semiMinorAxis;
        s_OrbitData->DrawRadius = drawRadius;
        s_Data.OrbitUniformBuffer2->UpdateData(s_OrbitData, 0, sizeof(OrbitData));
    
        s_Data.EllipseShader2->SetVec4("u_Color", color);
    
        RenderCommand::DrawIndexed(s_Data.EllipseVertexArray2);
#endif
    }


    void Renderer2D::DrawHyperbola(const Vector2& centre, const float rotation, const float semiMajorAxis, const float semiMinorAxis, const Vector2& escapePointFromCentre,
        const float thickness, const Vector4& color, int layer)
    {
        LV_CORE_ASSERT(false, "Do not use!");

        LV_PROFILE_FUNCTION();

#ifdef EXCLUDED
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
        s_Data.HyperbolaShader2->SetMat4("u_Transform", triangleTransform);
    
        s_OrbitData->XLimit = xLimit;
        s_OrbitData->YLimit = yLimit;
        s_OrbitData->XEscape = escapePointFromCentre.x;
        s_OrbitData->YEscape = escapePointFromCentre.y;
        s_OrbitData->SemiMajorAxis = semiMajorAxis;
        s_OrbitData->SemiMinorAxis = semiMinorAxis;
        s_OrbitData->DrawRadius = drawRadius;
        s_Data.OrbitUniformBuffer2->UpdateData(s_OrbitData, 0, sizeof(OrbitData));
    
        s_Data.HyperbolaShader2->SetVec4("u_Color", color);
    
        RenderCommand::DrawIndexed(s_Data.HyperbolaVertexArray2);
#endif
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
