#include "BigFloat.h"


namespace LVM
{

    BigFloat::BigFloat()
        : m_Coefficient(0.f), m_Exponent(0)
    {
    }


    BigFloat::BigFloat(float coefficient, int exponent)
        : m_Coefficient(coefficient), m_Exponent(exponent)
    {
    }


    BigFloat BigFloat::operator*(const BigFloat& rhs) const
    {
        return { this->m_Coefficient * rhs.m_Coefficient, this->m_Exponent + rhs.m_Exponent };
    }

}
