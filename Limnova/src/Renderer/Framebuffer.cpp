#include "Framebuffer.h"

#include "Platform/OpenGL/OpenGLFramebuffer.h"
#include "Renderer.h"


namespace Limnova
{

    Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None: LV_CORE_ASSERT(false, "RendererAPI::None is not supported!"); return nullptr;
        case RendererAPI::API::OpenGL: return CreateRef<OpenGLFramebuffer>(spec);
        }
        LV_CORE_ASSERT(false, "Renderer::GetAPI() returned unknown RendererAPI!");
        return nullptr;
    }

}
