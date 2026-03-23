#pragma once

#include <type_traits>

namespace Yogi
{

template <typename Enum>
struct EnableEnumFlags : std::false_type
{};

template <typename Enum>
inline constexpr bool EnableEnumFlagsV = EnableEnumFlags<Enum>::value;

template <typename Enum>
class EnumFlags
{
public:
    using Underlying = std::underlying_type_t<Enum>;

    constexpr EnumFlags() noexcept = default;
    constexpr EnumFlags(Enum bit) noexcept : m_mask(static_cast<Underlying>(bit)) {}
    constexpr explicit EnumFlags(Underlying mask) noexcept : m_mask(mask) {}

    constexpr EnumFlags& operator|=(EnumFlags rhs) noexcept
    {
        m_mask |= rhs.m_mask;
        return *this;
    }

    constexpr EnumFlags& operator&=(EnumFlags rhs) noexcept
    {
        m_mask &= rhs.m_mask;
        return *this;
    }

    constexpr EnumFlags& operator^=(EnumFlags rhs) noexcept
    {
        m_mask ^= rhs.m_mask;
        return *this;
    }

    constexpr EnumFlags& operator|=(Enum rhs) noexcept { return (*this |= EnumFlags(rhs)); }

    constexpr EnumFlags& operator&=(Enum rhs) noexcept { return (*this &= EnumFlags(rhs)); }

    constexpr EnumFlags& operator^=(Enum rhs) noexcept { return (*this ^= EnumFlags(rhs)); }

    constexpr explicit operator bool() const noexcept { return m_mask != 0; }
    constexpr          operator Enum() const noexcept { return static_cast<Enum>(m_mask); }
    constexpr explicit operator Underlying() const noexcept { return m_mask; }

    constexpr Underlying Value() const noexcept { return m_mask; }

    friend constexpr EnumFlags operator|(EnumFlags lhs, EnumFlags rhs) noexcept
    {
        lhs |= rhs;
        return lhs;
    }

    friend constexpr EnumFlags operator&(EnumFlags lhs, EnumFlags rhs) noexcept
    {
        lhs &= rhs;
        return lhs;
    }

    friend constexpr EnumFlags operator^(EnumFlags lhs, EnumFlags rhs) noexcept
    {
        lhs ^= rhs;
        return lhs;
    }

    friend constexpr EnumFlags operator~(EnumFlags value) noexcept
    {
        return EnumFlags(static_cast<Underlying>(~value.m_mask));
    }

    friend constexpr EnumFlags operator|(EnumFlags lhs, Enum rhs) noexcept
    {
        lhs |= rhs;
        return lhs;
    }

    friend constexpr EnumFlags operator&(EnumFlags lhs, Enum rhs) noexcept
    {
        lhs &= rhs;
        return lhs;
    }

    friend constexpr EnumFlags operator^(EnumFlags lhs, Enum rhs) noexcept
    {
        lhs ^= rhs;
        return lhs;
    }

    friend constexpr EnumFlags operator|(Enum lhs, EnumFlags rhs) noexcept
    {
        rhs |= lhs;
        return rhs;
    }

    friend constexpr EnumFlags operator&(Enum lhs, EnumFlags rhs) noexcept
    {
        rhs &= lhs;
        return rhs;
    }

    friend constexpr EnumFlags operator^(Enum lhs, EnumFlags rhs) noexcept
    {
        rhs ^= lhs;
        return rhs;
    }

    friend constexpr bool operator==(EnumFlags lhs, EnumFlags rhs) noexcept { return lhs.m_mask == rhs.m_mask; }
    friend constexpr bool operator!=(EnumFlags lhs, EnumFlags rhs) noexcept { return !(lhs == rhs); }

    friend constexpr bool operator==(EnumFlags lhs, Enum rhs) noexcept
    {
        return lhs.m_mask == static_cast<Underlying>(rhs);
    }

    friend constexpr bool operator!=(EnumFlags lhs, Enum rhs) noexcept { return !(lhs == rhs); }

    friend constexpr bool operator==(Enum lhs, EnumFlags rhs) noexcept { return rhs == lhs; }
    friend constexpr bool operator!=(Enum lhs, EnumFlags rhs) noexcept { return !(lhs == rhs); }

private:
    Underlying m_mask = 0;
};

template <typename Enum, typename = std::enable_if_t<EnableEnumFlagsV<Enum>>>
constexpr EnumFlags<Enum> operator|(Enum a, Enum b) noexcept
{
    return EnumFlags<Enum>(a) | b;
}

template <typename Enum, typename = std::enable_if_t<EnableEnumFlagsV<Enum>>>
constexpr EnumFlags<Enum> operator&(Enum a, Enum b) noexcept
{
    return EnumFlags<Enum>(a) & b;
}

template <typename Enum, typename = std::enable_if_t<EnableEnumFlagsV<Enum>>>
constexpr EnumFlags<Enum> operator^(Enum a, Enum b) noexcept
{
    return EnumFlags<Enum>(a) ^ b;
}

template <typename Enum, typename = std::enable_if_t<EnableEnumFlagsV<Enum>>>
constexpr EnumFlags<Enum> operator~(Enum a) noexcept
{
    return ~EnumFlags<Enum>(a);
}

template <typename Enum, typename = std::enable_if_t<EnableEnumFlagsV<Enum>>>
constexpr Enum& operator|=(Enum& a, Enum b) noexcept
{
    a = static_cast<Enum>(EnumFlags<Enum>(a) | b);
    return a;
}

template <typename Enum, typename = std::enable_if_t<EnableEnumFlagsV<Enum>>>
constexpr Enum& operator&=(Enum& a, Enum b) noexcept
{
    a = static_cast<Enum>(EnumFlags<Enum>(a) & b);
    return a;
}

template <typename Enum, typename = std::enable_if_t<EnableEnumFlagsV<Enum>>>
constexpr Enum& operator^=(Enum& a, Enum b) noexcept
{
    a = static_cast<Enum>(EnumFlags<Enum>(a) ^ b);
    return a;
}

#define YG_ENABLE_ENUM_FLAGS(EnumType)                \
    template <>                                       \
    struct EnableEnumFlags<EnumType> : std::true_type \
    {}

} // namespace Yogi
