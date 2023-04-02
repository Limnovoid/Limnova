#pragma once

#include "glm.h"
#include "Quaternion.h"
#include "Vector4.h"


namespace Limnova
{

    class Matrix4
    {
    public:
        glm::mat4 mat;
    public:
        constexpr Matrix4() = default;
        constexpr Matrix4(const Matrix4&) = default;
        constexpr Matrix4(const glm::mat4& M) : mat(M) {}
        Matrix4(const Quaternion& q) : mat(glm::toMat4(glm::quat(q.w, q.v.x, q.v.y, q.v.z))) {}

        static constexpr Matrix4 Identity() { return { glm::identity<glm::mat4>() }; }

        float* Ptr() { return glm::value_ptr(mat); }
        Matrix4 Inverse() const { return { glm::inverse(mat) }; }
    public:
        Vector4 operator*(const Vector4& rhs) const;
        Matrix4 operator*(const Matrix4& rhs) const;
    public:
        operator glm::mat4() const { return mat; }
    public:
        inline glm::mat4 glm_mat4() const { return mat; }
    };

}
