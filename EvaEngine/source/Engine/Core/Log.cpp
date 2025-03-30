#include "pch.h"

#include "Log.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace Engine
{

	std::shared_ptr<spdlog::logger> Log::m_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::m_ClientLogger;

	void Log::Init()
	{
		//************************************************
		// https://github.com/gabime/spdlog
		//  %^ : Start color range(for colored terminal output).
		//	[% H:% M : % S] : Timestamp(hour, minute, second).
		//	[% n] : Logger's name.
		//	% l : Full log level(e.g., info, warning).
		//	[thread % t] : Thread ID.
		//	% v : The actual log message.
		//	% $ : End color range.*/
		//************************************************
		spdlog::set_pattern("%^ [%H:%M:%S] [%n] %l [thread %t] [%s] %v %$");

		m_CoreLogger = spdlog::stdout_color_mt("Engine");
		m_CoreLogger->set_level(spdlog::level::trace);

		m_ClientLogger = spdlog::stdout_color_mt("Client");
		m_ClientLogger->set_level(spdlog::level::trace);

	}
}