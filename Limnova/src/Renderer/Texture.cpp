#include "Texture.h"

#include "Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"


namespace Limnova
{

    Ref<Texture2D> Texture2D::Create(const uint32_t width, const uint32_t height)
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None: LV_CORE_ASSERT(false, "RendererAPI::None is not supported!"); return nullptr;
        case RendererAPI::API::OpenGL: return std::make_shared<OpenGLTexture2D>(width, height);
        }
        LV_CORE_ASSERT(false, "Renderer::GetAPI() returned unknown RendererAPI!");
        return nullptr;
    }


    Ref<Texture2D> Texture2D::Create(const std::string& path, const WrapMode wrap)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None: LV_CORE_ASSERT(false, "RendererAPI::None is not supported!"); return nullptr;
            case RendererAPI::API::OpenGL: return std::make_shared<OpenGLTexture2D>(path, wrap);
        }
        LV_CORE_ASSERT(false, "Renderer::GetAPI() returned unknown RendererAPI!");
        return nullptr;
    }

}
