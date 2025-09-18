#pragma once

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
#include <any>
#include <typeindex>
#ifdef __GNUG__
#    include <cxxabi.h>
#endif

#ifdef YG_SHARED_LIB
#    ifdef YG_EXPORT
#        define YG_API __declspec(dllexport)
#    else
#        define YG_API __declspec(dllimport)
#    endif
#else
#    define YG_API
#endif

#include "Core/Log.h"
#include "Core/Handle.h"
#include "Core/Ref.h"
#include "Debug/Instrumentor.h"

namespace Yogi
{

#define YG_BIND_FN(x, ...) std::bind(&x, this, std::placeholders::_1, ##__VA_ARGS__)

} // namespace Yogi