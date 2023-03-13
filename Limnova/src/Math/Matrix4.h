#pragma once

#include "glm.h"
#include "Quaternion.h"


namespace Limnova
{

    class Matrix4
    {
    public:
        glm::mat4 mat;
    public:
        constexpr Matrix4() = default;
        constexpr Matrix4(const Matrix4&) = default;
        Matrix4(const Quaternion& q) : mat(glm::toMat4(glm::quat(q.w, q.v.x, q.v.y, q.v.z))) {}
    public:
        operator glm::mat4() const { return mat; }
    public:
        inline glm::mat4 glm_mat4() const { return mat; }
    };

}
