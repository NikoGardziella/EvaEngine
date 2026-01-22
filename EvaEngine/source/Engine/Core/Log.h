#pragma once

// Core.h no longer includes Log.h, so this is safe now
#include "Core.h"

#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

namespace Engine {
    class Log
    {
    public:
        static void Init();
        inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return m_CoreLogger; }
        inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return m_ClientLogger; }
    private:
        static std::shared_ptr<spdlog::logger> m_CoreLogger;
        static std::shared_ptr<spdlog::logger> m_ClientLogger;
    };
}

#define EE_FILE_NAME ([]() constexpr { \
    std::string_view sv(__FILE__); \
    auto pos = sv.rfind('\\'); \
    return pos == std::string_view::npos ? sv : sv.substr(pos + 1); \
}())

// Core log macros
#define EE_CORE_TRACE(msg, ...)    ::Engine::Log::GetCoreLogger()->trace("[{}] " msg, EE_FILE_NAME, __VA_ARGS__)
#define EE_CORE_INFO(msg, ...)     ::Engine::Log::GetCoreLogger()->info("[{}] " msg, EE_FILE_NAME, __VA_ARGS__)
#define EE_CORE_ERROR(msg, ...)    ::Engine::Log::GetCoreLogger()->error("[{}] " msg, EE_FILE_NAME, __VA_ARGS__)
#define EE_CORE_WARN(msg, ...)     ::Engine::Log::GetCoreLogger()->warn("[{}] " msg, EE_FILE_NAME, __VA_ARGS__)
#define EE_CORE_CRITICAL(msg, ...) ::Engine::Log::GetCoreLogger()->critical("[{}] " msg, EE_FILE_NAME, __VA_ARGS__)

// Client log macros
#define EE_TRACE(msg, ...)         ::Engine::Log::GetClientLogger()->trace("[{}] " msg, EE_FILE_NAME, __VA_ARGS__)
#define EE_INFO(msg, ...)          ::Engine::Log::GetClientLogger()->info("[{}] " msg, EE_FILE_NAME, __VA_ARGS__)
#define EE_WARN(msg, ...)          ::Engine::Log::GetClientLogger()->warn("[{}] " msg, EE_FILE_NAME, __VA_ARGS__)
#define EE_ERROR(msg, ...)         ::Engine::Log::GetClientLogger()->error("[{}] " msg, EE_FILE_NAME, __VA_ARGS__)
#define EE_CRITICAL(msg, ...)      ::Engine::Log::GetClientLogger()->critical("[{}] " msg, EE_FILE_NAME, __VA_ARGS__)