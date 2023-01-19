#include "Renderer2D.h"

#include "VertexArray.h"
#include "Shader.h"

#define ASSET_DIR "C:\\Programming\\source\\Limnova\\DevTool\\assets"


namespace Limnova
{

    Ref<UniformBuffer> Renderer2D::m_SceneUniformBuffer = nullptr;


    struct Renderer2DData
    {
        Ref<VertexArray> SquareVertexArray;
        Ref<Shader> TextureShader;
        Ref<Texture2D> WhiteTexture;
    };

    static Renderer2DData* s_Data;


    void Renderer2D::Init(const Ref<UniformBuffer>& sceneUniformBuffer)
    {
        LV_PROFILE_FUNCTION();

        m_SceneUniformBuffer = sceneUniformBuffer;

        s_Data = new Renderer2DData();

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
    }


    void Renderer2D::Shutdown()
    {
        LV_PROFILE_FUNCTION();

        delete s_Data;
    }


    void Renderer2D::BeginScene(Camera& camera)
    {
        LV_PROFILE_FUNCTION();

        m_SceneUniformBuffer->UpdateData((void*)camera.GetData(), offsetof(Renderer::SceneData, Renderer::SceneData::CameraData), sizeof(Camera::Data));

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

}
