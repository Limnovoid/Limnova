#include "BigVector2.h"


namespace Limnova
{

    BigVector2 BigVector2::Normalized() const
    {
        BigFloat sqrmag = this->SqrMagnitude();
        if (sqrmag.IsZero())
            return *this;
        return (*this) / BigFloat::Sqrt(sqrmag);
    }


    BigVector2& BigVector2::Normalize()
    {
        *this = this->Normalized();
        return *this;
    }


    BigFloat BigVector2::Dot(const BigVector2 rhs) const
    {
        return this->x * rhs.x + this->y * rhs.y;
    }


    BigVector2 BigVector2::operator+(const BigVector2 rhs) const
    {
        return { this->x + rhs.x, this->y + rhs.y };
    }


    BigVector2& BigVector2::operator+=(const BigVector2 rhs)
    {
        this->x += rhs.x;
        this->y += rhs.y;
        return *this;
    }


    BigVector2 BigVector2::operator-(const BigVector2 rhs) const
    {
        return { this->x - rhs.x, this->y - rhs.y };
    }


    BigVector2& BigVector2::operator-=(const BigVector2 rhs)
    {
        this->x -= rhs.x;
        this->y -= rhs.y;
        return *this;
    }


    BigVector2 BigVector2::operator*(const BigFloat scalar) const
    {
        return { scalar * this->x, scalar * this->y };
    }


    BigVector2& BigVector2::operator*=(const BigFloat scalar)
    {
        this->x *= scalar;
        this->y *= scalar;
        return *this;
    }


    BigVector2 BigVector2::operator/(const BigFloat scalar) const
    {
        return { this->x / scalar, this->y / scalar };
    }


    BigVector2& BigVector2::operator/=(const BigFloat scalar)
    {
        this->x /= scalar;
        this->y /= scalar;
        return *this;
    }


    BigVector2 operator-(const BigVector2& vector)
    {
        return { -vector.x, -vector.y };
    }


    BigVector2 operator*(const float scalar, const BigVector2 vector)
    {
        return vector * (BigFloat)scalar;
    }


    BigVector2 operator*(const BigFloat scalar, const BigVector2 vector)
    {
        return vector * scalar;
    }


    std::ostream& operator<<(std::ostream& ostream, const BigVector2& v)
    {
        ostream << "(" << v.x << " " << v.y << ")";
        return ostream;
    }

}
