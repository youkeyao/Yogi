#define GLFW_INCLUDE_NONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
// #define YG_PROFILE

#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <functional>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <array>
#include <algorithm>
#include <utility>
#include <optional>
#include <cxxabi.h>

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
        int status = -1;
        char* demangled = nullptr;
        demangled = abi::__cxa_demangle(typeid(Type).name(), nullptr, nullptr, &status);
        if (status == 0) {
            std::string className = demangled;
            free(demangled);
            return className;
        } else {
            free(demangled);
            return std::string(typeid(Type).name());
        }
    }
}