#define GLFW_INCLUDE_NONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
// #define YG_PROFILE

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#ifdef __GNUG__
#include <cxxabi.h>
#endif

#include "runtime/core/log.h"
#include "runtime/debug/instrumentor.h"

namespace Yogi {
template <typename T> using Scope = std::unique_ptr<T>;
template <typename T, typename... Args> constexpr Scope<T> CreateScope(Args &&...args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T> using Ref = std::shared_ptr<T>;
template <typename T, typename... Args> constexpr Ref<T> CreateRef(Args &&...args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename Type> auto get_type_name()
{
    int status = -1;
#ifdef __GNUG__
    char *demangled = abi::__cxa_demangle(typeid(Type).name(), nullptr, nullptr, &status);
    if (status == 0) {
        std::string className = demangled;
        free(demangled);
        return className;
    } else {
        free(demangled);
        return std::string(typeid(Type).name());
    }
#elif defined(_MSC_VER)
    std::string name = typeid(Type).name();
    size_t      pos = name.find(' ');
    if (pos != std::string::npos) {
        return name.substr(pos + 1);
    }
    return name;
#endif
}
}  // namespace Yogi