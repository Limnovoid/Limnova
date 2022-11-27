#pragma once

#include "Vector3.h"


namespace Limnova
{

    class Quaternion
    {
    public:
        Quaternion();
        Quaternion(Vector3 rotationAxis, float angleRadians);

        Vector3 RotateVector(const Vector3 vec) const;

        Quaternion Multiply(const Quaternion& rhs) const;
        Quaternion& operator*=(const Quaternion& rhs);
        friend Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs);

        Quaternion Inverse() const;

        friend std::ostream& operator<<(std::ostream& ostream, const Quaternion& q);
    private:
        Quaternion(Vector3 vec);
        void Normalize();
    private:
        Vector3 v;
        float w;
    };


    std::ostream& operator<<(std::ostream& ostream, const Quaternion& q);
    Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs);

}