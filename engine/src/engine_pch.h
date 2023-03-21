#define GLFW_INCLUDE_NONE
#define YG_WINDOW_GLFW 1
#define YG_WINDOW_SDL 2
#define YG_RENDERER_OPENGL 1
#define YG_RENDERER_VULKAN 2
// #define YG_PROFILE

#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <fstream>
#include <sstream>
#include <functional>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <array>
#include <algorithm>
#include <utility>

#include "runtime/core/log.h"
#include "runtime/debug/instrumentor.h"

namespace Yogi {
    template<typename T>
    using Scope = std::unique_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Scope<T> CreateScope(Args&& ... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    using Ref = std::shared_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Ref<T> CreateRef(Args&& ... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template <typename Type>
    auto get_type_name()
    {
        std::string pretty_function{__PRETTY_FUNCTION__};
        auto first = pretty_function.find_first_not_of(' ', pretty_function.find_first_of('=') + 1);
        auto value = pretty_function.substr(first, pretty_function.find_last_of(']') - first);
        return value;
    }
}