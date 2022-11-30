#include "Vector2.h"


namespace Limnova
{


    Vector2 Vector2::Normalized() const
    {
        float sqrmag = this->SqrMagnitude();
        if (sqrmag == 0)
            return *this;
        return (*this) / sqrt(sqrmag);
    }


    Vector2& Vector2::Normalize()
    {
        *this = this->Normalized();
        return *this;
    }


    float Vector2::Dot(const Vector2 rhs) const
    {
        return this->x * rhs.x + this->y * rhs.y;
    }


    Vector2 Vector2::operator+(const Vector2 rhs) const
    {
        return Vector2(this->x + rhs.x, this->y + rhs.y);
    }


    Vector2& Vector2::operator+=(const Vector2 rhs)
    {
        this->x += rhs.x;
        this->y += rhs.y;
        return *this;
    }


    Vector2 Vector2::operator-(const Vector2 rhs) const
    {
        return Vector2(this->x - rhs.x, this->y - rhs.y);
    }


    Vector2& Vector2::operator-=(const Vector2 rhs)
    {
        this->x -= rhs.x;
        this->y -= rhs.y;
        return *this;
    }


    Vector2 Vector2::operator*(const float scalar) const
    {
        return Vector2(scalar * this->x, scalar * this->y);
    }


    Vector2& Vector2::operator*=(const float scalar)
    {
        this->x *= scalar;
        this->y *= scalar;
        return *this;
    }


    Vector2 operator*(const float scalar, const Vector2 vector)
    {
        return vector * scalar;
    }


    Vector2 Vector2::operator/(const float scalar) const
    {
        return Vector2(this->x / scalar, this->y / scalar);
    }


    Vector2& Vector2::operator/=(const float scalar)
    {
        this->x /= scalar;
        this->y /= scalar;
        return *this;
    }


    std::ostream& operator<<(std::ostream& ostream, const Vector2& v)
    {
        ostream << "(" << v.x << " " << v.y << ")";
        return ostream;
    }

}