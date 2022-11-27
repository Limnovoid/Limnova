#include "Math.h"


namespace Limnova
{

    Vector3 Rotate(const Vector3 vec, const Vector3 rAxis, const float rAngleRad)
    {
        Quaternion r(rAxis, rAngleRad);
        return r.RotateVector(vec);
    }

}