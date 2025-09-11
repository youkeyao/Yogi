#pragma once

namespace Yogi
{

class YG_API Timestep
{
public:
    Timestep(float time = 0.0f) : m_time(time) {}

    inline operator float() const { return m_time; }

    inline float GetSeconds() const { return m_time; }
    inline float GetMilliseconds() const { return m_time * 1000.0f; }

private:
    float m_time;
};

} // namespace Yogi