#include "Renderer.h"


namespace Limnova
{

    std::unique_ptr<UniformBuffer> Renderer::m_CameraUniformBuffer;


    void Renderer::BeginScene(const std::shared_ptr<Camera>& camera)
    {
        m_CameraUniformBuffer->UpdateData((void*)(camera->GetData()), 0, sizeof(Camera::BufferData));
    }


    void Renderer::EndScene()
    {
    }


    void Renderer::Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray)
    {
        shader->Bind();
        vertexArray->Bind();
        RenderCommand::DrawIndexed(vertexArray);
    }

}
