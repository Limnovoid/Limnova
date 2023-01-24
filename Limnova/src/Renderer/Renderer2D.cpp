#include "Renderer2D.h"

#include "VertexArray.h"
#include "Shader.h"

#define ASSET_DIR "C:\\Programming\\source\\Limnova\\DevTool\\assets"


namespace Limnova
{

    Ref<UniformBuffer> Renderer2D::s_SceneUniformBuffer = nullptr;


    struct Renderer2DData
    {
        Ref<VertexArray> SquareVertexArray;
        Ref<Shader> TextureShader;
        Ref<Texture2D> WhiteTexture;
        Ref<VertexArray> HyperbolaVertexArray;
        Ref<Shader> HyperbolaShader;
        Ref<UniformBuffer> HyperbolaUniformBuffer;
    };

    static Renderer2DData* s_Data;


    struct HyperbolaData
    {
        float XLimit;
        float YLimit;
        float XEscape;
        float YEscape;
        float SemiMajorAxis;
        float SemiMinorAxis;
        float DrawRadius;
        float XEscapeTangent;
    };

    static HyperbolaData* s_HyperbolaData;


    void Renderer2D::Init(const Ref<UniformBuffer>& sceneUniformBuffer)
    {
        LV_PROFILE_FUNCTION();

        s_SceneUniformBuffer = sceneUniformBuffer;

        s_Data = new Renderer2DData();

        // Color and texture quad
        s_Data->SquareVertexArray = VertexArray::Create();

        float squareVertices[(3 + 2) * 4] = {
           -.5f, -.5f,  0.f,       0.f, 0.f,
            .5f, -.5f,  0.f,       1.f, 0.f,
            .5f,  .5f,  0.f,       1.f, 1.f,
           -.5f,  .5f,  0.f,       0.f, 1.f
        };
        Ref<VertexBuffer> squareVB = VertexBuffer::Create(squareVertices, sizeof(squareVertices));
        squareVB->SetLayout({
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float2, "a_TexCoord" }
        });
        s_Data->SquareVertexArray->AddVertexBuffer(squareVB);

        uint32_t squareIndices[6] = { 0, 1, 2, 0, 2, 3 };
        Ref<IndexBuffer> squareIB = IndexBuffer::Create(squareIndices, std::size(squareIndices));
        s_Data->SquareVertexArray->SetIndexBuffer(squareIB);

        s_Data->WhiteTexture = Texture2D::Create(1, 1);
        uint32_t whiteTextureData = 0xffffffff;
        s_Data->WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

        s_Data->TextureShader = Shader::Create(ASSET_DIR"\\shaders\\Texture.lvglsl");
        s_Data->TextureShader->BindUniformBuffer(Renderer::GetSceneUniformBufferId(), "CameraUniform");
        s_Data->TextureShader->Bind();
        s_Data->TextureShader->SetInt("u_Texture", 0);

        // Color hyperbola
        s_Data->HyperbolaVertexArray = VertexArray::Create();

        float hyperbolaVertices[3 * 3] = {
            0.f,  0.f,  0.f,
           -1.f,  1.f,  0.f,
           -1.f, -1.f,  0.f
        };
        Ref<VertexBuffer> hyperbolaVB = VertexBuffer::Create(hyperbolaVertices, sizeof(hyperbolaVertices));
        hyperbolaVB->SetLayout({
            { ShaderDataType::Float3, "a_Position" }
            });
        s_Data->HyperbolaVertexArray->AddVertexBuffer(hyperbolaVB);

        uint32_t hyperbolaIndices[3] = { 0, 1, 2 };
        Ref<IndexBuffer> hyperbolaIB = IndexBuffer::Create(hyperbolaIndices, std::size(hyperbolaIndices));
        s_Data->HyperbolaVertexArray->SetIndexBuffer(hyperbolaIB);

        s_Data->HyperbolaShader = Shader::Create(ASSET_DIR"\\shaders\\Hyperbola.lvglsl");
        s_Data->HyperbolaShader->BindUniformBuffer(Renderer::GetSceneUniformBufferId(), "CameraUniform");

        s_HyperbolaData = new HyperbolaData();
        s_Data->HyperbolaUniformBuffer = UniformBuffer::Create((void*)&s_HyperbolaData, sizeof(HyperbolaData));
        s_Data->HyperbolaShader->BindUniformBuffer(s_Data->HyperbolaUniformBuffer->GetRendererId(), "HyperbolaUniform");
    }


    void Renderer2D::Shutdown()
    {
        LV_PROFILE_FUNCTION();

        delete s_Data;
    }


    void Renderer2D::BeginScene(Camera& camera)
    {
        LV_PROFILE_FUNCTION();

        s_SceneUniformBuffer->UpdateData((void*)camera.GetData(), offsetof(Renderer::SceneData, Renderer::SceneData::CameraData), sizeof(Camera::Data));

        s_Data->TextureShader->Bind();
    }


    void Renderer2D::EndScene()
    {
        LV_PROFILE_FUNCTION();
    }


    void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, const Vector4& color)
    {
        LV_PROFILE_FUNCTION();

        glm::mat4 squareTransform = glm::translate(glm::mat4(1.f), (glm::vec3)position);
        squareTransform = glm::scale(squareTransform, glm::vec3((glm::vec2)size, 1.f));
        s_Data->TextureShader->SetMat4("u_Transform", squareTransform);

        s_Data->TextureShader->SetVec4("u_Color", color);
        s_Data->TextureShader->SetVec2("u_TexScale", 1.f);
        s_Data->WhiteTexture->Bind(0);

        s_Data->SquareVertexArray->Bind();
        RenderCommand::DrawIndexed(s_Data->SquareVertexArray);
    }


    void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Vector4& color)
    {
        DrawQuad({ position.x, position.y, 0.f }, size, color);
    }


    void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, const Ref<Texture2D>& texture, const Vector4& tint, const Vector2& textureScale)
    {
        LV_PROFILE_FUNCTION();

        glm::mat4 squareTransform = glm::translate(glm::mat4(1.f), (glm::vec3)position);
        squareTransform = glm::scale(squareTransform, glm::vec3((glm::vec2)size, 1.f));
        s_Data->TextureShader->SetMat4("u_Transform", squareTransform);

        s_Data->TextureShader->SetVec4("u_Color", tint);
        s_Data->TextureShader->SetVec2("u_TexScale", textureScale);
        texture->Bind(0);

        s_Data->SquareVertexArray->Bind();
        RenderCommand::DrawIndexed(s_Data->SquareVertexArray);
    }


    void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Ref<Texture2D>& texture, const Vector4& tint, const Vector2& textureScale)
    {
        DrawQuad({ position.x, position.y, 0.f }, size, texture, tint, textureScale);
    }


    void Renderer2D::DrawRotatedQuad(const Vector3& position, const Vector2& size, const float rotation, const Vector4& color)
    {
        LV_PROFILE_FUNCTION();

        glm::mat4 squareTransform = glm::translate(glm::mat4(1.f), (glm::vec3)position);
        squareTransform = glm::rotate(squareTransform, rotation, {0.f, 0.f, 1.f});
        squareTransform = glm::scale(squareTransform, glm::vec3((glm::vec2)size, 1.f));
        s_Data->TextureShader->SetMat4("u_Transform", squareTransform);

        s_Data->TextureShader->SetVec4("u_Color", color);
        s_Data->TextureShader->SetVec2("u_TexScale", 1.f);
        s_Data->WhiteTexture->Bind(0);

        s_Data->SquareVertexArray->Bind();
        RenderCommand::DrawIndexed(s_Data->SquareVertexArray);
    }


    void Renderer2D::DrawRotatedQuad(const Vector2& position, const Vector2& size, const float rotation, const Vector4& color)
    {
        DrawRotatedQuad({ position.x, position.y, 0.f }, size, rotation, color);
    }


    void Renderer2D::DrawRotatedQuad(const Vector3& position, const Vector2& size, const float rotation, const Ref<Texture2D>& texture, const Vector4& tint, const Vector2& textureScale)
    {
        LV_PROFILE_FUNCTION();

        glm::mat4 squareTransform = glm::translate(glm::mat4(1.f), (glm::vec3)position);
        squareTransform = glm::rotate(squareTransform, rotation, { 0.f, 0.f, 1.f });
        squareTransform = glm::scale(squareTransform, glm::vec3((glm::vec2)size, 1.f));
        s_Data->TextureShader->SetMat4("u_Transform", squareTransform);

        s_Data->TextureShader->SetVec4("u_Color", tint);
        s_Data->TextureShader->SetVec2("u_TexScale", textureScale);
        texture->Bind(0);

        s_Data->SquareVertexArray->Bind();
        RenderCommand::DrawIndexed(s_Data->SquareVertexArray);
    }


    void Renderer2D::DrawRotatedQuad(const Vector2& position, const Vector2& size, const float rotation, const Ref<Texture2D>& texture, const Vector4& tint, const Vector2& textureScale)
    {
        DrawRotatedQuad({ position.x, position.y, 0.f }, size, rotation, texture, tint, textureScale);
    }


    void Renderer2D::DrawLine(const Vector2& start, const Vector2& end, const float thickness, const Vector4& color, int layer)
    {
        auto line = end - start;
        auto midpoint = start + (0.5f * line);
        Vector2 dimensions = { sqrt(line.SqrMagnitude()) + thickness, thickness };
        float rotation = atanf(line.y / line.x);

        DrawRotatedQuad({ midpoint.x, midpoint.y, (float)layer }, dimensions, rotation, color);
    }


    void Renderer2D::DrawHyperbola(const Vector2& centre, const float rotation, const float semiMajorAxis, const float semiMinorAxis, const Vector2& escapePoint,
        const float thickness, const Vector4& color, int layer)
    {
        LV_PROFILE_FUNCTION();

        s_Data->HyperbolaShader->Bind();

        // TODO - use thickness to pad xLimit and yLimit to draw full width of line at escape point
        // Coordinates of the top-left corner of the triangle, measured in the hyperbola's coordinate system.
        s_HyperbolaData->XEscapeTangent = escapePoint.y * semiMajorAxis * semiMajorAxis / (semiMinorAxis * semiMinorAxis * escapePoint.x);
        float xEscapeTanNormalised = s_HyperbolaData->XEscapeTangent / sqrt(s_HyperbolaData->XEscapeTangent * s_HyperbolaData->XEscapeTangent + 1.f);
        float drawRadius = thickness / 2.f;
        float xLimit = escapePoint.x + drawRadius;
        float yLimit = (escapePoint.y + drawRadius / xEscapeTanNormalised) * xLimit / escapePoint.x;

        glm::mat4 triangleTransform = glm::translate(glm::mat4(1.f), { (glm::vec2)centre, (float)layer });
        triangleTransform = glm::rotate(triangleTransform, rotation, { 0.f, 0.f, 1.f });
        triangleTransform = glm::scale(triangleTransform, glm::vec3(xLimit, yLimit, 1.f));
        s_Data->HyperbolaShader->SetMat4("u_Transform", triangleTransform);

        s_HyperbolaData->XLimit = xLimit;
        s_HyperbolaData->YLimit = yLimit;
        s_HyperbolaData->XEscape = escapePoint.x;
        s_HyperbolaData->YEscape = escapePoint.y;
        s_HyperbolaData->SemiMajorAxis = semiMajorAxis;
        s_HyperbolaData->SemiMinorAxis = semiMinorAxis;
        s_HyperbolaData->DrawRadius = drawRadius;
        s_Data->HyperbolaUniformBuffer->UpdateData(s_HyperbolaData, 0, sizeof(HyperbolaData));

        s_Data->HyperbolaShader->SetVec4("u_Color", color);

        s_Data->HyperbolaVertexArray->Bind();
        RenderCommand::DrawIndexed(s_Data->HyperbolaVertexArray);

        s_Data->TextureShader->Bind();
    }

}
