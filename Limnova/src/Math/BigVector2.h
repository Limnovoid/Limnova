#pragma once

#include "BigFloat.h"

#include "Vector2.h"
#include "glm.h"


namespace Limnova
{

    class BigVector2
    {
    public:
        BigFloat x, y;
    public:
        BigVector2() : x({ 0.f, 0 }), y({ 0.f, 0 }) {}
        BigVector2(BigFloat v) : x(v), y(v) {}
        BigVector2(BigFloat x, BigFloat y) : x(x), y(y) {}
        BigVector2(Vector2 vec) : x(vec.x), y(vec.y) {}
        BigVector2(glm::vec2 glmv) : x(glmv.x), y(glmv.y) {}

        inline BigFloat SqrMagnitude() const { return x * x + y * y; }

        // Returns a normalized copy of a vector.
        BigVector2 Normalized() const;
        // Normalizes a vector in-place and returns it by reference.
        BigVector2& Normalize();

        BigFloat Dot(const BigVector2 rhs) const;

        bool IsZero() const { return x.IsZero() && y.IsZero(); }

        // Returns the zero vector.
        static BigVector2 Zero() { return BigVector2{ {0.f, 0}, {0.f, 0} }; }
    public:
        BigVector2 operator+(const BigVector2 rhs) const;
        BigVector2& operator+=(const BigVector2 rhs);
        BigVector2 operator-(const BigVector2 rhs) const;
        BigVector2& operator-=(const BigVector2 rhs);
        BigVector2 operator*(const BigFloat scalar) const;
        BigVector2& operator*=(const BigFloat scalar);
        BigVector2 operator/(const BigFloat scalar) const;
        BigVector2& operator/=(const BigFloat scalar);
    public:
        operator Vector2() const { return { x.Float(), y.Float() }; }
        operator glm::vec2() const { return { x.Float(), y.Float() }; }
    public:
        inline Vector2 Vector2() const { return { x.Float(), y.Float() }; }
        inline glm::vec2 glm_vec2() const { return { x.Float(), y.Float() }; }
    };


    BigVector2 operator-(const BigVector2& vector);
    BigVector2 operator*(const float scalar, const BigVector2 vector);
    BigVector2 operator*(const BigFloat scalar, const BigVector2 vector);

    std::ostream& operator<<(std::ostream& ostream, const BigVector2& v);

}
