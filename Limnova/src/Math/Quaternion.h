#pragma once

#include "Vector3.h"


namespace Limnova
{

    /* Forced-normalized quaternions */
    class Quaternion
    {
    private:
        Vector3 v = { 0.f };
        float w = 1.f;

        friend class Matrix4;
    public:
        constexpr Quaternion() = default;
        constexpr Quaternion(const Quaternion&) = default;
        Quaternion(Vector3 rotationAxis, float angleRadians);
        Quaternion(float x, float y, float z, float w);
        constexpr Quaternion(const glm::quat& glmQ)
            : v(glmQ.x, glmQ.y, glmQ.z), w(glmQ.w)
        {
        }

        Vector3 RotateVector(const Vector3 vec) const;

        Quaternion Multiply(const Quaternion& rhs) const;
        Quaternion& operator*=(const Quaternion& rhs);
        friend Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs);

        Quaternion Inverse() const;

        static constexpr Quaternion Unit() { return Quaternion(); } // Returns the unit quaternion (which applies zero rotation).

        Vector3 ToEulerAngles() const;

        float GetX() const { return v.x; }
        float GetY() const { return v.y; }
        float GetZ() const { return v.z; }
        float GetW() const { return w; }
    private:
        Quaternion(Vector3 vec);
        void Normalize();
    public:
        operator glm::quat() const { return glm::quat(w, v.x, v.y, v.z); }
    public:
        inline glm::quat glm_quat() const { return glm::quat(w, v.x, v.y, v.z); }
    public:
        friend std::ostream& operator<<(std::ostream& ostream, const Quaternion& q);
    };


    std::ostream& operator<<(std::ostream& ostream, const Quaternion& q);
    Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs);

}