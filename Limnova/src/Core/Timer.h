#pragma once

#include <chrono>


namespace Limnova
{

    class Timer
    {
    public:
        Timer()
        {
            Reset();
        }

        void Reset()
        {
            m_Start = std::chrono::high_resolution_clock::now();
        }

        float ElapsedMillis()
        {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - m_Start).count()
                * 1e-6f;
        }

        float Elapsed()
        {
            return ElapsedMillis() * 1e-3f;
        }
    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
    };

}
