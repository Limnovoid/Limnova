#pragma once


namespace Limnova
{

    class Shader
    {
    public:
        static Shader* Create(const std::string& vertexSrc, const std::string& fragmentSrc);
        virtual ~Shader() {}

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void AddUniformBuffer(const uint32_t buffer, const std::string& uniformBlockName) = 0;
    };

}
