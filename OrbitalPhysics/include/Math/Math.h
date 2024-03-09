#pragma once

#include "MathConstants.h"
#include "Matrix4.h"
#include "Quaternion.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

#include <functional>

namespace Limnova
{

inline double Radians(double degrees)
{
	return degrees * PI / 180.0;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline float Radiansf(float degrees)
{
	return degrees * PIf / 180.f;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline Vector3 RadiansVec3(const Vector3& degrees)
{
	return degrees * PIf / 180.f;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline double Degrees(double radians)
{
	return radians * 180.0 / PI;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline float Degreesf(float radians)
{
	return radians * 180.f / PIf;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline Vector3 DegreesVec3(Vector3 radians)
{
	return radians * 180.f / PIf;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline double Wrap(double x, double lowerBound, double upperBound)
{
	if (x < lowerBound)
	{
		x += upperBound - lowerBound;
	}
	else if (x >= upperBound)
	{
		x -= upperBound - lowerBound;
	}
	return x;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline float Wrapf(float x, float lowerBound, float upperBound)
{
	float range = upperBound - lowerBound;

	if (x < lowerBound - lowerBound * kEps)
		x += range;
	else if (x > upperBound - upperBound * kEps)
		x -= range;

	return x;
}

// ---------------------------------------------------------------------------------------------------------------------------------

/// <summary> Wraps 'x' in the range [0, upperBound). Assumes x > 0. </summary>
inline float Wrapf(float x, float upperBound)
{
	if (x > upperBound - upperBound * kEps)
		x -= upperBound;

	return x;
}

// ---------------------------------------------------------------------------------------------------------------------------------

/// <summary> Wraps 'x' in the range [0, upperBound). </summary>
inline int Wrapi(int x, int upperBound)
{
	if (x >= upperBound)
		x -= upperBound;

	return x;
}

// ---------------------------------------------------------------------------------------------------------------------------------

/// <summary> Wraps 'x' in the range [lowerBound, upperBound). </summary>
inline int Wrapi(int x, int lowerBound, int upperBound)
{
	int range = upperBound - lowerBound;

	if (x < lowerBound)
		x += range;
	else if (x >= upperBound)
		x -= range;

	return x;
}

// ---------------------------------------------------------------------------------------------------------------------------------

uint32_t Factorial(uint32_t x);

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T, typename COEFFICIENT, typename EXPONENT>
::std::pair<COEFFICIENT, EXPONENT> ToScientific(T x)
{
	COEFFICIENT coefficient = x;
	EXPONENT exponent = 0;

	if (coefficient != 0)
	{
		while (abs(coefficient) >= 10.f)
		{
			coefficient /= 10.f;
			exponent++;
		}
		while (abs(coefficient) < 1.f)
		{
			coefficient *= 10.f;
			exponent--;
		}
	}

	return { coefficient, exponent };
}

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T, typename Coefficient, typename Exponent>
T FromScientific(Coefficient c, Exponent e)
{
	return c * pow(10.0, e);
}

// ---------------------------------------------------------------------------------------------------------------------------------

/// <summary> Equivalent to abs(signedVariable) > unsignedConstant. </summary>
template<typename T>
bool AbsGreaterThan(T signedLhs, T unsignedRhs)
{
	if (signedLhs < 0.f)
		signedLhs = -signedLhs;

	return signedLhs > unsignedRhs;
}

// ---------------------------------------------------------------------------------------------------------------------------------

/// <summary> Equivalent to abs(lhs) > abs(rhs). </summary>
template<typename T>
bool AbsGreaterThan2(T lhs, T rhs)
{
	if (lhs < 0.f)
		lhs = -lhs;

	if (rhs < 0.f)
		rhs = -rhs;

	return lhs > rhs;
}

// Vector operations -----------------------------------------------------------------------------------------------------------

/// <summary> Rotate a vector by a given angle about a given axis. </summary>
Vector3 Rotate(const Vector3 vector, const Vector3 rotationAxis, const float rotationAngle);

// ---------------------------------------------------------------------------------------------------------------------------------

/// <summary> Returns the shortest-arc rotation from the start vector to the end vector. </summary>
Quaternion Rotation(const Vector3& start, const Vector3& end);

// ---------------------------------------------------------------------------------------------------------------------------------

/// <summary>
/// Returns the angle (radians) between two unit vectors.
/// Created specifically to handle floating point error which can result in a dot product being greater than 1 for parallel or near-parallel unit vectors.
/// </summary>
inline float AngleBetweenUnitVectorsf(const Vector3& u0, const Vector3& u1)
{
	return ::std::acosf(::std::min(1.f, u0.Dot(u1)));
}

// ---------------------------------------------------------------------------------------------------------------------------------

/// <summary>
/// Returns the angle (radians) between two unit vectors.
/// Created specifically to handle floating point error which can result in a dot product being greater than 1 for parallel or near-parallel unit vectors.
/// </summary>
inline double AngleBetweenUnitVectors(const Vector3d& u0, const Vector3d& u1)
{
	return ::std::acos(::std::min(1.0, u0.Dot(u1)));
}

// ---------------------------------------------------------------------------------------------------------------------------------

bool DecomposeTransform(const Matrix4& transform, Vector3& position, Quaternion& orientation, Vector3& scale);

// ---------------------------------------------------------------------------------------------------------------------------------

template<typename T>
T SolveNetwon(std::function<T(T)> function, std::function<T(T)> functionFirstDerivative, T initialX, T tolerance, size_t nMaxIterations)
{
	assert(functionFirstDerivative(initialX) != 0.f);

	T x = initialX;
	T fx = function(initialX);
	for (size_t i = 0; AbsGreaterThan(fx, tolerance) && (i < nMaxIterations); ++i)
	{
		T f_1dx = functionFirstDerivative(x);

		if (f_1dx == 0.f)
			x -= -tolerance;
		else
			x = x - fx / f_1dx;

		fx = function(x);
	}
	return x;
}

}
