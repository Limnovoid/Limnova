#include "Math.h"


namespace Limnova
{

    double Wrap(double x, double lowerBound, double upperBound)
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


    float Wrap(float x, float lowerBound, float upperBound)
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


    Vector3 Rotate(const Vector3 vec, const Vector3 rAxis, const float rAngleRad)
    {
        Quaternion r(rAxis, rAngleRad);
        return r.RotateVector(vec);
    }

}