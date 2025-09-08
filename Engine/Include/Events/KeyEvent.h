#pragma once

#include "Events/Event.h"

namespace Yogi
{

class YG_API KeyEvent : public Event
{
public:
    inline int GetKeyCode() const { return m_keyCode; }

    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

protected:
    KeyEvent(int keyCode, void* event) : m_keyCode(keyCode), Event(event) {}
    int m_keyCode = 0;
};

class YG_API KeyPressedEvent : public KeyEvent
{
public:
    KeyPressedEvent(int key_code, int repeat_count, void* event) :
        KeyEvent(key_code, event),
        m_repeatCount(repeat_count)
    {}

    inline int GetRepeatCount() const { return m_repeatCount; }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyPressedEvent: " << m_keyCode << " (" << m_repeatCount << " repeats)";
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyPressed)

protected:
    int m_repeatCount = 0;
};

class YG_API KeyReleasedEvent : public KeyEvent
{
public:
    KeyReleasedEvent(int keyCode, void* event) : KeyEvent(keyCode, event) {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyReleasedEvent: " << m_keyCode;
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyReleased)
};

} // namespace Yogi