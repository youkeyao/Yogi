#pragma once

namespace hazel {

    class TimeStep
    {
    public:
        TimeStep(float time = 0.0f) : m_time(time) {}

        operator float() const { return m_time; }

        float get_seconds() { return m_time; }
        float get_miliseconds() { return m_time * 1000.0f; }
    private:
        float m_time;
    };

}