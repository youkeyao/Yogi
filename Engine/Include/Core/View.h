#pragma once

#include "Core/Owner.h"
#include "Core/WRef.h"

namespace Yogi
{

template <typename T>
class View
{
    template <typename U>
    friend class View;

public:
    View() noexcept = default;
    View(std::nullptr_t) noexcept {}

    static View<T> Create(const WRef<T>& ref) noexcept
    {
        View view;
        view.m_ptr = ref.Get();
        return view;
    }

    static View<T> Create(const Owner<T>& owner) noexcept
    {
        View view;
        view.m_ptr = owner.Get();
        return view;
    }

    static View<T> Create(const T* ptr) noexcept
    {
        View view;
        view.m_ptr = ptr;
        return view;
    }

    template <typename U, typename = std::enable_if_t<std::is_base_of<T, U>::value || std::is_base_of<U, T>::value>>
    static View<T> Cast(const View<U>& other) noexcept
    {
        View view;
        view.m_ptr = static_cast<const T*>(other.Get());
        return view;
    }

    inline const T* Get() const noexcept { return m_ptr; }

    inline const T* operator->() const noexcept { return Get(); }

    inline const T& operator*() const noexcept { return *Get(); }

    inline bool operator==(const View& other) const noexcept { return m_ptr == other.m_ptr; }

    inline          operator const T*() const noexcept { return Get(); }
    inline explicit operator bool() const noexcept { return Get() != nullptr; }

private:
    const T* m_ptr = nullptr;
};

} // namespace Yogi

namespace std
{
template <typename T>
struct hash<Yogi::View<T>>
{
    size_t operator()(const Yogi::View<T>& v) const noexcept
    {
        return std::hash<const void*>()(static_cast<const void*>(v.Get()));
    }
};
} // namespace std
