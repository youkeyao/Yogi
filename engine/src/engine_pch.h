#pragma once

#define GLFW_INCLUDE_NONE

#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <functional>
#include <vector>
#include <array>
#include <algorithm>
#include <utility>

#include "core/log.h"

#define BIT(x) (1 << x)
#define HZ_BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

namespace hazel {
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
}