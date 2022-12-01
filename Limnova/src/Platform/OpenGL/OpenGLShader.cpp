#include "OpenGLShader.h"

#include <glad/glad.h>

#include <fstream>


namespace Limnova
{

    static GLenum ShaderTypeFromString(const std::string& type)
    {
        if (type == "vertex")
        {
            return GL_VERTEX_SHADER;
        }
        if (type == "fragment" || type == "pixel")
        {
            return GL_FRAGMENT_SHADER;
        }
        LV_CORE_ERROR("Unknown shader type '{0}'!", type);
        return 0;
    }


    OpenGLShader::OpenGLShader(const std::string& filepath)
        : m_RendererId(0), m_NumUniformBlocks(0)
    {
        std::string source = ReadFile(filepath);
        auto shaderSources = Preprocess(source);
        Compile(shaderSources);

        // Extract name from filepath
        auto lastSlash = filepath.find_last_of("/\\");
        lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
        auto lastDot = filepath.rfind('.');
        auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
        m_Name = filepath.substr(lastSlash, count);
    }


    OpenGLShader::OpenGLShader(const std::string& name, const std::string& filepath)
    {
        std::string source = ReadFile(filepath);
        auto shaderSources = Preprocess(source);
        Compile(shaderSources);

        m_Name = name;
    }


    OpenGLShader::OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
        : m_Name(name), m_RendererId(0), m_NumUniformBlocks(0)
    {
        std::unordered_map<GLenum, std::string> shaderSources;
        shaderSources[GL_VERTEX_SHADER] = vertexSrc;
        shaderSources[GL_FRAGMENT_SHADER] = fragmentSrc;
        Compile(shaderSources);
    }


    OpenGLShader::~OpenGLShader()
    {
        glDeleteShader(m_RendererId);
    }


    std::string OpenGLShader::ReadFile(const std::string& filepath)
    {
        std::string result;
        std::ifstream in(filepath, std::ios::in | std::ios::binary);
        if (in)
        {
            in.seekg(0, std::ios::end);
            result.resize(in.tellg());
            in.seekg(0, std::ios::beg);
            //in.read(&result[0], result.size());
            in.read(result.data(), result.size());
            in.close();
        }
        else
        {
            LV_CORE_ERROR("Could not open shader file '{0}'!", filepath);
        }
        return result;
    }


    std::unordered_map<GLenum, std::string> OpenGLShader::Preprocess(const std::string& source)
    {
        std::unordered_map<GLenum, std::string> shaderSources;

        const char* typeToken = "#type";
        size_t typeTokenLength = strlen(typeToken);
        size_t pos = source.find(typeToken, 0);
        while (pos != std::string::npos)
        {
            size_t eol = source.find_first_of("\r\n", pos);
            LV_CORE_ASSERT(eol != std::string::npos, "Syntax error in shader file!");
            size_t begin = pos + typeTokenLength + 1;
            std::string type = source.substr(begin, eol - begin);
            LV_CORE_ASSERT(ShaderTypeFromString(type), "Invalid shader type specified in shader file!");

            size_t nextLinePos = source.find_first_not_of("\r\n", eol);
            pos = source.find(typeToken, nextLinePos);
            shaderSources[ShaderTypeFromString(type)] = source.substr(
                nextLinePos,
                pos - (nextLinePos == std::string::npos ? source.size() - 1 : nextLinePos)
            );
        }
        return shaderSources;
    }


    void OpenGLShader::Compile(const std::unordered_map<GLenum, std::string>& sources)
    {
        GLuint program = glCreateProgram();
        LV_CORE_ASSERT(sources.size() <= 4, "Number of provided shader sources exceeds the supported maximum (4)!");
        std::array<GLenum, 4> glShaderIds;
        size_t shaderIndex = 0;
        for (auto& kv : sources)
        {
            GLenum type = kv.first;
            const std::string& source = kv.second;

            GLuint shader = glCreateShader(type);

            const GLchar* sourceCStr = (const GLchar*)source.c_str();
            glShaderSource(shader, 1, &sourceCStr, 0);

            glCompileShader(shader);

            GLint isCompiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
            if (isCompiled == GL_FALSE)
            {
                GLint maxLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

                // The maxLength includes the NULL character
                std::vector<GLchar> infoLog(maxLength);
                glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

                // We don't need the shader anymore.
                glDeleteShader(shader);

                LV_CORE_ERROR("Shader info log: {0}", infoLog.data());
                LV_CORE_ASSERT(false, "Shader compilation failed!");
                break;
            }

            glAttachShader(program, shader);
            glShaderIds[shaderIndex++] = shader;
        }

        glLinkProgram(program);

        GLint isLinked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
        if (isLinked == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

            // The maxLength includes the NULL character
            std::vector<GLchar> infoLog(maxLength);
            glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

            // We don't need the program anymore.
            glDeleteProgram(program);
            for (size_t id = 0; id < shaderIndex; id++)
            {
                glDeleteShader((GLenum)id);
            }

            LV_CORE_ERROR("OpenGL program info log: {0}", infoLog.data());
            LV_CORE_ASSERT(false, "Shader link failed!");
            return;
        }

        for (size_t id = 0; id < shaderIndex; id++)
        {
            glDetachShader(program, (GLenum)id);
        }

        m_RendererId = program;
    }


    void OpenGLShader::Bind() const
    {
        glUseProgram(m_RendererId);
    }


    void OpenGLShader::Unbind() const
    {
        glUseProgram(0);
    }


    void OpenGLShader::SetInt(const std::string& name, const int value)
    {
        UploadUniformInt(name, value);
    }


    void OpenGLShader::SetFloat(const std::string& name, const float value)
    {
        UploadUniformFloat(name, value);
    }


    void OpenGLShader::SetVec2(const std::string& name, const Vector2& value)
    {
        UploadUniformFloat2(name, value);
    }


    void OpenGLShader::SetVec3(const std::string& name, const Vector3& value)
    {
        UploadUniformFloat3(name, value);
    }


    void OpenGLShader::SetVec4(const std::string& name, const Vector4& value)
    {
        UploadUniformFloat4(name, value);
    }


    void OpenGLShader::SetMat3(const std::string& name, const glm::mat3& value)
    {
        UploadUniformMat4f(name, value);
    }


    void OpenGLShader::SetMat4(const std::string& name, const glm::mat4& value)
    {
        UploadUniformMat4f(name, value);
    }


    void OpenGLShader::BindUniformBuffer(const uint32_t buffer, const std::string& uniformBlockName)
    {
        uint32_t blockIndex = glGetUniformBlockIndex(m_RendererId, uniformBlockName.c_str());
        if (blockIndex == GL_INVALID_INDEX)
        {
            LV_CORE_WARN("Could not bind uniform buffer: could not find uniform block with name '{0}' in program {1}!",
                uniformBlockName, m_RendererId);
            return;
        }
        if (GL_MAX_UNIFORM_BUFFER_BINDINGS <= blockIndex)
        {
            LV_CORE_WARN("Could not bind uniform buffer: index for uniform block '{0}' in program {1} exceeded GL_MAX_UNIFORM_BUFFER_BINDINGS!",
                uniformBlockName, m_RendererId);
            return;
        }
        glUniformBlockBinding(m_RendererId, blockIndex, m_NumUniformBlocks);
        glBindBufferBase(GL_UNIFORM_BUFFER, m_NumUniformBlocks, buffer);
        m_NumUniformBlocks++;
    }


    static uint32_t CheckUniformIndex(const std::string& uniformName, const uint32_t index)
    {
        if (GL_INVALID_INDEX == index)
        {
            LV_CORE_ERROR("Invalid index ({0}) returned for uniform '{1}'!", index, uniformName);
            return 1;
        }
        return 0;
    }


    void OpenGLShader::UploadUniformInt(const std::string& uniformName, const int value)
    {
        uint32_t uniformIndex = glGetUniformLocation(m_RendererId, uniformName.c_str());
        if (CheckUniformIndex(uniformName, uniformIndex)) return;
        glUniform1i(uniformIndex, value);
    }


    void OpenGLShader::UploadUniformFloat(const std::string& uniformName, const float value)
    {
        uint32_t uniformIndex = glGetUniformLocation(m_RendererId, uniformName.c_str());
        if (CheckUniformIndex(uniformName, uniformIndex)) return;
        glUniform1f(uniformIndex, value);
    }


    void OpenGLShader::UploadUniformFloat2(const std::string& uniformName, const glm::vec2& values)
    {
        uint32_t uniformIndex = glGetUniformLocation(m_RendererId, uniformName.c_str());
        if (CheckUniformIndex(uniformName, uniformIndex)) return;
        glUniform2f(uniformIndex, values.x, values.y);
    }


    void OpenGLShader::UploadUniformFloat3(const std::string& uniformName, const glm::vec3& values)
    {
        uint32_t uniformIndex = glGetUniformLocation(m_RendererId, uniformName.c_str());
        if (CheckUniformIndex(uniformName, uniformIndex)) return;
        glUniform3f(uniformIndex, values.x, values.y, values.z);
    }


    void OpenGLShader::UploadUniformFloat4(const std::string& uniformName, const glm::vec4& values)
    {
        uint32_t uniformIndex = glGetUniformLocation(m_RendererId, uniformName.c_str());
        if (CheckUniformIndex(uniformName, uniformIndex)) return;
        glUniform4f(uniformIndex, values.x, values.y, values.z, values.w);
    }


    void OpenGLShader::UploadUniformMat3f(const std::string& uniformName, const glm::mat3& matrix)
    {
        uint32_t uniformIndex = glGetUniformLocation(m_RendererId, uniformName.c_str());
        if (CheckUniformIndex(uniformName, uniformIndex)) return;
        glUniformMatrix3fv(uniformIndex, 1, GL_FALSE, glm::value_ptr(matrix));
    }


    void OpenGLShader::UploadUniformMat4f(const std::string& uniformName, const glm::mat4& matrix)
    {
        uint32_t uniformIndex = glGetUniformLocation(m_RendererId, uniformName.c_str());
        if (CheckUniformIndex(uniformName, uniformIndex)) return;
        glUniformMatrix4fv(uniformIndex, 1, GL_FALSE, glm::value_ptr(matrix));
    }

}
