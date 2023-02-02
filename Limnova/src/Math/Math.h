#pragma once

#include "MathConstants.h"

#include "BigFloat.h"
#include "BigVector2.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Quaternion.h"


namespace Limnova
{

    double Wrap(double x, double lowerBound, double upperBound); // Return x wrapped to the range [0, 2PI]
    float Wrap(float x, float lowerBound, float upperBound); // Return x wrapped to the range [0, 2PI]

    /// <summary>
    /// Rotate a vector. Implemented with quaternions.
    /// </summary>
    /// <param name="vec">Original vector</param>
    /// <param name="rAxis">Rotation axis: must have MAGNITUDE 1</param>
    /// <param name="rAngleRad">Rotation angle in radians</param>
    /// <returns>A copy of the vector, rotated as per the arguments</returns>
    Vector3 Rotate(const Vector3 vector, const Vector3 rAxis, const float rAngleRadians);

}
