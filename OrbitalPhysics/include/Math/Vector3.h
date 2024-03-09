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

// ---------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
class TVector3
{
public:
	T x, y, z;
public:
	constexpr TVector3() : x(0), y(0), z(0) {}
	constexpr TVector3(T v) : x(v), y(v), z(v) {}
	constexpr TVector3(T x, T y, T z) : x(x), y(y), z(z) {}
	constexpr TVector3(TVector2<T>& vec2, T z) : x(vec2.x), y(vec2.y), z(z) {}
	constexpr TVector3(glm::tvec3<T>& glmv) : x(glmv.x), y(glmv.y), z(glmv.z) {}

	template<typename U>
	constexpr TVector3(TVector3<U> rhs) : x(static_cast<T>(rhs.x)), y(static_cast<T>(rhs.y)), z(static_cast<T>(rhs.z)) {}

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

	T Dot(const TVector3 rhs) const {
		return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z;
	}
	TVector3 Cross(const TVector3 rhs) const {
		return TVector3(
			this->y * rhs.z - this->z * rhs.y,
			this->z * rhs.x - this->x * rhs.z,
			this->x * rhs.y - this->y * rhs.x
		);
	}

	bool IsZero() const {
		return this->x == T(0)
			&& this->y == T(0)
			&& this->z == T(0);
	}
public:
	static TVector3 Cross(const TVector3 lhs, const TVector3 rhs) {
		return TVector3(
			lhs.y * rhs.z - lhs.z * rhs.y,
			lhs.z * rhs.x - lhs.x * rhs.z,
			lhs.x * rhs.y - lhs.y * rhs.x
		);
	}

	static constexpr TVector3 Zero() { return { 0, 0, 0 }; }

	static constexpr TVector3 X() { return { 1, 0, 0 }; }
	static constexpr TVector3 Y() { return { 0, 1, 0 }; }
	static constexpr TVector3 Z() { return { 0, 0, 1 }; }
	static constexpr TVector3 Forward() { return  { 0, 0,-1 }; }
	static constexpr TVector3 Up() { return       { 0, 1, 0 }; }
	static constexpr TVector3 Left() { return     {-1, 0, 0 }; }
	static constexpr TVector3 Backward() { return { 0, 0, 1 }; }
	static constexpr TVector3 Down() { return     { 0,-1, 0 }; }
	static constexpr TVector3 Right() { return    { 1, 0, 0 }; }
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
	//template<typename U>
	//operator TVector3<U>() const { return TVector3<U>(*this); }

	operator glm::tvec3<T>() const { return glm::tvec3<T>(x, y, z); }
public:
	inline glm::tvec3<T> glm_vec3() const { return glm::tvec3<T>(x, y, z); }
};

}
