#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/ostr.h>

namespace hazel {

    class Log
    {
    public:
        static void init();
        inline static std::shared_ptr<spdlog::logger> get_core_logger() { return ms_core_logger; }
        inline static std::shared_ptr<spdlog::logger> get_client_logger() { return ms_client_logger; }
    private:
        static std::shared_ptr<spdlog::logger> ms_core_logger;
        static std::shared_ptr<spdlog::logger> ms_client_logger;
    };

}

// Core log macros
#define HZ_CORE_TRACE(...)  ::hazel::Log::get_core_logger()->trace(__VA_ARGS__)
#define HZ_CORE_INFO(...)   ::hazel::Log::get_core_logger()->info(__VA_ARGS__)
#define HZ_CORE_WARN(...)   ::hazel::Log::get_core_logger()->warn(__VA_ARGS__)
#define HZ_CORE_ERROR(...)  ::hazel::Log::get_core_logger()->error(__VA_ARGS__)
#define HZ_CORE_FATAL(...)  ::hazel::Log::get_core_logger()->critical(__VA_ARGS__)

// Client log macros
#define HZ_TRACE(...)       ::hazel::Log::get_client_logger()->trace(__VA_ARGS__)
#define HZ_INFO(...)        ::hazel::Log::get_client_logger()->info(__VA_ARGS__)
#define HZ_WARN(...)        ::hazel::Log::get_client_logger()->warn(__VA_ARGS__)
#define HZ_ERROR(...)       ::hazel::Log::get_client_logger()->error(__VA_ARGS__)
#define HZ_FATAL(...)       ::hazel::Log::get_client_logger()->critical(__VA_ARGS__)