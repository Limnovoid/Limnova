#include "OpenGLShader.h"

#include <glad/glad.h>


namespace Limnova
{

    OpenGLShader::OpenGLShader(const std::string& vertexSrc, const std::string& fragmentSrc)
        : m_NumUniformBlocks(0)
    {
        // Create an empty vertex shader handle
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

        // Send the vertex shader source code to GL
        // Note that std::string's .c_str is NULL character terminated.
        const GLchar* source = (const GLchar*)vertexSrc.c_str();
        glShaderSource(vertexShader, 1, &source, 0);

        // Compile the vertex shader
        glCompileShader(vertexShader);

        GLint isCompiled = 0;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

            // The maxLength includes the NULL character
            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(vertexShader, maxLength, &maxLength, &infoLog[0]);

            // We don't need the shader anymore.
            glDeleteShader(vertexShader);

            LV_CORE_ERROR("Shader info log: {0}", infoLog.data());
            LV_CORE_ASSERT(false, "Vertex shader compilation failed!");
            return;
        }

        // Create an empty fragment shader handle
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

        // Send the fragment shader source code to GL
        // Note that std::string's .c_str is NULL character terminated.
        source = (const GLchar*)fragmentSrc.c_str();
        glShaderSource(fragmentShader, 1, &source, 0);

        // Compile the fragment shader
        glCompileShader(fragmentShader);

        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

            // The maxLength includes the NULL character
            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, &infoLog[0]);

            // We don't need the shader anymore.
            glDeleteShader(fragmentShader);
            // Either of them. Don't leak shaders.
            glDeleteShader(vertexShader);

            LV_CORE_ERROR("OpenGL shader info log: {0}", infoLog.data());
            LV_CORE_ASSERT(false, "Fragment shader compilation failed!");
            return;
        }

        // Vertex and fragment shaders are successfully compiled.
        // Now time to link them together into a program.
        // Get a program object.
        GLuint program = glCreateProgram();

        // Attach our shaders to our program
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);

        // Link our program
        glLinkProgram(program);

        // Note the different functions here: glGetProgram* instead of glGetShader*.
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
            // Don't leak shaders either.
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);

            LV_CORE_ERROR("OpenGL program info log: {0}", infoLog.data());
            LV_CORE_ASSERT(false, "Shader link failed!");
            return;
        }

        // Always detach shaders after a successful link.
        glDetachShader(program, vertexShader);
        glDetachShader(program, fragmentShader);

        m_RendererId = program;
    }


    OpenGLShader::~OpenGLShader()
    {
        glDeleteShader(m_RendererId);
    }


    void OpenGLShader::Bind() const
    {
        glUseProgram(m_RendererId);
    }


    void OpenGLShader::Unbind() const
    {
        glUseProgram(0);
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


    uint32_t OpenGLShader::CheckUniformIndex(const std::string& uniformName, const uint32_t index)
    {
        if (GL_INVALID_INDEX == index)
        {
            LV_CORE_ERROR("Invalid index ({0}) returned for uniform '{1}'!", index, uniformName);
            return 1;
        }
        return 0;
    }

}
