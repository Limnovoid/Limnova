#pragma once

#include "Math/glm.h"


namespace Limnova
{

    class Shader
    {
    public:
        static Ref<Shader> Create(const std::string& filepath);
        static Shader* Create(const std::string& vertexSrc, const std::string& fragmentSrc);
        virtual ~Shader() {}

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void BindUniformBuffer(const uint32_t buffer, const std::string& uniformBlockName) = 0;
    };

}
