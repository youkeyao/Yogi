#pragma once

namespace Yogi
{

template <typename T>
class WRef;

class ControlBlockBase
{
    template <typename T>
    friend class Owner;

public:
    virtual ~ControlBlockBase() = default;

    void AddRef() noexcept { ++m_count; }
    void SubRef() noexcept
    {
        if (--m_count == 0)
            delete this;
    }

    void Release() noexcept
    {
        if (!m_alive)
            return;

        DestroyObject();
        m_alive = false;
    }

    template <typename T>
    T* PtrAs() const noexcept
    {
        return m_alive ? static_cast<T*>(RawObjectPtr()) : nullptr;
    }

protected:
    virtual void* RawObjectPtr() const = 0;
    virtual void  DestroyObject()      = 0;

private:
    int  m_count = 1;
    bool m_alive = true;
};

template <typename U>
class InplaceControlBlock final : public ControlBlockBase
{
public:
    template <typename... Args>
    explicit InplaceControlBlock(std::in_place_t, Args&&... args)
    {
        ::new (static_cast<void*>(m_storage)) U(std::forward<Args>(args)...);
    }

protected:
    void* RawObjectPtr() const override { return const_cast<U*>(std::launder(reinterpret_cast<const U*>(m_storage))); }
    void  DestroyObject() override { std::launder(reinterpret_cast<U*>(m_storage))->~U(); }

private:
    alignas(U) std::byte m_storage[sizeof(U)];
};

template <typename T>
class Owner
{
    template <typename U>
    friend class Owner;
    friend class WRef<T>;

public:
    Owner() noexcept = default;
    Owner(std::nullptr_t) noexcept {}
    Owner(const Owner&)                  = delete;
    Owner& operator=(const Owner& other) = delete;
    Owner(Owner&& other) noexcept : m_cb(std::exchange(other.m_cb, nullptr)) {}
    Owner& operator=(Owner&& other) noexcept
    {
        if (this != &other)
        {
            Reset();
            m_cb = std::exchange(other.m_cb, nullptr);
        }
        return *this;
    }
    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    Owner(Owner<U>&& other) noexcept : m_cb(std::exchange(other.m_cb, nullptr))
    {}
    ~Owner() { Reset(); }

    T* Get() const noexcept { return m_cb ? m_cb->template PtrAs<T>() : nullptr; }

    T*       operator->() noexcept { return Get(); }
    const T* operator->() const noexcept { return Get(); }

    T&       operator*() noexcept { return *Get(); }
    const T& operator*() const noexcept { return *Get(); }

    explicit operator bool() const noexcept { return Get() != nullptr; }

    void Reset() noexcept
    {
        if (!m_cb)
            return;

        m_cb->Release();
        m_cb->SubRef();
        m_cb = nullptr;
    }

    template <typename... Args>
    static Owner<T> Create(Args&&... args)
    {
        return Owner(new InplaceControlBlock<T>(std::in_place, std::forward<Args>(args)...));
    }

private:
    explicit Owner(ControlBlockBase* cb) noexcept : m_cb(cb) {}

private:
    ControlBlockBase* m_cb = nullptr;
};

} // namespace Yogi
