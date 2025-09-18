#pragma once

#include "boost/pfr.hpp"

namespace Yogi
{

template <typename T>
concept StdHashable = requires(T v) {
    { std::hash<T>{}(v) } -> std::convertible_to<size_t>;
};

constexpr void HashCombine(uint64_t& seed, uint64_t value)
{
    seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
}

template <typename T>
struct UniversalHash
{
    size_t operator()(const T& value) const
    {
        if constexpr (StdHashable<T>)
        {
            return std::hash<T>{}(value);
        }
        else
        {
            size_t h = 0;
            boost::pfr::for_each_field(value, [&](const auto& field) {
                h ^=
                    UniversalHash<std::decay_t<decltype(field)>>{}(field) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
            });
            return h;
        }
    }
};

template <typename... Args>
constexpr uint64_t HashArgs(Args&&... args)
{
    uint64_t seed = 0;
    (HashCombine(seed, UniversalHash<std::decay_t<Args>>{}(std::forward<Args>(args))), ...);
    return seed;
}

} // namespace Yogi

namespace std
{

template <typename T>
struct hash<std::vector<T>>
{
    size_t operator()(const std::vector<T>& vec) const noexcept
    {
        size_t seed = 0;
        for (auto& e : vec)
        {
            seed ^= Yogi::UniversalHash<T>{}(e) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

} // namespace std
