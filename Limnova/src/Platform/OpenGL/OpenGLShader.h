#pragma once

#include "Renderer/Shader.h"

typedef unsigned int GLenum; // TEMPORARY


namespace Limnova
{

    class OpenGLShader : public Shader
    {
    public:
        OpenGLShader(const std::string& filepath);
        OpenGLShader(const std::string& name, const std::string& filepath);
        OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
        ~OpenGLShader();

        const std::string& GetName() const override { return m_Name; }

        void Bind() const override;
        void Unbind() const override;

        void SetInt(const std::string& name, const int value) override;
        void SetIntArray(const std::string& name, const int* values, const uint32_t count) override;
        void SetFloat(const std::string& name, const float value) override;
        void SetVec2(const std::string& name, const Vector2& value) override;
        void SetVec3(const std::string& name, const Vector3& value) override;
        void SetVec4(const std::string& name, const Vector4& value) override;
        void SetMat3(const std::string& name, const glm::mat3& value) override;
        void SetMat4(const std::string& name, const glm::mat4& value) override;

        void BindUniformBuffer(const uint32_t buffer, const std::string& uniformBlockName) override;

        // Update the values of uniforms in this shader.
        // This shader is bound separately with Bind() - there must be a call to Bind() from this shader anywhere before calling UploadUniform* and after any calls to Bind() from a different shader.
        void UploadUniformInt(const std::string& uniformName, const int value);
        void UploadUniformIntArray(const std::string& uniformName, const int* values, const uint32_t count);

        void UploadUniformFloat(const std::string& uniformName, const float value);
        void UploadUniformFloat2(const std::string& uniformName, const glm::vec2& values);
        void UploadUniformFloat3(const std::string& uniformName, const glm::vec3& values);
        void UploadUniformFloat4(const std::string& uniformName, const glm::vec4& values);

        void UploadUniformMat3f(const std::string& uniformName, const glm::mat3& matrix);
        void UploadUniformMat4f(const std::string& uniformName, const glm::mat4& matrix);
    private:
        std::string ReadFile(const std::string& filepath);
        std::unordered_map<GLenum, std::string> Preprocess(const std::string& source);
        void Compile(const std::unordered_map<GLenum, std::string>& sources);
    private:
        std::string m_Name;
        uint32_t m_RendererId;
        uint32_t m_NumUniformBlocks;
    };

}
