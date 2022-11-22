#include "Renderer.h"

#include "Platform/OpenGL/OpenGLShader.h" // TEMPORARY shader casting


namespace Limnova
{

    Scope<UniformBuffer> Renderer::m_CameraUniformBuffer;


    void Renderer::BeginScene(const Ref<Camera>& camera)
    {
        m_CameraUniformBuffer->UpdateData((void*)(camera->GetData()), 0, sizeof(Camera::BufferData));
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
