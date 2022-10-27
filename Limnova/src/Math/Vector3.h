#pragma once


namespace Limnova
{

	class Vector3
	{
	public:
		Vector3() : x(0), y(0), z(0) {}
		Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

		float x, y, z;

		inline float SqrMagnitude() const { return x * x + y * y + z * z; }

		// Normalized() returns a normalized copy of a vector.
		Vector3 Normalized() const;
		// Normalize() normalizes a vector in-place and returns it by reference.
		Vector3& Normalize();

		float Dot(const Vector3 rhs) const;
		Vector3 Cross(const Vector3 rhs) const;

		Vector3 operator+(const Vector3 rhs) const;
		Vector3& operator+=(const Vector3 rhs);
		Vector3 operator*(const float scalar) const;
		Vector3& operator*=(const float scalar);
		Vector3 operator/(const float scalar) const;
		Vector3& operator/=(const float scalar);
	};


	std::ostream& operator<<(std::ostream& ostream, const Vector3& q);
	Vector3 operator*(const float scalar, const Vector3 vector);

}