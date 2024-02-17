#include "Math/Matrix4.h"

#include "Math/Vector4.h"

namespace Limnova
{

	Vector4 Matrix4::operator*(const Vector4& rhs) const
	{
		return Vector4(this->mat * rhs.glm_vec4());
	}


	Matrix4 Matrix4::operator*(const Matrix4& rhs) const
	{
		return Matrix4(this->mat * rhs.mat);
	}

}
