#include "BigFloat.h"


namespace Limnova
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
        float coef = this->m_Coefficient * rhs.m_Coefficient;
        int exp = this->m_Exponent + rhs.m_Exponent;
        while (coef >= 10.f)
        {
            coef /= 10.f;
            exp++;
        }
        return { coef, exp };
    }


    BigFloat BigFloat::operator/(const BigFloat& rhs) const
    {
        float coef = this->m_Coefficient / rhs.m_Coefficient;
        int exp = this->m_Exponent - rhs.m_Exponent;
        while (coef < 10.f)
        {
            coef *= 10.f;
            exp--;
        }
        return { coef, exp };
    }


    std::ostream& operator<<(std::ostream& ostream, const BigFloat& bf)
    {
        ostream << bf.GetCoefficient() << "E" << bf.GetExponent();
        return ostream;
    }

}
