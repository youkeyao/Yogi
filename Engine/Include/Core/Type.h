#pragma once

namespace Yogi
{

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
#if defined(__clang__)
    constexpr std::string_view p      = __PRETTY_FUNCTION__; // e.g. "std::string_view Yogi::GetTypeName() [T = int]"
    constexpr std::string_view prefix = "std::string_view Yogi::GetTypeName() [T = ";
    constexpr std::string_view suffix = "]";
#elif defined(__GNUC__)
    constexpr std::string_view p =
        __PRETTY_FUNCTION__; // e.g. "constexpr std::string_view Yogi::GetTypeName() [with T = int]"
    constexpr std::string_view prefix = "constexpr std::string_view Yogi::GetTypeName() [with T = ";
    constexpr std::string_view suffix = "]";
#elif defined(_MSC_VER)
    constexpr std::string_view p =
        __FUNCSIG__; // e.g. "class std::basic_string_view<char,struct std::char_traits<char> > __cdecl GetTypeName<int>(void)"
    constexpr std::string_view prefix =
        "class std::basic_string_view<char,struct std::char_traits<char> > __cdecl GetTypeName<";
    constexpr std::string_view suffix = ">(void)";
#else
#    error "Unsupported compiler"
#endif

    const auto start = p.find(prefix) + prefix.size();
    const auto end   = p.rfind(suffix);
    return p.substr(start, end - start);
}

template <typename T>
constexpr uint32_t GetTypeHash()
{
    constexpr auto name = GetTypeName<T>();
    return fnv1a_32(name.data(), name.size());
}

} // namespace Yogi
