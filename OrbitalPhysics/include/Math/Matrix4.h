#pragma once

#include "Quaternion.h"
#include "Vector4.h"

#include "glm.h"

namespace Limnova
{

class Matrix4
{
public:
	constexpr Matrix4();
	constexpr Matrix4(Matrix4 const& rhs);
	constexpr Matrix4(glm::mat4 const& glmMat4);
	Matrix4(Quaternion const& quaternion);

	static constexpr Matrix4 Identity();

	float* Ptr();
	Matrix4 Inverse() const;

	Vector4 operator*(Vector4 const& rhs) const;
	Matrix4 operator*(Matrix4 const& rhs) const;

	operator glm::mat4() const;

	glm::mat4 glm_mat4() const;

	glm::mat4 m_mat4;
};

// ---------------------------------------------------------------------------------------------------------------------------------

inline constexpr Matrix4::Matrix4() :
	m_mat4()
{
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline constexpr Matrix4::Matrix4(Matrix4 const& rhs) :
	m_mat4(rhs.m_mat4)
{
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline constexpr Matrix4::Matrix4(glm::mat4 const& glmMat4) :
	m_mat4(glmMat4)
{
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline constexpr Matrix4 Matrix4::Identity()
{
	return Matrix4(glm::identity<glm::mat4>());
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline float* Matrix4::Ptr()
{
	return glm::value_ptr(m_mat4);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline Matrix4 Matrix4::Inverse() const
{
	return Matrix4(glm::inverse(m_mat4));
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline Vector4 Matrix4::operator*(Vector4 const& rhs) const
{
	return Vector4(this->m_mat4 * rhs.glm_vec4());
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline Matrix4 Matrix4::operator*(Matrix4 const& rhs) const
{
	return Matrix4(this->m_mat4 * rhs.m_mat4);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline Matrix4::operator glm::mat4() const
{
	return m_mat4;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline glm::mat4 Matrix4::glm_mat4() const
{
	return m_mat4;
}

}
