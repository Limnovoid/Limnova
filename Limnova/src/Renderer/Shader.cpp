#include "Shader.h"

#include "Renderer.h"
#include "Platform/OpenGL/OpenGLShader.h"


namespace Limnova
{

    Ref<Shader> Shader::Create(const std::string& filepath)
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None: LV_CORE_ASSERT(false, "RendererAPI::None is not supported!"); return nullptr;
        case RendererAPI::API::OpenGL: return std::make_shared<OpenGLShader>(filepath);
        }
        LV_CORE_ASSERT(false, "Renderer::GetAPI() returned unknown RendererAPI!");
        return nullptr;
    }


    Shader* Shader::Create(const std::string& vertexSrc, const std::string& fragmentSrc)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None: LV_CORE_ASSERT(false, "RendererAPI::None is not supported!"); return nullptr;
            case RendererAPI::API::OpenGL: return new OpenGLShader(vertexSrc, fragmentSrc);
        }
        LV_CORE_ASSERT(false, "Renderer::GetAPI() returned unknown RendererAPI!");
        return nullptr;
    }

}
