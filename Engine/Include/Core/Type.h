#pragma once

constexpr uint32_t fnv1a_32(const char* str, std::size_t n)
{
    uint32_t hash = 2166136261u; // offset basis
    for (std::size_t i = 0; i < n; ++i)
    {
        hash ^= static_cast<unsigned char>(str[i]);
        hash *= 16777619u; // FNV prime
    }
    return hash;
}

template <typename T>
constexpr std::string_view GetTypeName()
{
#if defined(__clang__) || defined(__GNUC__)
    constexpr std::string_view sig = __PRETTY_FUNCTION__;
    constexpr std::string_view key = "T = ";
    const auto                 pos = sig.find(key);
    if (pos == std::string_view::npos)
        return sig;
    const auto  start  = pos + key.size();
    std::size_t end    = sig.size();
    std::size_t endpos = sig.find(';', start);
    if (endpos != std::string_view::npos)
        end = endpos < end ? endpos : end;
    endpos = sig.find(']', start);
    if (endpos != std::string_view::npos)
        end = endpos < end ? endpos : end;
    return sig.substr(start, end - start);
#elif defined(_MSC_VER)
    constexpr std::string_view sig     = __FUNCSIG__;
    constexpr std::string_view key     = "GetTypeName<";
    const auto                 pos     = sig.find(key);
    if (pos == std::string_view::npos)
        return sig;
    auto start = pos + key.size();
    const auto blank = sig.find(' ', start);
    if (blank != std::string_view::npos)
        start = blank + 1;
    const auto end   = sig.find('>', start);
    if (end == std::string_view::npos)
        return sig;
    return sig.substr(start, end - start);
#else
#    error "Unsupported compiler"
#endif
}

template <typename T>
constexpr uint32_t GetTypeHash()
{
    constexpr auto name = GetTypeName<std::remove_cv_t<std::remove_reference_t<T>>>();
    return fnv1a_32(name.data(), name.size());
}