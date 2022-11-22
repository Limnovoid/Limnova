#pragma once


namespace Limnova
{

    class Timestep
    {
    public:
        Timestep(float time = 0.f)
            : m_Time(time)
        {
        }

        float GetSeconds() const { return m_Time; }
        float GetMilliseconds() const { return 1000.f * m_Time; }
    public:
        operator float() const { return m_Time; }
    private:
        float m_Time;
    };

}