#pragma once

#include "Math/Math.h"
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

        virtual void SetInt(const std::string& name, const int value) = 0;
        virtual void SetIntArray(const std::string& name, const int* values, const uint32_t count) = 0;
        virtual void SetFloat(const std::string& name, const float value) = 0;
        virtual void SetVec2(const std::string& name, const Vector2& value) = 0;
        virtual void SetVec3(const std::string& name, const Vector3& value) = 0;
        virtual void SetVec4(const std::string& name, const Vector4& value) = 0;
        virtual void SetMat3(const std::string& name, const glm::mat3& value) = 0;
        virtual void SetMat4(const std::string& name, const glm::mat4& value) = 0;
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
