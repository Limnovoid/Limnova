#pragma once

#include "Math/glm.h"


namespace Limnova
{

    class Shader
    {
    public:
        static Ref<Shader> Create(const std::string& filepath);
        static Ref<Shader> Create(const std::string& name, const std::string& filepath);
        static Ref<Shader> Create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
        virtual ~Shader() {}

        virtual const std::string& GetName() const = 0;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void BindUniformBuffer(const uint32_t buffer, const std::string& uniformBlockName) = 0;
    };


    class ShaderLibrary
    {
    public:
        void Add(const Ref<Shader>& shader);
        void Add(const std::string& name, const Ref<Shader>& shader);
        Ref<Shader> Load(const std::string& filepath);
        Ref<Shader> Load(const std::string& name, const std::string& filepath);

        Ref<Shader> Get(const std::string& name);
    private:
        bool Exists(const std::string& name);
    private:
        std::unordered_map<std::string, Ref<Shader>> m_Shaders;
    };

}
