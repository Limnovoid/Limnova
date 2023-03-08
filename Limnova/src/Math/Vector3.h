#pragma once

#include "Vector2.h"
#include "glm.h"


namespace Limnova
{

    class Vector3
    {
    public:
        float x, y, z;
    public:
        Vector3() : x(0), y(0), z(0) {}
        Vector3(float v) : x(v), y(v), z(v) {}
        Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
        Vector3(Vector2& vec2, float z) : x(vec2.x), y(vec2.y), z(z) {}
        Vector3(glm::vec3& glmv) : x(glmv.x), y(glmv.y), z(glmv.z) {}

        Vector2 XY() const { return { x, y }; }
        float* Ptr() { return &x; }

        inline float SqrMagnitude() const { return x * x + y * y + z * z; }

        // Normalized() returns a normalized copy of a vector.
        Vector3 Normalized() const;
        // Normalize() normalizes a vector in-place and returns it by reference.
        Vector3& Normalize();

        float Dot(const Vector3 rhs) const;
        Vector3 Cross(const Vector3 rhs) const;
    public:
        static Vector3 Cross(const Vector3 lhs, const Vector3 rhs);
    public:
        Vector3& operator=(const glm::vec3& rhs);
        Vector3 operator+(const Vector3& rhs) const;
        Vector3& operator+=(const Vector3& rhs);
        Vector3 operator-(const Vector3& rhs) const;
        Vector3& operator-=(const Vector3& rhs);
        Vector3 operator*(const float scalar) const;
        Vector3& operator*=(const float scalar);
        Vector3 operator/(const float scalar) const;
        Vector3& operator/=(const float scalar);
    public:
        operator glm::vec3() const { return glm::vec3(x, y, z); }
    public:
        inline glm::vec3 glm_vec3() const { return glm::vec3(x, y, z); }
    };

    std::ostream& operator<<(std::ostream& ostream, const Vector3& v);
    Vector3 operator*(const float scalar, const Vector3 vector);

}
