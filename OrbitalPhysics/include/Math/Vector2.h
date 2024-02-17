#pragma once

#include "Math/glm.h"

namespace Limnova
{

	template<typename T>
	class TVector2
	{
	public:
		T x, y;
	public:
		constexpr TVector2() : x(0), y(0) {}
		constexpr TVector2(T v) : x(v), y(v) {}
		constexpr TVector2(T x, T y) : x(x), y(y) {}
		constexpr TVector2(glm::tvec2<T>& glmv) : x(glmv.x), y(glmv.y) {}

		T* Ptr() { return &x; }

		inline T SqrMagnitude() const { return x * x + y * y; }

		// Returns a normalized copy of a vector.
		TVector2 Normalized() const
		{
			T sqrMag = this->SqrMagnitude();
			if (sqrMag == 0) {
				return *this;
			}
			return (*this) / sqrt(sqrMag);
		}

		// Normalizes a vector in-place and returns it by reference.
		TVector2& Normalize()
		{
			T sqrMag = this->SqrMagnitude();
			if (sqrMag == 0) {
				return *this;
			}
			return (*this) /= sqrt(sqrMag);
		}

		T Dot(const TVector2 rhs) const
		{
			return this->x * rhs.x + this->y * rhs.y;
		}

		bool IsZero() const { return x == 0.f && y == 0.f; }

		// Returns the zero vector.
		static TVector2 Zero() { return TVector2{ 0.f, 0.f }; }
	public:
		TVector2 operator+(const TVector2 rhs) const {
			return TVector2(this->x + rhs.x, this->y + rhs.y);
		}
		TVector2& operator+=(const TVector2 rhs) {
			this->x += rhs.x;
			this->y += rhs.y;
			return *this;
		}
		TVector2 operator-(const TVector2 rhs) const {
			return TVector2(this->x - rhs.x, this->y - rhs.y);
		}
		TVector2& operator-=(const TVector2 rhs) {
			this->x -= rhs.x;
			this->y -= rhs.y;
			return *this;
		}
		TVector2 operator*(const T scalar) const {
			return TVector2(scalar * this->x, scalar * this->y);
		}
		TVector2& operator*=(const T scalar) {
			this->x *= scalar;
			this->y *= scalar;
			return *this;
		}
		TVector2 operator/(const T scalar) const {
			return TVector2(this->x / scalar, this->y / scalar);
		}
		TVector2& operator/=(const T scalar) {
			this->x /= scalar;
			this->y /= scalar;
			return *this;
		}

		template<typename T>
		friend bool operator==(const TVector2<T>& lhs, const TVector2<T>& rhs);
		template<typename T>
		friend bool operator!=(const TVector2<T>& lhs, const TVector2<T>& rhs);
	public:
		operator glm::tvec2<T>() const { return glm::tvec2<T>(x, y); }
	public:
		inline glm::tvec2<T> glm_vec2() const { return glm::tvec2<T>(x, y); }
	};

	template<typename T>
	TVector2<T> operator-(const TVector2<T>& vector)
	{
		return vector * -1.f;
	}

	template<typename T>
	TVector2<T> operator*(const T scalar, const TVector2<T> vector)
	{
		return vector * scalar;
	}

	template<typename T>
	bool operator==(const TVector2<T>& lhs, const TVector2<T>& rhs)
	{
		return lhs.x == rhs.x && lhs.y == rhs.y;
	}

	template<typename T>
	bool operator!=(const TVector2<T>& lhs, const TVector2<T>& rhs)
	{
		return lhs.x != rhs.x || lhs.y != rhs.y;
	}

	template<typename T>
	::std::ostream& operator<<(::std::ostream& ostream, const TVector2<T>& v)
	{
		ostream << "(" << v.x << " " << v.y << ")";
		return ostream;
	}

	using Vector2 = TVector2<float>;
	using Vector2d = TVector2<double>;

}
