#pragma once

#include "runtime/events/event.h"

namespace Yogi {

    class MouseMovedEvent : public Event
    {
    public:
        MouseMovedEvent(float x, float y, void* event) : m_x(x), m_y(y), Event(event) {}

        inline float get_x() const { return m_x; }
        inline float get_y() const { return m_y; }

        std::string to_string() const override
        {
            auto ss = std::stringstream();
            ss << "MouseMovedEvent: " << m_x << ", " << m_y;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseMoved)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

    private:
        float m_x;
        float m_y;
    };

    class MouseScrolledEvent : public Event
    {
    public:
        MouseScrolledEvent(float x_offset, float y_offset, void* event) : m_x_offset(x_offset), m_y_offset(y_offset), Event(event) {}

        inline float get_x_offset() const { return m_x_offset; }
        inline float get_y_offset() const { return m_y_offset; }

        std::string to_string() const override
        {
            auto ss = std::stringstream();
            ss << "MouseScrolledEvent: " << m_x_offset << ", " << m_y_offset;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseScrolled)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

    private:
        float m_x_offset;
        float m_y_offset;
    };

    class MouseButtonEvent : public Event
    {
    public:
        inline int get_mouse_button() const { return m_button; }

        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

    protected:
        MouseButtonEvent(int button, void* event) : m_button(button), Event(event) {}
        int m_button;
    };

    class MouseButtonPressedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonPressedEvent(int button, void* event) : MouseButtonEvent(button, event) {}

        std::string to_string() const override
        {
            auto ss = std::stringstream();
            ss << "MouseButtonPressedEvent: " << m_button;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseButtonPressed)
    };

    class MouseButtonReleasedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonReleasedEvent(int button, void* event) : MouseButtonEvent(button, event) {}

        std::string to_string() const override
        {
            auto ss = std::stringstream();
            ss << "MouseButtonReleasedEvent: " << m_button;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseButtonReleased)
    };

}