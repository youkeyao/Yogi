#pragma once

#include "Core/Owner.h"

namespace Yogi
{

template <typename T>
class Ref
{
    template <typename U>
    friend class Ref;
    friend struct std::hash<Ref<T>>;

public:
    Ref() : m_cb(nullptr) {}
    Ref(std::nullptr_t) noexcept : m_cb(nullptr) {}
    Ref(const Ref& other) : m_cb(other.m_cb)
    {
        if (m_cb)
            m_cb->AddRef();
    }
    Ref& operator=(const Ref& other)
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

    Ref(Ref&& other) : m_cb(other.m_cb) { other.m_cb = nullptr; }
    Ref& operator=(Ref&& other)
    {
        if (this != &other)
        {
            Release();
            m_cb       = other.m_cb;
            other.m_cb = nullptr;
        }
        return *this;
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    Ref(const Ref<U>& other) : m_cb(reinterpret_cast<typename Owner<T>::ControlBlock*>(other.m_cb))
    {
        if (m_cb)
            m_cb->AddRef();
    }
    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    Ref(Ref<U>&& other) noexcept : m_cb(reinterpret_cast<typename Owner<T>::ControlBlock*>(other.m_cb))
    {
        other.m_cb = nullptr;
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<T*, U*>::value>>
    static Ref<T> Cast(const Ref<U>& other)
    {
        return Ref(reinterpret_cast<typename Owner<T>::ControlBlock*>(other.m_cb));
    }

    ~Ref() { Release(); }

    inline T*                               Get() const { return m_cb ? m_cb->Ptr() : nullptr; }
    inline typename Owner<T>::ControlBlock* GetCB() const { return m_cb; }

    inline T*       operator->() { return Get(); }
    inline const T* operator->() const { return Get(); }

    inline T&       operator*() { return *Get(); }
    inline const T& operator*() const { return *Get(); }

    inline bool operator==(const Ref& other) const noexcept { return m_cb == other.m_cb; }
    inline bool operator==(const Owner<T>& owner) const noexcept { return m_cb == owner.GetCB(); }

    inline operator bool() const { return Get() != nullptr; }

    static Ref Create(const Owner<T>& owner) { return Ref(owner.GetCB()); }

private:
    explicit Ref(typename Owner<T>::ControlBlock* cb) : m_cb(cb)
    {
        if (m_cb)
            m_cb->AddRef();
    }
    void Release()
    {
        if (m_cb)
        {
            m_cb->SubRef();
            m_cb = nullptr;
        }
    }

private:
    typename Owner<T>::ControlBlock* m_cb;
};

} // namespace Yogi

namespace std
{
template <typename T>
struct hash<Yogi::Ref<T>>
{
    size_t operator()(const Yogi::Ref<T>& r) const noexcept { return std::hash<void*>()(r.m_cb); }
};
} // namespace std
