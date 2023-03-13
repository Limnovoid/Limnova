#pragma once

#include "Vector3.h"


namespace Limnova
{

    class Quaternion
    {
    private:
        Vector3 v = { 0.f };
        float w = 1.f;

        friend class Matrix4;
    public:
        constexpr Quaternion() = default;
        constexpr Quaternion(const Quaternion&) = default;
        Quaternion(Vector3 rotationAxis, float angleRadians)
            : v(sinf(0.5f * angleRadians) * rotationAxis), w(cosf(0.5f * angleRadians)) {}

        Vector3 RotateVector(const Vector3 vec) const;

        Quaternion Multiply(const Quaternion& rhs) const;
        Quaternion& operator*=(const Quaternion& rhs);
        friend Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs);

        Quaternion Inverse() const;

        static constexpr Quaternion Unit() { return Quaternion(); } // Returns the unit quaternion (which applies zero rotation).
    private:
        Quaternion(Vector3 vec);
        void Normalize();
    public:
        friend std::ostream& operator<<(std::ostream& ostream, const Quaternion& q);
    };


    std::ostream& operator<<(std::ostream& ostream, const Quaternion& q);
    Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs);

}