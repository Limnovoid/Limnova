#include "Math/Vector4.h"

namespace Limnova
{

	Vector4 Vector4::Normalized() const
	{
		float sqrmag = this->SqrMagnitude();
		if (sqrmag == 0)
			return *this;
		return (*this) / sqrt(sqrmag);
	}


	Vector4& Vector4::Normalize()
	{
		*this = this->Normalized();
		return *this;
	}


	float Vector4::Dot(const Vector4 rhs) const
	{
		return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z + this->w * rhs.w;
	}


	Vector4 Vector4::operator+(const Vector4 rhs) const
	{
		return Vector4(this->x + rhs.x, this->y + rhs.y, this->z + rhs.z, this->w + rhs.w);
	}


	Vector4& Vector4::operator+=(const Vector4 rhs)
	{
		this->x += rhs.x;
		this->y += rhs.y;
		this->z += rhs.z;
		this->w += rhs.w;
		return *this;
	}


	Vector4 Vector4::operator-(const Vector4 rhs) const
	{
		return Vector4(this->x - rhs.x, this->y - rhs.y, this->z - rhs.z, this->w - rhs.w);
	}


	Vector4& Vector4::operator-=(const Vector4 rhs)
	{
		this->x -= rhs.x;
		this->y -= rhs.y;
		this->z -= rhs.z;
		this->w -= rhs.w;
		return *this;
	}


	Vector4 Vector4::operator*(const float scalar) const
	{
		return Vector4(scalar * this->x, scalar * this->y, scalar * this->z, scalar * this->w);
	}


	Vector4& Vector4::operator*=(const float scalar)
	{
		this->x *= scalar;
		this->y *= scalar;
		this->z *= scalar;
		this->w *= scalar;
		return *this;
	}


	Vector4 operator*(const float scalar, const Vector4 vector)
	{
		return vector * scalar;
	}


	Vector4 Vector4::operator/(const float scalar) const
	{
		return Vector4(this->x / scalar, this->y / scalar, this->z / scalar, this->w / scalar);
	}


	Vector4& Vector4::operator/=(const float scalar)
	{
		this->x /= scalar;
		this->y /= scalar;
		this->z /= scalar;
		this->w /= scalar;
		return *this;
	}


	std::ostream& operator<<(std::ostream& ostream, const Vector4& v)
	{
		ostream << "(" << v.x << " " << v.y << " " << v.z << " " << v.w << ")";
		return ostream;
	}

}