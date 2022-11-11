#pragma once

#include "Renderer/Shader.h"


namespace Limnova
{

    class OpenGLShader : public Shader
    {
    public:
        OpenGLShader(const std::string& vertexSrc, const std::string& fragmentSrc);
        ~OpenGLShader();

        void Bind() const override;
        void Unbind() const override;
    private:
        uint32_t m_RendererId;
    };

}
