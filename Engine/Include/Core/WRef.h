#pragma once

#include "Core/Owner.h"

namespace Yogi
{

template <typename T>
class WRef
{
    template <typename U>
    friend class WRef;
    friend struct std::hash<WRef<T>>;

public:
    WRef() noexcept = default;
    WRef(std::nullptr_t) noexcept {}
    WRef(const WRef& other) noexcept : m_cb(other.m_cb)
    {
        if (m_cb)
            m_cb->AddRef();
    }
    WRef& operator=(const WRef& other) noexcept
    {
        if (this != &other)
        {
            Release();
            m_cb = other.m_cb;
            if (m_cb)
                m_cb->AddRef();
        }
        return *this;
    }

    WRef(WRef&& other) noexcept : m_cb(std::exchange(other.m_cb, nullptr)) {}
    WRef& operator=(WRef&& other) noexcept
    {
        if (this != &other)
        {
            Release();
            m_cb = std::exchange(other.m_cb, nullptr);
        }
        return *this;
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    WRef(const WRef<U>& other) noexcept : m_cb(other.m_cb)
    {
        if (m_cb)
            m_cb->AddRef();
    }
    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    WRef(WRef<U>&& other) noexcept : m_cb(other.m_cb)
    {
        other.m_cb = nullptr;
    }

    template <typename U, typename = std::enable_if_t<std::is_base_of<T, U>::value || std::is_base_of<U, T>::value>>
    static WRef<T> Cast(const WRef<U>& other)
    {
        return WRef(other.m_cb);
    }

    ~WRef() { Release(); }

    inline T* Get() const noexcept { return m_cb ? m_cb->template PtrAs<T>() : nullptr; }

    inline T*       operator->() noexcept { return Get(); }
    inline const T* operator->() const noexcept { return Get(); }

    inline T&       operator*() noexcept { return *Get(); }
    inline const T& operator*() const noexcept { return *Get(); }

    inline bool operator==(const WRef& other) const noexcept { return m_cb == other.m_cb; }
    inline bool operator!=(const WRef& other) const noexcept { return m_cb != other.m_cb; }

    inline explicit operator bool() const noexcept { return Get() != nullptr; }
    inline bool     Expired() const noexcept { return Get() == nullptr; }

    inline int GetCount() const noexcept { return m_cb ? m_cb->GetCount() : 0; }

    template <typename U>
    WRef<U> DynamicCast() const noexcept
    {
        T* p = Get();
        if (p == nullptr)
            return WRef<U>{};
        if (dynamic_cast<U*>(p) == nullptr)
            return WRef<U>{};
        return WRef<U>(m_cb);
    }

    static WRef Create(const Owner<T>& owner) noexcept { return WRef(owner.m_cb); }

private:
    explicit WRef(ControlBlockBase* cb) noexcept : m_cb(cb)
    {
        if (m_cb)
            m_cb->AddRef();
    }
    void Release() noexcept
    {
        if (m_cb)
        {
            m_cb->SubRef();
            m_cb = nullptr;
        }
    }

private:
    ControlBlockBase* m_cb = nullptr;
};

} // namespace Yogi

namespace std
{
template <typename T>
struct hash<Yogi::WRef<T>>
{
    size_t operator()(const Yogi::WRef<T>& r) const noexcept { return std::hash<void*>()(r.m_cb); }
};
} // namespace std
