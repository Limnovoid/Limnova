#pragma once

#include "BigFloat.h"

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
        BigVector2(glm::vec2 glmv) : x(glmv.x), y(glmv.y) {}

        inline BigFloat SqrMagnitude() const { return x * x + y * y; }

        // Normalized() returns a normalized copy of a vector.
        BigVector2 Normalized() const;
        // Normalize() normalizes a vector in-place and returns it by reference.
        BigVector2& Normalize();

        BigFloat Dot(const BigVector2 rhs) const;
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
        operator glm::vec2() const { return glm::vec2(x.GetFloat(), y.GetFloat()); }
    public:
        inline glm::vec2 glm_vec2() const { return glm::vec2(x.GetFloat(), y.GetFloat()); }
    };


    BigVector2 operator*(const BigFloat scalar, const BigVector2 vector);

    std::ostream& operator<<(std::ostream& ostream, const BigVector2& v);

}
