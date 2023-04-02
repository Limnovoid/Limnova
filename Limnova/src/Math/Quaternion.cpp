#include "Quaternion.h"

#include "Math.h"
#include "MathConstants.h"


namespace Limnova
{

    Quaternion::Quaternion(Vector3 vec)
        : v(vec), w(0)
    {
    }


    Quaternion::Quaternion(Vector3 rotationAxis, float angleRadians)
        : v(sinf(0.5f * angleRadians)* rotationAxis), w(cosf(0.5f * angleRadians))
    {
    }


    Quaternion::Quaternion(float x, float y, float z, float w)
        : v(x, y, z), w(w)
    {
        this->Normalize();
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
        // TODO - fix gimbal lock!

        // Formula from:
        // http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/
        static constexpr float kEulerEpsilon = std::numeric_limits<float>::epsilon() * 0.5f;

        float rotX, rotY, rotZ;

        float test = v.x * v.y + v.z * w;
        if (test > 0.5f - kEulerEpsilon) { // singularity at north pole
            rotX = 0.f;
            rotY = 2.f * atan2f(v.x, w);
            rotZ = PIf / 2.f;
            return { rotX, rotY, rotZ };
        }
        if (test < kEulerEpsilon - 0.5f) { // singularity at south pole
            rotX = 0.f;
            rotY = Wrapf(-2.f * atan2f(v.x, w), 0.f, PI2f);
            rotZ = PIf * 3.f / 2.f;
            return { rotX, rotY, rotZ };
        }
        double sqx = v.x * v.x;
        double sqy = v.y * v.y;
        double sqz = v.z * v.z;
        rotX = atan2f(2.f * v.x * w - 2.f * v.y * v.z, 1.f - 2.f * sqx - 2.f * sqz);
        rotY = atan2f(2.f * v.y * w - 2.f * v.x * v.z, 1.f - 2.f * sqy - 2.f * sqz);
        rotZ = asinf(2.f * test);
        return { rotX, rotY, rotZ };
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