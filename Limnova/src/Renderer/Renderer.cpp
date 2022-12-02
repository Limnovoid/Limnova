#include "Renderer.h"

#include "Renderer2D.h"

#include "Platform/OpenGL/OpenGLShader.h" // TEMPORARY shader casting


namespace Limnova
{

    Ref<UniformBuffer> Renderer::m_SceneUniformBuffer = nullptr;


    void Renderer::Init()
    {
        LV_PROFILE_FUNCTION();

        RenderCommand::Init();

        SceneData scene = { { glm::mat4(1.f), glm::vec4(0.f), glm::vec4(0.f,0.f,-1.f,0.f) } };
        m_SceneUniformBuffer = UniformBuffer::Create((void*)&scene, sizeof(Camera::Data));

        Renderer2D::Init(m_SceneUniformBuffer);
        LV_CORE_ASSERT(m_SceneUniformBuffer.use_count() == 2, "Incorrect number of references to Renderer::m_SceneUniformBuffer!");
    }


    void Renderer::OnWindowResize(uint32_t width, uint32_t height)
    {
        RenderCommand::SetViewport(0, 0, width, height);
    }


    void Renderer::BeginScene(Camera& camera)
    {
        m_SceneUniformBuffer->UpdateData((void*)camera.GetData(), offsetof(SceneData, SceneData::CameraData), sizeof(Camera::Data));
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
