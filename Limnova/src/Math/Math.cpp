#include "Math.h"


namespace Limnova
{

    BigFloat WrapBf(BigFloat x, const BigFloat& lowerBound, const BigFloat& upperBound)
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


    float Wrapf(float x, float lowerBound, float upperBound)
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


    uint32_t Factorial(uint32_t x)
    {
        switch (x)
        {
        case 0: return 1;
        case 1: return 1;
        case 2: return 2;
        case 3: return 6;
        case 4: return 24;
        case 5: return 120;
        case 6: return 720;
        case 7: return 5040;
        case 8: return 40320;
        case 9: return 362880;
        }
        uint32_t res = x--;
        while (x > 9)
        {
            res *= x--;
        }
        return res * 362880;
    }


    Vector3 Rotate(const Vector3 vec, const Vector3 rAxis, const float rAngleRad)
    {
        Quaternion r(rAxis, rAngleRad);
        return r.RotateVector(vec);
    }

}