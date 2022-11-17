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

        void AddUniformBuffer(const uint32_t buffer, const std::string& uniformBlockName) override;
    private:
        uint32_t m_RendererId;
        uint32_t m_NumUniformBlocks;
    };

}
