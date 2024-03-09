#include "Math/Quaternion.h"

#include "Math/Math.h"
#include "Math/glm.h"

namespace Limnova
{

Quaternion::Quaternion(Vector3 const& rotationAxis, float angleRadians)
	: m_vector(sinf(0.5f * angleRadians) * rotationAxis), m_scalar(cosf(0.5f * angleRadians))
{
}

// ---------------------------------------------------------------------------------------------------------------------------------

Quaternion::Quaternion(float x, float y, float z, float m_scalar)
	: m_vector(x, y, z), m_scalar(m_scalar)
{
	this->Normalize();
}

// ---------------------------------------------------------------------------------------------------------------------------------

Quaternion::Quaternion(Vector3 const& vector) :
	m_vector(vector),
	m_scalar(0)
{
}

// ---------------------------------------------------------------------------------------------------------------------------------

Vector3 Quaternion::RotateVector(const Vector3 vector) const
{
	Quaternion vq(vector);

	vq = (*this) * vq * this->Inverse();

	return vq.m_vector;
}

// ---------------------------------------------------------------------------------------------------------------------------------

Quaternion Quaternion::Multiply(const Quaternion& rhs) const
{
	Quaternion res;
	res.m_vector = (this->m_scalar * rhs.m_vector) + (rhs.m_scalar * this->m_vector) + this->m_vector.Cross(rhs.m_vector);
	res.m_scalar = (this->m_scalar * rhs.m_scalar) - this->m_vector.Dot(rhs.m_vector);
	return res;
}

// ---------------------------------------------------------------------------------------------------------------------------------

Quaternion& Quaternion::operator*=(const Quaternion& rhs)
{
	m_vector = (this->m_scalar * rhs.m_vector) + (rhs.m_scalar * this->m_vector) + this->m_vector.Cross(rhs.m_vector);
	m_scalar = (this->m_scalar * rhs.m_scalar) - this->m_vector.Dot(rhs.m_vector);

	return *this;
}

// ---------------------------------------------------------------------------------------------------------------------------------

Quaternion Quaternion::Inverse() const
{
	Quaternion res(*this);

	res.m_vector = -res.m_vector;

	return res;
}

// ---------------------------------------------------------------------------------------------------------------------------------

Vector3 Quaternion::ToEulerAngles() const
{
	// TODO - fix gimbal lock!

	// Formula from:
	// http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/
	static constexpr float kEulerEpsilon = std::numeric_limits<float>::epsilon() * 0.5f;

	float rotX, rotY, rotZ;

	float test = m_vector.x * m_vector.y + m_vector.z * m_scalar;

	if (test > (0.5f - kEulerEpsilon)) // singularity at north pole
	{
		rotX = 0.f;
		rotY = 2.f * atan2f(m_vector.x, m_scalar);
		rotZ = PIf / 2.f;
		return Vector3(rotX, rotY, rotZ);
	}

	if (test < (kEulerEpsilon - 0.5f)) // singularity at south pole
	{
		rotX = 0.f;
		rotY = Wrapf(-2.f * atan2f(m_vector.x, m_scalar), 0.f, PI2f);
		rotZ = PIf * 3.f / 2.f;
		return Vector3(rotX, rotY, rotZ);
	}

	double sqx = m_vector.x * m_vector.x;
	double sqy = m_vector.y * m_vector.y;
	double sqz = m_vector.z * m_vector.z;

	rotX = atan2f((2.f * m_vector.x * m_scalar) - (2.f * m_vector.y * m_vector.z), 1.f - (2.f * sqx) - (2.f * sqz));
	rotY = atan2f((2.f * m_vector.y * m_scalar) - (2.f * m_vector.x * m_vector.z), 1.f - (2.f * sqy) - (2.f * sqz));
	rotZ = asinf(2.f * test);

	return Vector3(rotX, rotY, rotZ);
}

// ---------------------------------------------------------------------------------------------------------------------------------

void Quaternion::Normalize()
{
	float magnitude = sqrtf(m_vector.SqrMagnitude() + (m_scalar * m_scalar));

	m_vector /= magnitude;
	m_scalar /= magnitude;
}

}