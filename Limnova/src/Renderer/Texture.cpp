#include "Texture.h"

#include "Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"


namespace Limnova
{

    Ref<Texture2D> Texture2D::Create(const std::string& path)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None: LV_CORE_ASSERT(false, "RendererAPI::None is not supported!"); return nullptr;
            case RendererAPI::API::OpenGL: return std::make_shared<OpenGLTexture2D>(path);
        }
        LV_CORE_ASSERT(false, "Renderer::GetAPI() returned unknown RendererAPI!");
        return nullptr;
    }

}
