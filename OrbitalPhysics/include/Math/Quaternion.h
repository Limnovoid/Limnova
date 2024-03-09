#pragma once

#include "Vector3.h"

namespace Limnova
{

/* Forced-normalized quaternions */
class Quaternion
{
	friend class Matrix4;
public:
	constexpr Quaternion();
	constexpr Quaternion(Quaternion const& rhs);
	Quaternion(Vector3 const& rotationAxis, float const angleRadians);
	Quaternion(float x, float y, float z, float w);
	constexpr Quaternion(const glm::quat& glmQ);

	/// <summary> Construct the quaternion representation of a vector. </summary>
	Quaternion(Vector3 const& vector);

	/// <summary> Get the unit quaternion (which applies zero rotation). </summary>
	static constexpr Quaternion Unit();

	Vector3 RotateVector(const Vector3 vector) const;

	Quaternion Multiply(const Quaternion& rhs) const;

	Quaternion& operator*=(const Quaternion& rhs);

	friend Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs);

	Quaternion Inverse() const;

	Vector3 ToEulerAngles() const;

	float GetX() const;
	float GetY() const;
	float GetZ() const;
	float GetW() const;

	operator glm::quat() const;

	glm::quat glm_quat() const;

	friend std::ostream& operator<<(std::ostream& ostream, const Quaternion& q);

	void Normalize();

	Vector3		m_vector;
	float		m_scalar;

};

// ---------------------------------------------------------------------------------------------------------------------------------

inline constexpr Quaternion::Quaternion() :
	m_vector(0.f),
	m_scalar(0.f)
{
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline constexpr Quaternion::Quaternion(Quaternion const& rhs) :
	m_vector(rhs.m_vector),
	m_scalar(rhs.m_scalar)
{
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline constexpr Quaternion::Quaternion(const glm::quat& glmQ)
	: m_vector(glmQ.x, glmQ.y, glmQ.z), m_scalar(glmQ.w)
{
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline constexpr Quaternion Quaternion::Unit()
{
	return Quaternion();
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline float Quaternion::GetX() const
{
	return m_vector.x;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline float Quaternion::GetY() const
{
	return m_vector.y;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline float Quaternion::GetZ() const
{
	return m_vector.z;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline float Quaternion::GetW() const
{
	return m_scalar;
}

// ---------------------------------------------------------------------------------------------------------------------------------


inline Quaternion::operator glm::quat() const
{
	return glm::quat(m_scalar, m_vector.x, m_vector.y, m_vector.z);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline glm::quat Quaternion::glm_quat() const
{
	return glm::quat(m_scalar, m_vector.x, m_vector.y, m_vector.z);
}

// ---------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------

inline Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs)
{
	return lhs.Multiply(rhs);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline std::ostream& operator<<(std::ostream& ostream, const Quaternion& q)
{
	ostream << "[" << q.m_vector.x << " " << q.m_vector.y << " " << q.m_vector.z << " " << q.m_scalar << "]";
	return ostream;
}

}
