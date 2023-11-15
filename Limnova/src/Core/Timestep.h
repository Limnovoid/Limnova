#pragma once


namespace Limnova
{

    class Timestep
    {
        float m_Time;
    public:
        Timestep(float time = 0.f) : m_Time(time) {}

        float GetSeconds() const { return m_Time; }
        float GetMilliseconds() const { return 1000.f * m_Time; }
    public:
        static constexpr float kDefaultTimestep = 1.f / 60.f;
    public:
        operator float() const { return m_Time; }
    };

}
