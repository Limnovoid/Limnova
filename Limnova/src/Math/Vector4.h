#pragma once

#include "glm.h"


namespace Limnova
{

    class Vector4
    {
    public:
        float x, y, z, w;
    public:
        Vector4() : x(0), y(0), z(0), w(0) {}
        Vector4(float v) : x(v), y(v), z(v), w(v) {}
        Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
        Vector4(glm::vec4 glmv) : x(glmv.x), y(glmv.y), z(glmv.z), w(glmv.w) {}

        inline float SqrMagnitude() const { return x * x + y * y + z * z + w * w; }

        // Normalized() returns a normalized copy of a vector.
        Vector4 Normalized() const;
        // Normalize() normalizes a vector in-place and returns it by reference.
        Vector4& Normalize();

        float Dot(const Vector4 rhs) const;
    public:
        Vector4 operator+(const Vector4 rhs) const;
        Vector4& operator+=(const Vector4 rhs);
        Vector4 operator-(const Vector4 rhs) const;
        Vector4& operator-=(const Vector4 rhs);
        Vector4 operator*(const float scalar) const;
        Vector4& operator*=(const float scalar);
        Vector4 operator/(const float scalar) const;
        Vector4& operator/=(const float scalar);
    public:
        operator glm::vec4() const { return glm::vec4(x, y, z, w); }
    public:
        inline glm::vec4 glm_vec4() const { return glm::vec4(x, y, z, w); }
    };


    std::ostream& operator<<(std::ostream& ostream, const Vector4& v);
    Vector4 operator*(const float scalar, const Vector4 vector);

}
