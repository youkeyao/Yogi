#pragma once

namespace Yogi
{

template <typename T>
class Handle
{
    template <typename U>
    friend class Handle;

public:
    struct ControlBlock
    {
        T*                    Ptr;
        std::function<void()> CallBackFunc;
        int                   Count;
    };

public:
    Handle(std::nullptr_t) noexcept : m_cb(nullptr) {}
    Handle(const Handle&)                  = delete;
    Handle& operator=(const Handle& other) = delete;
    Handle(Handle&& other) : m_cb(other.m_cb) { other.m_cb = nullptr; }
    Handle& operator=(Handle&& other)
    {
        if (this != &other)
        {
            Cleanup();
            m_cb       = reinterpret_cast<ControlBlock*>(other.m_cb);
            other.m_cb = nullptr;
        }
        return *this;
    }
    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    Handle(Handle<U>&& other) noexcept : m_cb(reinterpret_cast<ControlBlock*>(other.m_cb))
    {
        other.m_cb = nullptr;
    }
    ~Handle() { Cleanup(); }

    inline T*            Get() const { return m_cb->Ptr; }
    inline ControlBlock* GetCB() const { return m_cb; }

    inline T*       operator->() { return m_cb->Ptr; }
    inline const T* operator->() const { return m_cb->Ptr; }

    inline T&       operator*() { return *(m_cb->Ptr); }
    inline const T& operator*() const { return *(m_cb->Ptr); }

    inline operator bool() const { return m_cb != nullptr; }

    inline int  GetRefCount() const { return m_cb->Count; }
    inline void SetSubCallBack(std::function<void()> callBackFunc) { m_cb->CallBackFunc = callBackFunc; }

    void Cleanup()
    {
        if (m_cb)
        {
            if (m_cb->Ptr)
                delete m_cb->Ptr;
            m_cb->Ptr          = nullptr;
            m_cb->CallBackFunc = nullptr;
            if (--m_cb->Count == 0)
            {
                delete m_cb;
            }
        }
    }

    template <typename... Args>
    static Handle<T> Create(Args&&... args)
    {
        return Handle(new T(std::forward<Args>(args)...));
    }

private:
    explicit Handle(T* p) : m_cb(new ControlBlock{ p, nullptr, 1 }) {}

private:
    ControlBlock* m_cb;
};

} // namespace Yogi
