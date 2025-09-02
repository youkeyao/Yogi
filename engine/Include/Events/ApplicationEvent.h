#pragma once

#include "Events/Event.h"

namespace Yogi
{

class YG_API WindowResizeEvent : public Event
{
public:
    WindowResizeEvent(uint32_t width, uint32_t height, void* event) : m_width(width), m_height(height), Event(event) {}

    inline uint32_t GetWidth() const { return m_width; }
    inline uint32_t GetHeight() const { return m_height; }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "WindowResizeEvent: " << m_width << ", " << m_height;
        return ss.str();
    }

    EVENT_CLASS_TYPE(WindowResize)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)

private:
    uint32_t m_width = 0, m_height = 0;
};

class YG_API WindowCloseEvent : public Event
{
public:
    WindowCloseEvent(void* event) : Event(event) {}

    EVENT_CLASS_TYPE(WindowClose)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class YG_API WindowFocusEvent : public Event
{
public:
    WindowFocusEvent(void* event) : Event(event) {}

    EVENT_CLASS_TYPE(WindowFocus)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

} // namespace Yogi