#include "Matrix4.h"

#include "Vector4.h"


namespace Limnova
{

    Vector4 Matrix4::operator*(const Vector4& rhs) const
    {
        return { this->mat * rhs.glm_vec4() };
    }


    Matrix4 Matrix4::operator*(const Matrix4& rhs) const
    {
        return { this->mat * rhs.mat };
    }

}
