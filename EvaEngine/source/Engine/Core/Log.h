#pragma once
#include "Core.h"



// This ignores all warnings raised inside External headers
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

// core log macros
#define EE_CORE_TRACE(...)    ::Engine::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define EE_CORE_INFO(...)     ::Engine::Log::GetCoreLogger()->info(__VA_ARGS__)
#define EE_CORE_WARN(...)     ::Engine::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define EE_CORE_ERROR(...)    ::Engine::Log::GetCoreLogger()->error(__VA_ARGS__)
#define EE_CORE_CRITICAL(...) ::Engine::Log::GetCoreLogger()->critical(__VA_ARGS__)

// client log macros
#define EE_TRACE(...)    ::Engine::Log::GetClientLogger()->trace(__VA_ARGS__)
#define EE_INFO(...)     ::Engine::Log::GetClientLogger()->info(__VA_ARGS__)
#define EE_WARN(...)     ::Engine::Log::GetClientLogger()->warn(__VA_ARGS__)
#define EE_ERROR(...)    ::Engine::Log::GetClientLogger()->error(__VA_ARGS__)
#define EE_CRITICAL(...) ::Engine::Log::GetClientLogger()->critical(__VA_ARGS__)

