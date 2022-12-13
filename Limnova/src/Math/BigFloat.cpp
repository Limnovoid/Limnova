#include "BigFloat.h"


static constexpr float kRoot10 = 3.16227766;


namespace Limnova
{

    BigFloat::BigFloat(float value)
        : m_Coefficient(value), m_Exponent(0)
    {
        if (m_Coefficient != 0)
        {
            while (abs(m_Coefficient) >= 10.f)
            {
                m_Coefficient /= 10.f;
                m_Exponent++;
            }
            while (abs(m_Coefficient) < 1.f)
            {
                m_Coefficient *= 10.f;
                m_Exponent--;
            }
        }
    }


    BigFloat::BigFloat(float coefficient, int exponent)
        : m_Coefficient(coefficient), m_Exponent(exponent)
    {
        if (m_Coefficient == 0)
        {
            m_Exponent = 0;
            return;
        }
        while (abs(m_Coefficient) >= 10.f)
        {
            m_Coefficient /= 10.f;
            m_Exponent++;
        }
        while (abs(m_Coefficient) < 1.f)
        {
            m_Coefficient *= 10.f;
            m_Exponent--;
        }
    }


    BigFloat BigFloat::Pow(const BigFloat& value, const int power)
    {
        float coef = pow(value.m_Coefficient, power);
        int exp = value.m_Exponent * power;
        while (abs(coef) >= 10.f)
        {
            coef /= 10.f;
            exp++;
        }
        return { coef, exp };
    }


    BigFloat BigFloat::Sqrt(const BigFloat& value)
    {
        float coef = sqrt(value.m_Coefficient);
        int exp = value.m_Exponent / 2;
        if (abs(value.m_Exponent) % 2 != 0)
        {
            coef *= kRoot10;
            exp--;
        }
        while (abs(coef) >= 10.f)
        {
            coef /= 10.f;
            exp++;
        }
        return { coef, exp };
    }


    BigFloat BigFloat::Abs(const BigFloat& value)
    {
        BigFloat ret;
        ret.m_Coefficient = abs(value.m_Coefficient);
        ret.m_Exponent = value.m_Exponent;
        return ret;
    }


    BigFloat BigFloat::operator-() const
    {
        BigFloat ret;
        ret.m_Coefficient = -1.f * this->m_Coefficient;
        ret.m_Exponent = this->m_Exponent;
        return ret;
    }


    BigFloat BigFloat::operator*(const float rhs) const
    {
        float coef = this->m_Coefficient * rhs;
        if (coef == 0)
        {
            return { 0.f, 0 };
        }
        return { coef, this->m_Exponent };
    }


    BigFloat BigFloat::operator/(const float rhs) const
    {
        if (this->m_Coefficient == 0)
        {
            return { 0.f, 0 };
        }
        float coef = this->m_Coefficient / rhs;
        return { coef, this->m_Exponent };
    }


    BigFloat BigFloat::operator*(const BigFloat& rhs) const
    {
        float coef = this->m_Coefficient * rhs.m_Coefficient;
        int exp = this->m_Exponent + rhs.m_Exponent;
        return { coef, exp };
    }


    BigFloat& BigFloat::operator*=(const BigFloat& rhs)
    {
        this->m_Coefficient *= rhs.m_Coefficient;
        if (this->m_Coefficient == 0)
        {
            this->m_Exponent = 0;
            return *this;
        }
        this->m_Exponent += rhs.m_Exponent;
        while (abs(this->m_Coefficient) >= 10.f)
        {
            this->m_Coefficient /= 10.f;
            this->m_Exponent++;
        }
        return *this;
    }


    BigFloat BigFloat::operator/(const BigFloat& rhs) const
    {
        if (this->m_Coefficient == 0)
        {
            return { 0.f, 0 };
        }
        float coef = this->m_Coefficient / rhs.m_Coefficient;
        int exp = this->m_Exponent - rhs.m_Exponent;
        return { coef, exp };
    }


    BigFloat& BigFloat::operator/=(const BigFloat& rhs)
    {
        if (this->m_Coefficient == 0)
        {
            return *this;
        }
        this->m_Coefficient /= rhs.m_Coefficient;
        this->m_Exponent -= rhs.m_Exponent;
        while (abs(this->m_Coefficient) < 1.f)
        {
            this->m_Coefficient *= 10.f;
            this->m_Exponent--;
        }
        return *this;
    }


    BigFloat BigFloat::operator+(const BigFloat& rhs) const
    {
        float coef;
        int exp;
        if (rhs.m_Exponent < this->m_Exponent)
        {
            coef = this->m_Coefficient + rhs.m_Coefficient * (float)pow(10.f, rhs.m_Exponent - this->m_Exponent);
            exp = this->m_Exponent;
        }
        else
        {
            coef = this->m_Coefficient * (float)pow(10.f, this->m_Exponent - rhs.m_Exponent) + rhs.m_Coefficient;
            exp = rhs.m_Exponent;
        }

        if (coef == 0)
        {
            return { 0.f, 0 };
        }

        return { coef, exp };
    }


    BigFloat& BigFloat::operator+=(const BigFloat& rhs)
    {
        if (rhs.m_Exponent < this->m_Exponent)
        {
            this->m_Coefficient += rhs.m_Coefficient * (float)pow(10.f, rhs.m_Exponent - this->m_Exponent);
        }
        else
        {
            this->m_Coefficient = this->m_Coefficient * (float)pow(10.f, this->m_Exponent - rhs.m_Exponent) + rhs.m_Coefficient;
            this->m_Exponent = rhs.m_Exponent;
        }

        if (this->m_Coefficient == 0)
        {
            this->m_Exponent = 0;
            return *this;
        }

        while (abs(this->m_Coefficient) >= 10.f)
        {
            this->m_Coefficient /= 10.f;
            this->m_Exponent++;
        }
        while (abs(this->m_Coefficient) < 1.f)
        {
            this->m_Coefficient *= 10.f;
            this->m_Exponent--;
        }
        return *this;
    }


    BigFloat BigFloat::operator-(const BigFloat& rhs) const
    {
        float coef;
        int exp;
        if (rhs.m_Exponent < this->m_Exponent)
        {
            coef = this->m_Coefficient - rhs.m_Coefficient * (float)pow(10.f, rhs.m_Exponent - this->m_Exponent);
            exp = this->m_Exponent;
        }
        else
        {
            coef = this->m_Coefficient * (float)pow(10.f, this->m_Exponent - rhs.m_Exponent) - rhs.m_Coefficient;
            exp = rhs.m_Exponent;
        }

        if (coef == 0)
        {
            return { 0.f, 0 };
        }

        return { coef, exp };
    }


    BigFloat& BigFloat::operator-=(const BigFloat& rhs)
    {
        if (rhs.m_Exponent < this->m_Exponent)
        {
            this->m_Coefficient -= rhs.m_Coefficient * (float)pow(10.f, rhs.m_Exponent - this->m_Exponent);
        }
        else
        {
            this->m_Coefficient = this->m_Coefficient * (float)pow(10.f, this->m_Exponent - rhs.m_Exponent) - rhs.m_Coefficient;
            this->m_Exponent = rhs.m_Exponent;
        }

        if (this->m_Coefficient == 0)
        {
            this->m_Exponent = 0;
            return *this;
        }

        while (abs(this->m_Coefficient) >= 10.f)
        {
            this->m_Coefficient /= 10.f;
            this->m_Exponent++;
        }
        while (abs(this->m_Coefficient) < 1.f)
        {
            this->m_Coefficient *= 10.f;
            this->m_Exponent--;
        }
        return *this;
    }


    BigFloat operator*(const float lhs, const BigFloat& rhs)
    {
        return rhs * lhs;
    }


    BigFloat operator/(const float lhs, const BigFloat& rhs)
    {
        if (lhs == 0)
        {
            return { 0.f, 0 };
        }
        float coef = lhs / rhs.m_Coefficient;
        int exp = -rhs.m_Exponent;
        return { coef, exp };
    }


    std::ostream& operator<<(std::ostream& ostream, const BigFloat& bf)
    {
        ostream << bf.GetCoefficient() << "E" << bf.GetExponent();
        return ostream;
    }

}
