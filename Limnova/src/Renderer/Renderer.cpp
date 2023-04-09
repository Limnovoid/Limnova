#include "Renderer.h"

#include "Renderer2D.h"

#include "Platform/OpenGL/OpenGLShader.h" // TEMPORARY shader casting


namespace Limnova
{

    Ref<UniformBuffer> Renderer::s_SceneUniformBuffer = nullptr;


    void Renderer::Init()
    {
        LV_PROFILE_FUNCTION();

        RenderCommand::Init();

        s_SceneUniformBuffer = UniformBuffer::Create(0, sizeof(Camera::Data));
    }


    void Renderer::OnWindowResize(uint32_t width, uint32_t height)
    {
        RenderCommand::SetViewport(0, 0, width, height);
    }


    void Renderer::BeginScene(Camera& camera)
    {
        s_SceneUniformBuffer->UpdateData((void*)camera.GetData(), offsetof(SceneData, SceneData::CameraData), sizeof(Camera::Data));
    }


    void Renderer::EndScene()
    {
    }


    void Renderer::Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform)
    {
        shader->Bind();
        std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4f("u_Transform", transform);

        vertexArray->Bind();
        RenderCommand::DrawIndexed(vertexArray);
    }

}
