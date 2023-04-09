#include "OpenGLShader.h"

#include <Core/Timer.h>

#include <glad/glad.h>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>


namespace Limnova
{

    static constexpr const char* kCacheDir = LV_ASSET_DIR"/cache/shader/opengl";
    static constexpr bool kOptimizeShaderc = true;


    static GLenum ShaderTypeFromString(const std::string& type)
    {
        if (type == "vertex") {
            return GL_VERTEX_SHADER;
        }
        if (type == "fragment" || type == "pixel") {
            return GL_FRAGMENT_SHADER;
        }
        LV_CORE_ERROR("Unknown shader type '{0}'!", type);
        return 0;
    }


    static shaderc_shader_kind GLShaderStageToShaderc(GLenum stage)
    {
        switch (stage)
        {
        case GL_VERTEX_SHADER: return shaderc_glsl_vertex_shader;
        case GL_FRAGMENT_SHADER: return shaderc_glsl_fragment_shader;
        }
        LV_CORE_ASSERT(false, "Unknown shader stage!");
        return (shaderc_shader_kind)0;
    }


    static const char* GLShaderStageToString(GLenum stage)
    {
        switch (stage)
        {
        case GL_VERTEX_SHADER: return "GL_VERTEX_SHADER";
        case GL_FRAGMENT_SHADER: return "GL_FRAGMENT_SHADER";
        }
        LV_CORE_ASSERT(false, "Unknown shader stage!");
        return nullptr;
    }


    static const char* GetCacheDirectory()
    {
        // TODO : check assets directory is valid
        return kCacheDir;
    }


    static void CreateCacheDirectoryIfNeeded()
    {
        std::string cacheDirectory = GetCacheDirectory();
        if (!std::filesystem::exists(cacheDirectory)) {
            std::filesystem::create_directories(cacheDirectory);
        }
    }


    static const char* GLShaderStageCachedOpenGLFileExtension(uint32_t stage)
    {
        switch (stage)
        {
        case GL_VERTEX_SHADER: return ".cached_opengl.vert";
        case GL_FRAGMENT_SHADER: return ".cached_opengl.frag";
        }
        LV_CORE_ASSERT(false, "Unknown shader stage!");
        return "";
    }


    static const char* GLShaderStageCachedVulkanFileExtension(uint32_t stage)
    {
        switch (stage)
        {
        case GL_VERTEX_SHADER: return ".cached_vulkan.vert";
        case GL_FRAGMENT_SHADER: return ".cached_vulkan.frag";
        }
        LV_CORE_ASSERT(false, "Unknown shader stage!");
        return "";
    }


    OpenGLShader::OpenGLShader(const std::string& filepath)
        : m_Filepath(filepath)
    {
        LV_PROFILE_FUNCTION();

        CreateCacheDirectoryIfNeeded();

        std::string source = ReadFile(filepath);
        auto shaderSources = Preprocess(source);
        
        {
            Timer timer;
            CompileOrGetVulkanBinaries(shaderSources);
            CompileOrGetOpenGLBinaries();
            CreateProgram();
            LV_CORE_INFO("Shader creation took {0} ms", timer.ElapsedMillis());
        }

        // Extract name from filepath
        auto lastSlash = filepath.find_last_of("/\\");
        lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
        auto lastDot = filepath.rfind('.');
        auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
        m_Name = filepath.substr(lastSlash, count);
    }


    OpenGLShader::OpenGLShader(const std::string& name, const std::string& filepath)
        : m_Name(name), m_Filepath(filepath)
    {
        LV_PROFILE_FUNCTION();

        std::string source = ReadFile(filepath);
        auto shaderSources = Preprocess(source);

        CompileOrGetVulkanBinaries(shaderSources);
        CompileOrGetOpenGLBinaries();
        CreateProgram();
    }


    OpenGLShader::OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
        : m_Name(name)
    {
        LV_PROFILE_FUNCTION();

        std::unordered_map<GLenum, std::string> shaderSources;
        shaderSources[GL_VERTEX_SHADER] = vertexSrc;
        shaderSources[GL_FRAGMENT_SHADER] = fragmentSrc;

        CompileOrGetVulkanBinaries(shaderSources);
        CompileOrGetOpenGLBinaries();
        CreateProgram();
    }


    OpenGLShader::~OpenGLShader()
    {
        LV_PROFILE_FUNCTION();

        glDeleteShader(m_RendererId);
    }


    std::string OpenGLShader::ReadFile(const std::string& filepath)
    {
        LV_PROFILE_FUNCTION();

        std::string result;
        std::ifstream in(filepath, std::ios::in | std::ios::binary);
        if (in)
        {
            in.seekg(0, std::ios::end);
            size_t size = in.tellg();
            if (size != -1)
            {
                result.resize(size);
                in.seekg(0, std::ios::beg);
                in.read(&result[0], size);
            }
            else
            {
                LV_CORE_ERROR("Could not read from file '{0}'!", filepath);
            }
        }
        else
        {
            LV_CORE_ERROR("Could not open shader file '{0}'!", filepath);
        }
        return result;
    }


    std::unordered_map<GLenum, std::string> OpenGLShader::Preprocess(const std::string& source)
    {
        LV_PROFILE_FUNCTION();

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


    void OpenGLShader::CompileOrGetVulkanBinaries(const std::unordered_map<GLenum, std::string>& sources)
    {
        LV_PROFILE_FUNCTION();

        GLuint program = glCreateProgram();

        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan,
            shaderc_env_version_vulkan_1_2);
        if (kOptimizeShaderc) {
            options.SetOptimizationLevel(shaderc_optimization_level_performance);
        }

        std::filesystem::path cacheDirectory = GetCacheDirectory();
        auto& shaderData = m_VulkanSPIRV;
        shaderData.clear();
        for (auto&& [stage, source] : sources)
        {
            std::filesystem::path shaderFilepath = m_Filepath;
            std::filesystem::path cachedPath = cacheDirectory /
                (shaderFilepath.filename().string() +
                GLShaderStageCachedVulkanFileExtension(stage));

            std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
            if (in.is_open())
            {
                in.seekg(0, std::ios::end);
                auto size = in.tellg();
                in.seekg(0, std::ios::beg);

                auto& data = shaderData[stage];
                data.resize(size / sizeof(uint32_t));
                in.read((char*)data.data(), size);
            }
            else
            {
                shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
                    source, GLShaderStageToShaderc(stage), m_Filepath.c_str(), options);

                LV_CORE_ASSERT(module.GetCompilationStatus() == shaderc_compilation_status_success, module.GetErrorMessage());

                shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());
                std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
                if (out.is_open())
                {
                    auto& data = shaderData[stage];
                    out.write((char*)data.data(), data.size() * sizeof(uint32_t));
                    out.flush();
                    out.close();
                }
            }
        }

        for (auto&& [stage, data] : shaderData) {
            Reflect(stage, data);
        }
    }


    void OpenGLShader::CompileOrGetOpenGLBinaries()
    {
        auto& shaderData = m_OpenGLSPIRV;

        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_opengl,
            shaderc_env_version_opengl_4_5);
        if (kOptimizeShaderc) {
            options.SetOptimizationLevel(shaderc_optimization_level_performance);
        }

        std::filesystem::path cacheDirectory = GetCacheDirectory();

        shaderData.clear();
        m_OpenGLSource.clear();
        for (auto&& [stage, spirv] : m_VulkanSPIRV)
        {
            std::filesystem::path shaderFilepath = m_Filepath;
            std::filesystem::path cachedPath = cacheDirectory /
                (shaderFilepath.filename().string() +
                GLShaderStageCachedOpenGLFileExtension(stage));

            std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
            if (in.is_open())
            {
                in.seekg(0, std::ios::end);
                auto size = in.tellg();
                in.seekg(0, std::ios::beg);

                auto& data = shaderData[stage];
                data.resize(size / sizeof(uint32_t));
                in.read((char*)data.data(), size);
            }
            else
            {
                spirv_cross::CompilerGLSL glslCompiler(spirv);
                m_OpenGLSource[stage] = glslCompiler.compile();
                auto& source = m_OpenGLSource[stage];

                shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
                    source, GLShaderStageToShaderc(stage), m_Filepath.c_str());

                LV_CORE_ASSERT(module.GetCompilationStatus() == shaderc_compilation_status_success, module.GetErrorMessage());

                shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

                std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
                if (out.is_open())
                {
                    auto& data = shaderData[stage];
                    out.write((char*)data.data(), data.size() * sizeof(uint32_t));
                    out.flush();
                    out.close();
                }
            }
        }
    }


    void OpenGLShader::CreateProgram()
    {
        GLuint program = glCreateProgram();

        std::vector<GLuint> shaderIds;
        for (auto&& [stage, spirv] : m_OpenGLSPIRV)
        {
            GLuint shaderId = shaderIds.emplace_back(glCreateShader(stage));
            glShaderBinary(1, &shaderId, GL_SHADER_BINARY_FORMAT_SPIR_V,
                spirv.data(), spirv.size() * sizeof(uint32_t));
            glSpecializeShader(shaderId, "main", 0, nullptr, nullptr);
            glAttachShader(program, shaderId);
        }

        glLinkProgram(program);

        GLint isLinked;
        glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
        if (isLinked == GL_FALSE)
        {
            GLint maxLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
            std::vector<GLchar> infoLog(maxLength);
            glGetProgramInfoLog(program, maxLength, &maxLength, infoLog.data());
            LV_CORE_ERROR("Shader linking failed ({0}):\n{1}", m_Filepath, infoLog.data());

            glDeleteProgram(program);
            for (auto id : shaderIds) {
                glDeleteShader(id);
            }
        }

        for (auto id : shaderIds)
        {
            glDetachShader(program, id);
            glDeleteShader(id);
        }

        m_RendererId = program;
    }


    void OpenGLShader::Reflect(GLenum stage, const std::vector<uint32_t>& shaderData)
    {
        spirv_cross::Compiler compiler(shaderData);
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        LV_CORE_INFO("OpenGLShader::Reflect: {0} {1}", GLShaderStageToString(stage), m_Filepath);
        LV_CORE_INFO("> {0} uniform buffers", resources.uniform_buffers.size());
        LV_CORE_INFO("> {0} resources", resources.sampled_images.size());

        LV_CORE_INFO("Uniform buffers:");
        for (const auto& resource : resources.uniform_buffers)
        {
            const auto& bufferType = compiler.get_type(resource.base_type_id);
            uint32_t bufferSize = compiler.get_declared_struct_size(bufferType);
            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            int memberCount = bufferType.member_types.size();

            LV_CORE_INFO("> {0}", resource.name);
            LV_CORE_INFO("  > Size = {0}", bufferSize);
            LV_CORE_INFO("  > Binding = {0}", binding);
            LV_CORE_INFO("  > Members = {0}", memberCount);
            for (int idx = 0; idx < memberCount; idx++) {
                const std::string& name = compiler.get_member_name(resource.base_type_id, idx);
                LV_CORE_INFO("    > {0}", name);
            }
        }
    }


    void OpenGLShader::Bind() const
    {
        LV_PROFILE_FUNCTION();

        glUseProgram(m_RendererId);
    }


    void OpenGLShader::Unbind() const
    {
        LV_PROFILE_FUNCTION();

        glUseProgram(0);
    }


    void OpenGLShader::SetInt(const std::string& name, const int value)
    {
        LV_PROFILE_FUNCTION();

        UploadUniformInt(name, value);
    }


    void OpenGLShader::SetIntArray(const std::string& name, const int* values, const uint32_t count)
    {
        LV_PROFILE_FUNCTION();

        UploadUniformIntArray(name, values, count);
    }


    void OpenGLShader::SetFloat(const std::string& name, const float value)
    {
        LV_PROFILE_FUNCTION();

        UploadUniformFloat(name, value);
    }


    void OpenGLShader::SetVec2(const std::string& name, const Vector2& value)
    {
        LV_PROFILE_FUNCTION();

        UploadUniformFloat2(name, value);
    }


    void OpenGLShader::SetVec3(const std::string& name, const Vector3& value)
    {
        LV_PROFILE_FUNCTION();

        UploadUniformFloat3(name, value);
    }


    void OpenGLShader::SetVec4(const std::string& name, const Vector4& value)
    {
        LV_PROFILE_FUNCTION();

        UploadUniformFloat4(name, value);
    }


    void OpenGLShader::SetMat3(const std::string& name, const glm::mat3& value)
    {
        LV_PROFILE_FUNCTION();

        UploadUniformMat4f(name, value);
    }


    void OpenGLShader::SetMat4(const std::string& name, const glm::mat4& value)
    {
        LV_PROFILE_FUNCTION();

        UploadUniformMat4f(name, value);
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


    void OpenGLShader::UploadUniformIntArray(const std::string& uniformName, const int* values, const uint32_t count)
    {
        uint32_t uniformIndex = glGetUniformLocation(m_RendererId, uniformName.c_str());
        if (CheckUniformIndex(uniformName, uniformIndex)) return;
        glUniform1iv(uniformIndex, count, values);
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
