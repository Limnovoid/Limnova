#include "Quaternion.h"

#include "MathConstants.h"


namespace Limnova
{

    Quaternion::Quaternion(Vector3 vec)
        : v(vec), w(0)
    {
    }


    Vector3 Quaternion::RotateVector(const Vector3 vec) const
    {
        Quaternion vq(vec);
        vq = (*this) * vq * this->Inverse();
        return vq.v;
    }


    Quaternion Quaternion::Multiply(const Quaternion& rhs) const
    {
        Quaternion res;
        res.v = this->w * rhs.v + rhs.w * this->v + this->v.Cross(rhs.v);
        res.w = this->w * rhs.w - this->v.Dot(rhs.v);
        return res;
    }

    Quaternion& Quaternion::operator*=(const Quaternion& rhs)
    {
        Vector3 v = this->w * rhs.v + rhs.w * this->v + this->v.Cross(rhs.v);
        float w = this->w * rhs.w - this->v.Dot(rhs.v);
        this->v = v;
        this->w = w;
        return *this;
    }

    Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs)
    {
        return lhs.Multiply(rhs);
    }


    Quaternion Quaternion::Inverse() const
    {
        Quaternion res = *this;
        res.v *= -1;
        return res;
    }


    Vector3 Quaternion::ToEulerAngles() const
    {
        float sinx_cosy = 2.f * (v.x * w + v.y * v.z);
        float cosx_cosy = 1.f - 2.f * (v.x * v.x + v.y * v.y);
        float rotationX = atan2f(sinx_cosy, cosx_cosy);

        float siny = sqrtf(1.f + 2.f * (v.y * w - v.x * v.z));
        float cosy = sqrtf(1.f - 2.f * (v.y * w - v.x * v.z));
        float rotationY = 2.f * atan2f(siny, cosy) - PIf / 2.f;

        float sinz_cosy = 2.f * (v.z * w + v.x * v.y);
        float cosz_cosy = 1.f - 2.f * (v.y * v.y + v.z * v.z);
        float rotationZ = atan2f(sinz_cosy, cosz_cosy);

        return { rotationX, rotationY, rotationZ };
    }


    void Quaternion::Normalize()
    {
        float mag = sqrt(v.SqrMagnitude() + w * w);
        v /= mag;
        w /= mag;
    }


    std::ostream& operator<<(std::ostream& ostream, const Quaternion& q)
    {
        ostream << "[" << q.v.x << " " << q.v.y << " " << q.v.z << " " << q.w << "]";
        return ostream;
    }

}