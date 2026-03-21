#pragma once

namespace Yogi
{

template <typename T>
class Owner
{
    template <typename U>
    friend class Owner;

public:
    class ControlBlock
    {
    public:
        ControlBlock(T* p) : m_ptr(p), m_count(1) {}
        virtual ~ControlBlock() = default;

        int GetRefCount() const { return m_count; }
        T*  Ptr() const { return m_ptr; }

        virtual void AddRef() { ++m_count; }
        virtual void SubRef()
        {
            if (--m_count == 0)
                delete this;
        }
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
    Owner() noexcept : m_cb(nullptr) {}
    Owner(std::nullptr_t) noexcept : m_cb(nullptr) {}
    Owner(const Owner&)                  = delete;
    Owner& operator=(const Owner& other) = delete;
    Owner(Owner&& other) noexcept : m_cb(other.m_cb) { other.m_cb = nullptr; }
    Owner& operator=(Owner&& other) noexcept
    {
        if (this != &other)
        {
            Cleanup();
            m_cb       = other.m_cb;
            other.m_cb = nullptr;
        }
        return *this;
    }
    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    Owner(Owner<U>&& other) noexcept : m_cb(reinterpret_cast<ControlBlock*>(other.m_cb))
    {
        other.m_cb = nullptr;
    }
    ~Owner() { Cleanup(); }

    T*            Get() const { return m_cb ? m_cb->Ptr() : nullptr; }
    ControlBlock* GetCB() const { return m_cb; }
    void          SetCB(ControlBlock* cb) { m_cb = cb; }

    T*       operator->() { return Get(); }
    const T* operator->() const { return Get(); }

    T&       operator*() { return *Get(); }
    const T& operator*() const { return *Get(); }

    operator bool() const { return Get() != nullptr; }

    int GetRefCount() const { return m_cb ? m_cb->GetRefCount() : 0; }

    template <typename... Args>
    static Owner<T> Create(Args&&... args)
    {
        return Owner(new T(std::forward<Args>(args)...));
    }

private:
    explicit Owner(T* p) : m_cb(new ControlBlock(p)) {}

    void Cleanup()
    {
        if (!m_cb)
            return;

        m_cb->Release();
        m_cb->SubRef();
        m_cb = nullptr;
    }

private:
    ControlBlock* m_cb;
};

} // namespace Yogi
