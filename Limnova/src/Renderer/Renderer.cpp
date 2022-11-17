#include "Renderer.h"


namespace Limnova
{

    std::unique_ptr<UniformBuffer> Renderer::m_CameraUniformBuffer;


    void Renderer::BeginScene(const CameraData* camera)
    {
        m_CameraUniformBuffer->UpdateData((void*)camera, sizeof(CameraData));
    }


    void Renderer::EndScene()
    {
    }


    void Renderer::Submit(const std::shared_ptr<VertexArray>& vertexArray)
    {
        vertexArray->Bind();
        RenderCommand::DrawIndexed(vertexArray);
    }

}
