#include "Renderer.h"

#include "Platform/OpenGL/OpenGLShader.h" // TEMPORARY shader casting


namespace Limnova
{

    Ref<UniformBuffer> Renderer::m_CameraUniformBuffer = nullptr;


    void Renderer::Init()
    {
        RenderCommand::Init();

        Camera::BufferData data = { glm::mat4(1.f), glm::vec4(0.f), glm::vec4(0.f,0.f,-1.f,0.f) };
        m_CameraUniformBuffer = UniformBuffer::Create((void*)&data, sizeof(Camera::BufferData));
    }


    void Renderer::OnWindowResize(uint32_t width, uint32_t height)
    {
        RenderCommand::SetViewport(0, 0, width, height);
    }


    void Renderer::BeginScene(Camera& camera)
    {
        m_CameraUniformBuffer->UpdateData((void*)camera.GetData(), 0, sizeof(Camera::BufferData));
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
