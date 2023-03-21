#pragma once

#include "runtime/events/event.h"

namespace Yogi {

    class WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(uint32_t width, uint32_t height, void* event) : m_width(width), m_height(height), Event(event) {}

        inline uint32_t get_width() const { return m_width; }
        inline uint32_t get_height() const { return m_height; }

        std::string to_string() const override
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

    class WindowCloseEvent : public Event
    {
    public:
        WindowCloseEvent(void* event) : Event(event) {}

        EVENT_CLASS_TYPE(WindowClose)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

}