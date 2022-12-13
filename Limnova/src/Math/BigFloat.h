#pragma once


namespace Limnova
{

    class BigFloat
    {
    public:
        BigFloat() : m_Coefficient(0.f), m_Exponent(0) {}
        BigFloat(float value);
        BigFloat(float coefficient, int exponent);
    public:
        int GetCoefficient() const { return m_Coefficient; }
        int GetExponent() const { return m_Exponent; }

        bool IsZero() const { return m_Coefficient == 0; }

        static BigFloat Pow(const BigFloat& value, const int power);
        static BigFloat Sqrt(const BigFloat& value);
        inline BigFloat Sqrt() { return Sqrt(*this); }

        //operator float() { return m_Coefficient * (float)pow(10.f, m_Exponent); } // casting conflicts with stream operator overload
        float GetFloat() { return m_Coefficient * (float)pow(10.f, m_Exponent); }
        const float GetFloat() const { return m_Coefficient * (float)pow(10.f, m_Exponent); }
    public:
        BigFloat operator*(const BigFloat& rhs) const;
        BigFloat& operator*=(const BigFloat& rhs);
        BigFloat operator/(const BigFloat& rhs) const;
        BigFloat& operator/=(const BigFloat& rhs);

        BigFloat operator+(const BigFloat& rhs) const;
        BigFloat& operator+=(const BigFloat& rhs);
        BigFloat operator-(const BigFloat& rhs) const;
        BigFloat& operator-=(const BigFloat& rhs);
    private:
        float m_Coefficient;
        int m_Exponent;
    };


    std::ostream& operator<<(std::ostream& ostream, const BigFloat& bf);

}
