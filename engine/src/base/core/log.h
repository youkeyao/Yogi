#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/ostr.h>

namespace Yogi {

    class Log
    {
    public:
        static void init();
        inline static std::shared_ptr<spdlog::logger>& get_core_logger() { return ms_core_logger; }
        inline static std::shared_ptr<spdlog::logger>& get_client_logger() { return ms_client_logger; }
    private:
        static std::shared_ptr<spdlog::logger> ms_core_logger;
        static std::shared_ptr<spdlog::logger> ms_client_logger;
    };

}

// Core log macros
#define YG_CORE_TRACE(...)  ::Yogi::Log::get_core_logger()->trace(__VA_ARGS__)
#define YG_CORE_INFO(...)   ::Yogi::Log::get_core_logger()->info(__VA_ARGS__)
#define YG_CORE_WARN(...)   ::Yogi::Log::get_core_logger()->warn(__VA_ARGS__)
#define YG_CORE_ERROR(...)  ::Yogi::Log::get_core_logger()->error(__VA_ARGS__)
#define YG_CORE_FATAL(...)  ::Yogi::Log::get_core_logger()->critical(__VA_ARGS__)

// Client log macros
#define YG_TRACE(...)       ::Yogi::Log::get_client_logger()->trace(__VA_ARGS__)
#define YG_INFO(...)        ::Yogi::Log::get_client_logger()->info(__VA_ARGS__)
#define YG_WARN(...)        ::Yogi::Log::get_client_logger()->warn(__VA_ARGS__)
#define YG_ERROR(...)       ::Yogi::Log::get_client_logger()->error(__VA_ARGS__)
#define YG_FATAL(...)       ::Yogi::Log::get_client_logger()->critical(__VA_ARGS__)

// Assert
#define YG_CORE_ASSERT(x, ...) { if(!(x)) { YG_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); abort(); } }
#define YG_ASSERT(x, ...) { if(!(x)) { YG_ERROR("Assertion Failed: {0}", __VA_ARGS__); abort(); } }