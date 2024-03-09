#include "Math/Matrix4.h"

#include "Math/Vector4.h"

namespace Limnova
{

Matrix4::Matrix4(Quaternion const& quaternion) :
	m_mat4(glm::toMat4(glm::quat(quaternion.m_scalar, quaternion.m_vector.x, quaternion.m_vector.y, quaternion.m_vector.z)))
{
}

}
