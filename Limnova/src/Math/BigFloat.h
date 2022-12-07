#pragma once


namespace Limnova
{

    class BigFloat
    {
    public:
        BigFloat();
        BigFloat(float coefficient, int exponent);

        int GetCoefficient() const { return m_Coefficient; }
        int GetExponent() const { return m_Exponent; }

        //operator float() { return m_Coefficient * (float)pow(10.f, m_Exponent); }
        float GetFloat() { return m_Coefficient * (float)pow(10.f, m_Exponent); }
        const float GetFloat() const { return m_Coefficient * (float)pow(10.f, m_Exponent); }

        BigFloat operator*(const BigFloat& rhs) const;
        BigFloat operator/(const BigFloat& rhs) const;
    private:
        float m_Coefficient;
        int m_Exponent;
    };


    std::ostream& operator<<(std::ostream& ostream, const BigFloat& bf);

}
