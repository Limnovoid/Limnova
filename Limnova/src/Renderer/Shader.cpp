#include "Shader.h"

#include "Renderer.h"
#include "Platform/OpenGL/OpenGLShader.h"


namespace Limnova
{

    Shader* Shader::Create(const std::string& vertexSrc, const std::string& fragmentSrc)
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
                return new OpenGLShader(vertexSrc, fragmentSrc);
            }
        }
        LV_CORE_ASSERT(false, "Renderer::GetAPI() returned unknown RendererAPI!");
        return nullptr;
    }

}
