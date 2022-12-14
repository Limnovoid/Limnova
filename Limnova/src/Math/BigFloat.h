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
        float GetCoefficient() const { return m_Coefficient; }
        int GetExponent() const { return m_Exponent; }

        bool IsZero() const { return m_Coefficient == 0; }

        static BigFloat Pow(const BigFloat& value, const int power);
        static BigFloat Sqrt(const BigFloat& value);
        inline BigFloat Sqrt() { return Sqrt(*this); }
        static BigFloat Abs(const BigFloat& value);

        //operator float() { return m_Coefficient * (float)pow(10.f, m_Exponent); } // casting conflicts with stream operator overload
        float Float() { return m_Coefficient * (float)pow(10.f, m_Exponent); }
        const float Float() const { return m_Coefficient * (float)pow(10.f, m_Exponent); }
    public:
        BigFloat operator-() const;

        BigFloat operator*(const float rhs) const;
        BigFloat operator/(const float rhs) const;

        BigFloat operator*(const BigFloat& rhs) const;
        BigFloat& operator*=(const BigFloat& rhs);
        BigFloat operator/(const BigFloat& rhs) const;
        BigFloat& operator/=(const BigFloat& rhs);

        BigFloat operator+(const BigFloat& rhs) const;
        BigFloat& operator+=(const BigFloat& rhs);
        BigFloat operator-(const BigFloat& rhs) const;
        BigFloat& operator-=(const BigFloat& rhs);

        friend BigFloat operator/(const float lhs, const BigFloat& rhs);

        friend bool operator== (const BigFloat& lhs, const BigFloat& rhs);
        friend bool operator!= (const BigFloat& lhs, const BigFloat& rhs);
        friend bool operator< (const BigFloat& lhs, const BigFloat& rhs);
        friend bool operator> (const BigFloat& lhs, const BigFloat& rhs);
    private:
        float m_Coefficient;
        int m_Exponent;
    };


    BigFloat operator*(const float lhs, const BigFloat& rhs);
    BigFloat operator/(const float lhs, const BigFloat& rhs);

    bool operator== (const BigFloat& lhs, const BigFloat& rhs);
    bool operator!= (const BigFloat& lhs, const BigFloat& rhs);

    std::ostream& operator<<(std::ostream& ostream, const BigFloat& bf);

}
