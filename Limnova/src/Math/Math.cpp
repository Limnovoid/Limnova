#include "Math.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm/gtx/matrix_decompose.hpp>


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


    // Vector operations ///////////////////

    Vector3 Rotate(const Vector3 vec, const Vector3 rotationAxis, const float rotationAngle)
    {
        Quaternion r(rotationAxis, rotationAngle);
        return r.RotateVector(vec);
    }


    Quaternion Rotation(const Vector3& start, const Vector3& end)
    {
        float lengthProduct = sqrtf(start.SqrMagnitude() * end.SqrMagnitude());
        float dotProduct = start.Dot(end);
        if (abs(dotProduct) > lengthProduct * kParallelDotProductLimit)
        {
            if (dotProduct > 0.f) {
                // Parallel
                return Quaternion::Unit();
            }
            else {
                // Antiparallel
                Vector3 rotationAxis = (start.Dot(Vector3::X()) > kParallelDotProductLimit)
                    ? start.Cross(Vector3::Y()) : Vector3::X();
                return Quaternion(rotationAxis.Normalized(), PIf);
            }
            /* if-scope always returns */
        }

        Vector3 crossProduct = start.Cross(end);
        return Quaternion(crossProduct.x, crossProduct.y, crossProduct.z, lengthProduct + dotProduct);
    }


    // Matrix operations ///////////////////

    bool DecomposeTransform(const Matrix4& transform, Vector3& position, Quaternion& orientation, Vector3& scale)
    {
        glm::vec3 pos = (glm::vec3)position;
        glm::quat ort = (glm::quat)orientation;
        glm::vec3 scl = (glm::vec3)scale;

        glm::vec3 skew; glm::vec4 persp; // filler
        if (!glm::decompose((glm::mat4)transform, scl, ort, pos, skew, persp))
        {
            LV_CORE_ERROR("Decompose transform failed!");
            return false;
        }

        position = (Vector3)pos;
        orientation = (Quaternion)ort;
        scale = (Vector3)scl;
        return true;
    }

}