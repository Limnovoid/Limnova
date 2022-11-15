#include "VertexArray.h"

#include "Renderer.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"


namespace Limnova
{

    VertexArray* VertexArray::Create()
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:     LV_CORE_ASSERT(false, "RendererAPI::None is not supported!"); return nullptr;
            case RendererAPI::API::OpenGL:   return new OpenGLVertexArray();
        }
        LV_CORE_ASSERT(false, "Renderer::GetAPI() returned unknown RendererAPI!");
        return nullptr;
    }

}
