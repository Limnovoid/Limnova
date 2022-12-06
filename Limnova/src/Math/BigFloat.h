#pragma once


namespace LVM
{

    class BigFloat
    {
    public:
        BigFloat();
        BigFloat(float coefficient, int exponent);

        int GetCoefficient() const { return m_Coefficient; }
        int GetExponent() const { return m_Exponent; }

        operator float() { return m_Coefficient * (float)pow(10.f, m_Exponent); }
        float GetFloat() { return (float)*this; }

        BigFloat operator*(const BigFloat& rhs) const;
    private:
        float m_Coefficient;
        int m_Exponent;
    };

}
