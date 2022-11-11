#include "Buffer.h"

#include "Renderer.h"
#include "Platform/OpenGL/OpenGLBuffer.h"


namespace Limnova
{

    VertexBuffer* VertexBuffer::Create(float* vertices, uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::None:
            {
                LV_CORE_ASSERT(false, "RendererAPI::None is not supported!");
                return nullptr;
            }
            case RendererAPI::OpenGL:
            {
                return new OpenGLVertexBuffer(vertices, size);
            }
        }
        LV_CORE_ASSERT(false, "Renderer::GetAPI() returned unknown RendererAPI!");
        return nullptr;
    }


    IndexBuffer* IndexBuffer::Create(uint32_t* indices, uint32_t count)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::None:
            {
                LV_CORE_ASSERT(false, "RendererAPI::None is not supported!");
                return nullptr;
            }
            case RendererAPI::OpenGL:
            {
                return new OpenGLIndexBuffer(indices, count);
            }
        }
        LV_CORE_ASSERT(false, "Renderer::GetAPI() returned unknown RendererAPI!");
        return nullptr;
    }

}
