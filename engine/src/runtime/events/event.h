#pragma once

namespace Yogi {

    #define BIT(x) (1 << x)
    #define YG_BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

    enum class EventType
    {
        None = 0,
        WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
        AppTick, AppUpdate, AppRender,
        KeyPressed, KeyReleased,
        MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
    };
    
    enum EventCategory
    {
        None = 0,
        EventCategoryApplication = BIT(0),
        EventCategoryInput = BIT(1),
        EventCategoryKeyboard = BIT(2),
        EventCategoryMouse = BIT(3),
        EventCategoryMouseButton = BIT(4)
    };

    #define EVENT_CLASS_TYPE(type)                                                  \
        static EventType get_static_type() { return EventType::type; }              \
        virtual EventType get_event_type() const override { return get_static_type(); } \
        virtual const char* get_name() const override { return #type; }

    #define EVENT_CLASS_CATEGORY(category) \
        virtual int get_category_flags() const override { return category; }

    class Event
    {
        friend class EventDispatcher;

    public:
        virtual EventType get_event_type() const = 0;
        virtual const char* get_name() const = 0;
        virtual int get_category_flags() const = 0;
        virtual std::string to_string() const { return get_name(); }
        inline bool is_in_category(EventCategory category) { return get_category_flags() & category; }
        bool m_handled = false;
    };

    class EventDispatcher
    {
    public:
        EventDispatcher(Event& event) : m_event(event) {}

        template <typename T, typename... Args, typename F = std::function<bool(T&, Args...)>>
        bool dispatch(F func, Args&&... args)
        {
            if (m_event.get_event_type() == T::get_static_type() && !m_event.m_handled) {
                m_event.m_handled = func(*(T*) &m_event, std::forward<Args>(args)...);
                return true;
            }
            return false;
        }
    private:
        Event& m_event;
    };

    inline std::ostream& operator<<(std::ostream& os, const Event& e) { return os << e.to_string(); }

}