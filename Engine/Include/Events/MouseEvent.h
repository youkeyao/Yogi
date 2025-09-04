#pragma once

#include "Events/Event.h"

namespace Yogi
{

class YG_API MouseMovedEvent : public Event
{
public:
    MouseMovedEvent(float x, float y, void* event) : m_x(x), m_y(y), Event(event) {}

    inline float GetX() const { return m_x; }
    inline float GetY() const { return m_y; }

    std::string ToString() const override
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

class YG_API MouseScrolledEvent : public Event
{
public:
    MouseScrolledEvent(float offsetX, float offsetY, void* event) : m_offsetX(offsetX), m_offsetY(offsetY), Event(event)
    {}

    inline float GetXOffset() const { return m_offsetX; }
    inline float GetYOffset() const { return m_offsetY; }

    std::string ToString() const override
    {
        auto ss = std::stringstream();
        ss << "MouseScrolledEvent: " << m_offsetX << ", " << m_offsetY;
        return ss.str();
    }

    EVENT_CLASS_TYPE(MouseScrolled)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

private:
    float m_offsetX;
    float m_offsetY;
};

class YG_API MouseButtonEvent : public Event
{
public:
    inline int get_mouse_button() const { return m_button; }

    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

protected:
    MouseButtonEvent(int button, void* event) : m_button(button), Event(event) {}
    int m_button;
};

class YG_API MouseButtonPressedEvent : public MouseButtonEvent
{
public:
    MouseButtonPressedEvent(int button, void* event) : MouseButtonEvent(button, event) {}

    std::string ToString() const override
    {
        auto ss = std::stringstream();
        ss << "MouseButtonPressedEvent: " << m_button;
        return ss.str();
    }

    EVENT_CLASS_TYPE(MouseButtonPressed)
};

class YG_API MouseButtonReleasedEvent : public MouseButtonEvent
{
public:
    MouseButtonReleasedEvent(int button, void* event) : MouseButtonEvent(button, event) {}

    std::string ToString() const override
    {
        auto ss = std::stringstream();
        ss << "MouseButtonReleasedEvent: " << m_button;
        return ss.str();
    }

    EVENT_CLASS_TYPE(MouseButtonReleased)
};

} // namespace Yogi