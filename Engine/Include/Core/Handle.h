#pragma once

namespace Yogi
{

template <typename T>
class Handle
{
    template <typename U>
    friend class Handle;

public:
    Handle(std::nullptr_t) noexcept : m_ptr(nullptr), m_count(0), m_callBack(nullptr) {}
    Handle(const Handle&)                  = delete;
    Handle& operator=(const Handle& other) = delete;
    Handle(Handle&& other) : m_ptr(other.m_ptr), m_count(other.m_count), m_callBack(other.m_callBack)
    {
        other.m_ptr      = nullptr;
        other.m_count    = 0;
        other.m_callBack = nullptr;
    }
    Handle& operator=(Handle&& other)
    {
        if (this != &other)
        {
            Cleanup();
            m_ptr            = other.m_ptr;
            m_count          = other.m_count;
            m_callBack       = other.m_callBack;
            other.m_ptr      = nullptr;
            other.m_count    = 0;
            other.m_callBack = nullptr;
        }
        return *this;
    }
    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    Handle(Handle<U>&& other) noexcept : m_ptr(other.m_ptr), m_count(other.m_count), m_callBack(other.m_callBack)
    {
        other.m_ptr      = nullptr;
        other.m_count    = 0;
        other.m_callBack = nullptr;
    }
    ~Handle() { Cleanup(); }

    inline T* Get() const { return m_ptr; }

    inline T*       operator->() { return m_ptr; }
    inline const T* operator->() const { return m_ptr; }

    inline T&       operator*() { return *m_ptr; }
    inline const T& operator*() const { return *m_ptr; }

    inline operator bool() const { return m_ptr != nullptr; }

    inline void AddRef() { ++m_count; }
    inline void SubRef()
    {
        --m_count;
        if (m_callBack)
            m_callBack();
    }
    inline int  GetRefCount() const { return m_count; }
    inline void SetSubCallBack(std::function<void()> callBack) { m_callBack = callBack; }

    void Cleanup()
    {
        if (m_ptr)
            delete m_ptr;
        m_ptr   = nullptr;
        m_count = 0;
    }

    template <typename... Args>
    static Handle<T> Create(Args&&... args)
    {
        return Handle(new T(std::forward<Args>(args)...));
    }

private:
    explicit Handle(T* p) : m_ptr(p), m_count(1), m_callBack(nullptr) {}

private:
    T*                    m_ptr;
    std::function<void()> m_callBack;
    int                   m_count;
};

} // namespace Yogi
