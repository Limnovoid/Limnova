#pragma once

#include "MathConstants.h"

#include "BigFloat.h"
#include "BigVector2.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4.h"
#include "Quaternion.h"


namespace Limnova
{

    inline double Radians(double degrees) { return degrees * PI / 180.0; }
    inline float Radiansf(float degrees) { return degrees * PIf / 180.f; }
    inline double Degrees(double radians) { return radians * 180.0 / PI; }
    inline float Degreesf(float radians) { return radians * 180.f / PIf; }

    BigFloat WrapBf(BigFloat x, const BigFloat& lowerBound, const BigFloat& upperBound);
    double Wrap(double x, double lowerBound, double upperBound);
    float Wrapf(float x, float lowerBound, float upperBound);

    uint32_t Factorial(uint32_t x);

    /// <summary>
    /// Rotate a vector. Implemented with quaternions.
    /// </summary>
    /// <param name="vec">Original vector</param>
    /// <param name="rAxis">Rotation axis: must have MAGNITUDE 1</param>
    /// <param name="rAngleRad">Rotation angle in radians</param>
    /// <returns>A copy of the vector, rotated as per the arguments</returns>
    Vector3 Rotate(const Vector3 vector, const Vector3 rAxis, const float rAngleRadians);

    template<typename T, typename Coefficient, typename Exponent>
    std::pair<Coefficient, Exponent> ToScientific(T x)
    {
        Coefficient coefficient = x;
        Exponent exponent = 0;
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

    template<typename T, typename Coefficient, typename Exponent>
    T FromScientific(Coefficient c, Exponent e)
    {
        return c * pow(10.0, e);
    }
}
