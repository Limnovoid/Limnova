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


    template <typename REP, typename PERIOD>
    class Stopwatch
    {
    public:
        typedef std::chrono::steady_clock Clock;
        typedef std::chrono::duration<REP, PERIOD> Duration;

        Stopwatch(std::function<void(REP)> callback = {}) : m_start(Clock::now()), m_callback(callback), m_running(true)
        {
        }

        ~Stopwatch()
        {
            REP stopTime = Time();
            if (m_callback && m_running)
                m_callback(stopTime);
        }

        /// <summary> Return tick count time since start (does not stop or restart the clock) </summary>
        REP Time()
        {
            return Duration(Clock::now() - m_start).count();
        }

        REP Stop()
        {
            REP time = Time();
            m_running = false;
            return time;
        }

        /// <summary> Reset start time to now </summary>
        void Restart()
        {
            m_running = true;
            m_start = Clock::now();
        }

        void SetCallback(std::function<void(REP)> callback)
        {
            m_callback = callback;
        }

    private:
        std::function<void(REP)> m_callback;
        std::chrono::time_point<Clock> m_start;
        bool m_running;
    };

}
