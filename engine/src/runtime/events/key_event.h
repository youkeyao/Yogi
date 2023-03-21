#pragma once

#include "runtime/events/event.h"

namespace Yogi {

    class KeyEvent : public Event
    {
    public:
        inline int get_key_code() const { return m_key_code; }

        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

    protected:
        KeyEvent(int key_code, void* event) : m_key_code(key_code), Event(event) {}
        int m_key_code = 0;
    };

    class KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(int key_code, int repeat_count, void* event) : KeyEvent(key_code, event), m_repeat_count(repeat_count) {}

        inline int get_repeat_count() const { return m_repeat_count; }

        std::string to_string() const override
        {
            std::stringstream ss;
            ss << "KeyPressedEvent: " << m_key_code << " (" << m_repeat_count << " repeats)";
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyPressed)

    protected:
        int m_repeat_count = 0;
    };

    class KeyReleasedEvent : public KeyEvent
    {
    public:
        KeyReleasedEvent(int key_code, void* event) : KeyEvent(key_code, event) {}

        std::string to_string() const override
        {
            std::stringstream ss;
            ss << "KeyReleasedEvent: " << m_key_code;
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyReleased)
    };

}