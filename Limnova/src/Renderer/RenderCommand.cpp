#include "RenderCommand.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"


namespace Limnova
{

    RendererAPI* RenderCommand::s_RendererApi = new OpenGLRendererAPI;


    void RenderCommand::Init()
    {
        s_RendererApi->Init();
    }


    void RenderCommand::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        s_RendererApi->SetViewport(x, y, width, height);
    }


    void RenderCommand::SetClearColor(const glm::vec4& color)
    {
        s_RendererApi->SetClearColor(color);
    }


    void RenderCommand::Clear()
    {
        s_RendererApi->Clear();
    }


    void RenderCommand::DrawIndexed(const Ref<VertexArray>& vertexArray)
    {
        s_RendererApi->DrawIndexed(vertexArray);
    }


    void RenderCommand::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
    {
        s_RendererApi->DrawIndexed(vertexArray, indexCount);
    }

}
