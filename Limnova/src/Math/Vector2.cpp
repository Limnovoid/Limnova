#include "Vector2.h"


namespace Limnova
{

#ifdef EXPLICIT_DECL
    Vector2 Vector2::Normalized() const
    {
        float sqrmag = this->SqrMagnitude();
        if (sqrmag == 0) { return *this; }
        return (*this) / sqrt(sqrmag);
    }


    Vector2& Vector2::Normalize()
    {
        float mag = this->SqrMagnitude();
        if (mag == 0) { return *this; }

        mag = sqrt(mag);
        this->x /= mag;
        this->y /= mag;
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


    Vector2 operator-(const Vector2& vector)
    {
        return vector * -1.f;
    }


    Vector2 operator*(const float scalar, const Vector2 vector)
    {
        return vector * scalar;
    }


    bool operator==(const Vector2& lhs, const Vector2& rhs)
    {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }


    bool operator!=(const Vector2& lhs, const Vector2& rhs)
    {
        return lhs.x != rhs.x || lhs.y != rhs.y;
    }


    std::ostream& operator<<(std::ostream& ostream, const Vector2& v)
    {
        ostream << "(" << v.x << " " << v.y << ")";
        return ostream;
    }
#endif

}