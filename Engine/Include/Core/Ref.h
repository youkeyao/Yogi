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
    Ref() : m_handle(nullptr) {}
    Ref(std::nullptr_t) noexcept : m_handle(nullptr) {}
    Ref(const Ref& other) : m_handle(other.m_handle)
    {
        if (m_handle)
            m_handle->AddRef();
    }
    Ref& operator=(const Ref& other)
    {
        if (this != &other)
        {
            Release();
            m_handle = other.m_handle;
            if (m_handle)
                m_handle->AddRef();
        }
        return *this;
    }

    Ref(Ref&& other) : m_handle(other.m_handle) { other.m_handle = nullptr; }
    Ref& operator=(Ref&& other)
    {
        if (this != &other)
        {
            Release();
            m_handle       = other.m_handle;
            other.m_handle = nullptr;
        }
        return *this;
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    Ref(const Ref<U>& other) : m_handle(static_cast<Handle<T>*>(other.m_handle))
    {
        if (m_handle)
            m_handle->AddRef();
    }
    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    Ref(Ref<U>&& other) noexcept : m_handle(reinterpret_cast<Handle<T>*>(other.m_handle))
    {
        other.m_handle = nullptr;
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<T*, U*>::value>>
    static Ref<T> Cast(const Ref<U>& other)
    {
        return Ref<T>::Create(*(reinterpret_cast<Handle<T>*>(other.m_handle)));
    }

    ~Ref() { Release(); }

    inline T* Get() const { return m_handle->Get(); }

    inline T*       operator->() { return m_handle->operator->(); }
    inline const T* operator->() const { return m_handle->operator->(); }

    inline T&       operator*() { return *m_handle; }
    inline const T& operator*() const { return *m_handle; }

    inline bool operator==(const Ref& other) const noexcept { return m_handle == other.m_handle; }

    inline operator bool() const { return m_handle != nullptr; }

    static Ref Create(const Handle<T>& handle) { return Ref(const_cast<Handle<T>*>(&handle)); }

private:
    explicit Ref(Handle<T>* handle) : m_handle(handle) { m_handle->AddRef(); }
    void Release()
    {
        if (m_handle)
            m_handle->SubRef();
    }

private:
    Handle<T>* m_handle;
};

} // namespace Yogi

namespace std
{
template <typename T>
struct hash<Yogi::Ref<T>>
{
    size_t operator()(const Yogi::Ref<T>& r) const noexcept { return std::hash<Yogi::Handle<T>*>()(r.m_handle); }
};
} // namespace std
