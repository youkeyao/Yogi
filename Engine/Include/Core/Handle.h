#pragma once

namespace Yogi
{

template <typename T>
class Handle
{
    template <typename U>
    friend class Handle;

public:
    class ControlBlock
    {
    public:
        ControlBlock(T* p, int count) : m_ptr(p), m_count(count) {}
        virtual ~ControlBlock() = default;

        int GetRefCount() const { return m_count; }
        T*  Ptr() const { return m_ptr; }

        virtual void AddRef() { ++m_count; }
        virtual void SubRef() { --m_count; }
        virtual void Release()
        {
            if (m_ptr)
                delete m_ptr;
            m_ptr = nullptr;
        }

    protected:
        T*  m_ptr;
        int m_count;
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

    inline T*            Get() const { return m_cb->Ptr(); }
    inline ControlBlock* GetCB() const { return m_cb; }
    inline void          SetCB(ControlBlock* cb) { m_cb = cb; }

    inline T*       operator->() { return m_cb->Ptr(); }
    inline const T* operator->() const { return m_cb->Ptr(); }

    inline T&       operator*() { return *(m_cb->Ptr()); }
    inline const T& operator*() const { return *(m_cb->Ptr()); }

    inline operator bool() const { return m_cb != nullptr; }

    inline int GetRefCount() const { return m_cb->GetRefCount(); }

    template <typename... Args>
    static Handle<T> Create(Args&&... args)
    {
        return Handle(new T(std::forward<Args>(args)...));
    }

private:
    explicit Handle(T* p) : m_cb(new ControlBlock(p, 1)) {}
    void Cleanup()
    {
        if (m_cb)
        {
            m_cb->Release();
            m_cb->SubRef();
            if (m_cb->GetRefCount() == 0)
            {
                delete m_cb;
            }
        }
    }

private:
    ControlBlock* m_cb;
};

} // namespace Yogi
