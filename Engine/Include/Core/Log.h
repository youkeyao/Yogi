#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/ostr.h>

namespace Yogi
{

class YG_API Log
{
public:
    static void Init();

    inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_coreLogger; }
    inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_clientLogger; }

private:
    static std::shared_ptr<spdlog::logger> s_coreLogger;
    static std::shared_ptr<spdlog::logger> s_clientLogger;
};

} // namespace Yogi

// Core log macros
#define YG_CORE_TRACE(...) ::Yogi::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define YG_CORE_INFO(...)  ::Yogi::Log::GetCoreLogger()->info(__VA_ARGS__)
#define YG_CORE_WARN(...)  ::Yogi::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define YG_CORE_ERROR(...) ::Yogi::Log::GetCoreLogger()->error(__VA_ARGS__)
#define YG_CORE_FATAL(...) ::Yogi::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define YG_TRACE(...) ::Yogi::Log::GetClientLogger()->trace(__VA_ARGS__)
#define YG_INFO(...)  ::Yogi::Log::GetClientLogger()->info(__VA_ARGS__)
#define YG_WARN(...)  ::Yogi::Log::GetClientLogger()->warn(__VA_ARGS__)
#define YG_ERROR(...) ::Yogi::Log::GetClientLogger()->error(__VA_ARGS__)
#define YG_FATAL(...) ::Yogi::Log::GetClientLogger()->critical(__VA_ARGS__)

// Assert
#define YG_CORE_ASSERT(x, ...)                                   \
    {                                                            \
        if (!(x))                                                \
        {                                                        \
            YG_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); \
            abort();                                             \
        }                                                        \
    }
#define YG_ASSERT(x, ...)                                   \
    {                                                       \
        if (!(x))                                           \
        {                                                   \
            YG_ERROR("Assertion Failed: {0}", __VA_ARGS__); \
            abort();                                        \
        }                                                   \
    }