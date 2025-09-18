#pragma once

#include "Core/Handle.h"

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
            m_cb->Count++;
    }
    Ref& operator=(const Ref& other)
    {
        if (this != &other)
        {
            Release();
            m_cb = other.m_cb;
            if (m_cb)
                m_cb->Count++;
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
    Ref(const Ref<U>& other) : m_cb(static_cast<Handle<T>::ControlBlock*>(other.m_cb))
    {
        if (m_cb)
            m_cb->Count++;
    }
    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    Ref(Ref<U>&& other) noexcept : m_cb(reinterpret_cast<Handle<T>::ControlBlock*>(other.m_cb))
    {
        other.m_cb = nullptr;
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<T*, U*>::value>>
    static Ref<T> Cast(const Ref<U>& other)
    {
        return Ref(reinterpret_cast<Handle<T>::ControlBlock*>(other.m_cb));
    }

    ~Ref() { Release(); }

    inline T* Get() const { return m_cb->Ptr; }

    inline T*       operator->() { return m_cb->Ptr; }
    inline const T* operator->() const { return m_cb->Ptr; }

    inline T&       operator*() { return *m_cb->Ptr; }
    inline const T& operator*() const { return *m_cb->Ptr; }

    inline bool operator==(const Ref& other) const noexcept { return m_cb == other.m_cb; }
    inline bool operator==(const Handle<T>& handle) const noexcept { return m_cb == handle.GetCB(); }

    inline operator bool() const { return m_cb != nullptr; }

    static Ref Create(const Handle<T>& handle) { return Ref(handle.GetCB()); }

private:
    explicit Ref(Handle<T>::ControlBlock* cb) : m_cb(cb) { m_cb->Count++; }
    void Release()
    {
        if (m_cb)
        {
            if (m_cb->CallBackFunc)
                m_cb->CallBackFunc();
            if (--m_cb->Count == 0)
                delete m_cb;
        }
    }

private:
    Handle<T>::ControlBlock* m_cb;
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
