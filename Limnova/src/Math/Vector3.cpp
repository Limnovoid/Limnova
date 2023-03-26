#include "Vector3.h"


namespace Limnova
{

#ifdef EXPLICIT_DECL
    Vector3 Vector3::Normalized() const
    {
        float sqrmag = this->SqrMagnitude();
        if (sqrmag == 0)
            return *this;
        return (*this) / sqrt(sqrmag);
    }


    Vector3& Vector3::Normalize()
    {
        *this = this->Normalized();
        return *this;
    }


    float Vector3::Dot(const Vector3 rhs) const
    {
        return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z;
    }


    Vector3 Vector3::Cross(const Vector3 rhs) const
    {
        return Vector3(
            this->y * rhs.z - this->z * rhs.y,
            this->z * rhs.x - this->x * rhs.z,
            this->x * rhs.y - this->y * rhs.x
        );
    }


    Vector3 Vector3::Cross(const Vector3 lhs, const Vector3 rhs)
    {
        return Vector3(
            lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.z * rhs.x - lhs.x * rhs.z,
            lhs.x * rhs.y - lhs.y * rhs.x
        );
    }


    Vector3& Vector3::operator=(const glm::vec3& rhs)
    {
        this->x = rhs.x;
        this->y = rhs.y;
        this->z = rhs.z;
        return *this;
    }


    Vector3 Vector3::operator+(const Vector3& rhs) const
    {
        return Vector3(this->x + rhs.x, this->y + rhs.y, this->z + rhs.z);
    }


    Vector3& Vector3::operator+=(const Vector3& rhs)
    {
        this->x += rhs.x;
        this->y += rhs.y;
        this->z += rhs.z;
        return *this;
    }


    Vector3 Vector3::operator-(const Vector3& rhs) const
    {
        return Vector3(this->x - rhs.x, this->y - rhs.y, this->z - rhs.z);
    }


    Vector3& Vector3::operator-=(const Vector3& rhs)
    {
        this->x -= rhs.x;
        this->y -= rhs.y;
        this->z -= rhs.z;
        return *this;
    }


    Vector3 Vector3::operator*(const float scalar) const
    {
        return Vector3(scalar * this->x, scalar * this->y, scalar * this->z);
    }


    Vector3& Vector3::operator*=(const float scalar)
    {
        this->x *= scalar;
        this->y *= scalar;
        this->z *= scalar;
        return *this;
    }


    Vector3 Vector3::operator/(const float scalar) const
    {
        return Vector3(this->x / scalar, this->y / scalar, this->z / scalar);
    }


    Vector3& Vector3::operator/=(const float scalar)
    {
        this->x /= scalar;
        this->y /= scalar;
        this->z /= scalar;
        return *this;
    }


    bool Vector3::operator==(const TVector3& rhs) const
    {
        return this->x == rhs.x && this->y == rhs.y && this->z == rhs.z;
    }


    std::ostream& operator<<(std::ostream& ostream, const Vector3& v)
    {
        ostream << "(" << v.x << " " << v.y << " " << v.z << ")";
        return ostream;
    }
#endif

}