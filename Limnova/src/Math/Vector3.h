#pragma once

#include "Vector2.h"
#include "glm.h"


namespace Limnova
{

    template<typename T>
    class TVector3;

    using Vector3 = TVector3<float>;
    using Vector3d = TVector3<double>;


    template<typename T>
    TVector3<T> operator*(const T scalar, const TVector3<T>& vector) { return vector * scalar; }
    template<typename T>
    TVector3<T> operator-(const TVector3<T>& value) { return { -value.x, -value.y, -value.z }; }

    template<typename T>
    bool operator==(const TVector3<T>& lhs, const TVector3<T>& rhs) {
        return this->x == rhs.x && this->y == rhs.y && this->z == rhs.z;
    }
    template<typename T>
    bool operator!=(const TVector3<T>& lhs, const TVector3<T>& rhs) {
        return this->x != rhs.x || this->y != rhs.y || this->z != rhs.z;
    }

    template<typename T>
    std::ostream& operator<<(std::ostream& ostream, const TVector3<T>& v) {
        ostream << "(" << v.x << " " << v.y << " " << v.z << ")";
        return ostream;
    }


    template<typename T>
    class TVector3
    {
    public:
        T x, y, z;
    public:
        constexpr TVector3() : x(0), y(0), z(0) {}
        constexpr TVector3(T v) : x(v), y(v), z(v) {}
        constexpr TVector3(T x, T y, T z) : x(x), y(y), z(z) {}
        constexpr TVector3(Vector2& vec2, T z) : x(vec2.x), y(vec2.y), z(z) {}
        constexpr TVector3(glm::tvec3<T>& glmv) : x(glmv.x), y(glmv.y), z(glmv.z) {}

        template<typename S>
        constexpr TVector3(TVector3<S> v) : x((S)v.x), y((S)v.y), z((S)v.z) {}

        TVector2<T> XY() const { return { x, y }; }
        T* Ptr() { return &x; }

        inline T SqrMagnitude() const { return x * x + y * y + z * z; }

        // Normalized() returns a normalized copy of a vector.
        TVector3 Normalized() const {
            T sqrMag = this->SqrMagnitude();
            if (sqrMag == 0) {
                return *this;
            }
            return (*this) / sqrt(sqrMag);
        }

        // Normalize() normalizes a vector in-place and returns it by reference.
        TVector3& Normalize() {
            T sqrMag = this->SqrMagnitude();
            if (sqrMag == 0) {
                return *this;
            }
            return (*this) /= sqrt(sqrMag);
        }

        float Dot(const TVector3 rhs) const {
            return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z;
        }
        TVector3 Cross(const TVector3 rhs) const {
            return TVector3(
                this->y * rhs.z - this->z * rhs.y,
                this->z * rhs.x - this->x * rhs.z,
                this->x * rhs.y - this->y * rhs.x
            );
        }
    public:
        static TVector3 Cross(const TVector3 lhs, const TVector3 rhs) {
            return TVector3(
                lhs.y * rhs.z - lhs.z * rhs.y,
                lhs.z * rhs.x - lhs.x * rhs.z,
                lhs.x * rhs.y - lhs.y * rhs.x
            );
        }

        static constexpr TVector3 Forward() { return    { 0.0, 0.0,-1.0 }; }
        static constexpr TVector3 Up() { return         { 0.0, 1.0, 0.0 }; }
        static constexpr TVector3 Left() { return       {-1.0, 0.0, 0.0 }; }
        static constexpr TVector3 Backward() { return   { 0.0, 0.0, 1.0 }; }
        static constexpr TVector3 Down() { return       { 0.0,-1.0, 0.0 }; }
        static constexpr TVector3 Right() { return      { 1.0, 0.0, 0.0 }; }
    public:
        TVector3& operator=(const glm::vec3& rhs) {
            this->x = rhs.x;
            this->y = rhs.y;
            this->z = rhs.z;
            return *this;
        }
        TVector3 operator+(const TVector3& rhs) const {
            return TVector3(this->x + rhs.x, this->y + rhs.y, this->z + rhs.z);
        }
        TVector3& operator+=(const TVector3& rhs) {
            this->x += rhs.x;
            this->y += rhs.y;
            this->z += rhs.z;
            return *this;
        }
        TVector3 operator-(const TVector3& rhs) const {
            return TVector3(this->x - rhs.x, this->y - rhs.y, this->z - rhs.z);
        }
        TVector3& operator-=(const TVector3& rhs) {
            this->x -= rhs.x;
            this->y -= rhs.y;
            this->z -= rhs.z;
            return *this;
        }
        TVector3 operator*(const T scalar) const {
            return TVector3(scalar * this->x, scalar * this->y, scalar * this->z);
        }
        TVector3& operator*=(const T scalar) {
            this->x *= scalar;
            this->y *= scalar;
            this->z *= scalar;
            return *this;
        }
        TVector3 operator/(const T scalar) const {
            return TVector3(this->x / scalar, this->y / scalar, this->z / scalar);
        }
        TVector3& operator/=(const T scalar) {
            this->x /= scalar;
            this->y /= scalar;
            this->z /= scalar;
            return *this;
        }

        template<typename T>
        friend TVector3<T> operator-(const TVector3<T>& value);

        template<typename T>
        friend bool operator==(const TVector3<T>& lhs, const TVector3<T>& rhs);
        template<typename T>
        friend bool operator!=(const TVector3<T>& lhs, const TVector3<T>& rhs);
    public:
        operator glm::tvec3<T>() const { return glm::tvec3<T>(x, y, z); }
    public:
        inline glm::tvec3<T> glm_vec3() const { return glm::tvec3<T>(x, y, z); }
    };


#ifdef EXPLICIT_DECL
    class Vector3
    {
    public:
        float x, y, z;
    public:
        constexpr Vector3() : x(0), y(0), z(0) {}
        constexpr Vector3(float v) : x(v), y(v), z(v) {}
        constexpr Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
        constexpr Vector3(Vector2& vec2, float z) : x(vec2.x), y(vec2.y), z(z) {}
        constexpr Vector3(glm::vec3& glmv) : x(glmv.x), y(glmv.y), z(glmv.z) {}

        Vector2 XY() const { return { x, y }; }
        float* Ptr() { return &x; }

        inline float SqrMagnitude() const { return x * x + y * y + z * z; }

        // Normalized() returns a normalized copy of a vector.
        Vector3 Normalized() const;
        // Normalize() normalizes a vector in-place and returns it by reference.
        Vector3& Normalize();

        float Dot(const Vector3 rhs) const;
        Vector3 Cross(const Vector3 rhs) const;
    public:
        static Vector3 Cross(const Vector3 lhs, const Vector3 rhs);

        static constexpr Vector3 Forward()  { return { 0.f, 0.f,-1.f }; }
        static constexpr Vector3 Up()       { return { 0.f, 1.f, 0.f }; }
        static constexpr Vector3 Left()     { return {-1.f, 0.f, 0.f }; }
        static constexpr Vector3 Backward() { return { 0.f, 0.f, 1.f }; }
        static constexpr Vector3 Down()     { return { 0.f,-1.f, 0.f }; }
        static constexpr Vector3 Right()    { return { 1.f, 0.f, 0.f }; }
    public:
        Vector3& operator=(const glm::vec3& rhs);
        Vector3 operator+(const Vector3& rhs) const;
        Vector3& operator+=(const Vector3& rhs);
        Vector3 operator-(const Vector3& rhs) const;
        Vector3& operator-=(const Vector3& rhs);
        Vector3 operator*(const float scalar) const;
        Vector3& operator*=(const float scalar);
        Vector3 operator/(const float scalar) const;
        Vector3& operator/=(const float scalar);

        friend Vector3 operator-(const Vector3& value);

        bool operator==(const Vector3& rhs) const;
    public:
        operator glm::vec3() const { return glm::vec3(x, y, z); }
    public:
        inline glm::vec3 glm_vec3() const { return glm::vec3(x, y, z); }
    };

    inline Vector3 operator*(const float scalar, const Vector3 vector) { return vector * scalar; }
    inline Vector3 operator-(const Vector3& value) { return { -value.x, -value.y, -value.z }; }

    std::ostream& operator<<(std::ostream& ostream, const Vector3& v);
#endif

}
