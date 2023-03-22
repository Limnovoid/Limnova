#include "Matrix4.h"


namespace Limnova
{

    Matrix4 Matrix4::operator*(const Matrix4& rhs) const
    {
        return { this->mat * rhs.mat };
    }

}
