#pragma once

#include "glm.h"


namespace Limnova
{

    class Vector2
    {
    public:
        float x, y;
    public:
        Vector2() : x(0), y(0) {}
        Vector2(float v) : x(v), y(v) {}
        Vector2(float x, float y) : x(x), y(y) {}
        Vector2(glm::vec2& glmv) : x(glmv.x), y(glmv.y) {}

        inline float SqrMagnitude() const { return x * x + y * y; }

        // Returns a normalized copy of a vector.
        Vector2 Normalized() const;
        // Normalizes a vector in-place and returns it by reference.
        Vector2& Normalize();

        float Dot(const Vector2 rhs) const;

        bool IsZero() const { return x == 0.f && y == 0.f; }

        // Returns the zero vector.
        static Vector2 Zero() { return Vector2{ 0.f, 0.f }; }
    public:
        Vector2 operator+(const Vector2 rhs) const;
        Vector2& operator+=(const Vector2 rhs);
        Vector2 operator-(const Vector2 rhs) const;
        Vector2& operator-=(const Vector2 rhs);
        Vector2 operator*(const float scalar) const;
        Vector2& operator*=(const float scalar);
        Vector2 operator/(const float scalar) const;
        Vector2& operator/=(const float scalar);
    public:
        operator glm::vec2() const { return glm::vec2(x, y); }
    public:
        inline glm::vec2 glm_vec2() const { return glm::vec2(x, y); }
    };

    Vector2 operator-(const Vector2& vector);
    Vector2 operator*(const float scalar, const Vector2 vector);

    std::ostream& operator<<(std::ostream& ostream, const Vector2& v);


}