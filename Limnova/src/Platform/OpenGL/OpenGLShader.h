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

        void BindUniformBuffer(const uint32_t buffer, const std::string& uniformBlockName) override;

        // Update the values of uniforms in this shader.
        // This shader is bound separately with Bind() - there must be a call to Bind() from this shader anywhere before calling UploadUniform* and after any calls to Bind() from a different shader.
        void UploadUniformInt(const std::string& uniformName, const int value);

        void UploadUniformFloat(const std::string& uniformName, const float value);
        void UploadUniformFloat2(const std::string& uniformName, const glm::vec2& values);
        void UploadUniformFloat3(const std::string& uniformName, const glm::vec3& values);
        void UploadUniformFloat4(const std::string& uniformName, const glm::vec4& values);

        void UploadUniformMat3f(const std::string& uniformName, const glm::mat3& matrix);
        void UploadUniformMat4f(const std::string& uniformName, const glm::mat4& matrix);
    private:
        uint32_t CheckUniformIndex(const std::string& uniformName, const uint32_t index);
    private:
        uint32_t m_RendererId;
        uint32_t m_NumUniformBlocks;
    };

}
