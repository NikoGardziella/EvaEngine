#pragma once

#include "Engine/Core/Log.h"
//#include "Engine/Core/Core.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <string>
#include <thread>
#include <mutex>
#include <sstream>
#include <filesystem>

namespace Engine {

	using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;

	struct ProfileResult
	{
		std::string Name;

		FloatingPointMicroseconds Start;
		std::chrono::microseconds ElapsedTime;
		std::thread::id ThreadID;
	};

	struct InstrumentationSession
	{
		std::string Name;
	};

	class Instrumentor
	{
	public:
		Instrumentor(const Instrumentor&) = delete;
		Instrumentor(Instrumentor&&) = delete;

		void BeginSession(const std::string& name, const std::string& filepath = "results.json")
		{
			std::lock_guard lock(m_Mutex);
			if (m_CurrentSession)
			{
				// If there is already a current session, then close it before beginning new one.
				if (Log::GetCoreLogger())
				{
					EE_CORE_ERROR("Instrumentor::BeginSession('{}') when session '{}' already open.", name, m_CurrentSession->Name);
				}
				InternalEndSession();
			}

			// Ensure that the trace folder exists
			std::filesystem::path traceDirectory = "trace";
			if (!std::filesystem::exists(traceDirectory))
			{
				std::filesystem::create_directory(traceDirectory);
			}

			// Construct the full file path within the trace folder
			std::filesystem::path fullFilePath = traceDirectory / filepath;

			m_OutputStream.open(fullFilePath);

			if (m_OutputStream.is_open())
			{
				m_CurrentSession = new InstrumentationSession({ name });
				WriteHeader();
			}
			else
			{
				if (Engine::Log::GetCoreLogger())
				{
					EE_CORE_ERROR("Instrumentor could not open results file '{}'.", fullFilePath.string());
				}
			}
		}


		void EndSession()
		{
			std::lock_guard lock(m_Mutex);
			InternalEndSession();
		}

		void WriteProfile(const ProfileResult& result)
		{
			std::stringstream json;

			json << std::setprecision(3) << std::fixed;
			json << ",{";
			json << "\"cat\":\"function\",";
			json << "\"dur\":" << (result.ElapsedTime.count()) << ',';
			json << "\"name\":\"" << result.Name << "\",";
			json << "\"ph\":\"X\",";
			json << "\"pid\":0,";
			json << "\"tid\":" << result.ThreadID << ",";
			json << "\"ts\":" << result.Start.count();
			json << "}";

			std::lock_guard lock(m_Mutex);
			if (m_CurrentSession)
			{
				m_OutputStream << json.str();
				m_OutputStream.flush();
			}

			
			/*
			if (result.ElapsedTime > std::chrono::microseconds(10000)) // 10ms
			{
				double elapsedMs = std::chrono::duration<double, std::milli>(result.ElapsedTime).count();
				EE_CORE_WARN("Performance warning! Function: {0} exceeds 10ms. Elapsed time: {1:.2f} ms", result.Name, elapsedMs);
			}
			if (result.ElapsedTime > std::chrono::microseconds(16670)) // Exceeds 16.67ms (1 frame at 60 FPS)
			{
				double elapsedMs = std::chrono::duration<double, std::milli>(result.ElapsedTime).count();
				EE_CORE_CRITICAL("Performance warning! Function: {0} exceeds 16.67ms. Elapsed time: {1:.2f} ms", result.Name, elapsedMs);
			}
			*/


		}

		static Instrumentor& Get()
		{
			static Instrumentor instance;
			return instance;
		}
	private:
		Instrumentor()
			: m_CurrentSession(nullptr)
		{
		}

		~Instrumentor()
		{
			EndSession();
		}

		void WriteHeader()
		{
			m_OutputStream << "{\"otherData\": {},\"traceEvents\":[{}";
			m_OutputStream.flush();
		}

		void WriteFooter()
		{
			m_OutputStream << "]}";
			m_OutputStream.flush();
		}

		// Note: you must already own lock on m_Mutex before
		// calling InternalEndSession()
		void InternalEndSession()
		{
			if (m_CurrentSession)
			{
				WriteFooter();
				m_OutputStream.close();
				delete m_CurrentSession;
				m_CurrentSession = nullptr;
			}
		}
	private:
		std::mutex m_Mutex;
		InstrumentationSession* m_CurrentSession;
		std::ofstream m_OutputStream;
	};

	class InstrumentationTimer
	{
	public:
		InstrumentationTimer(const char* name)
			: m_Name(name), m_Stopped(false)
		{
			m_StartTimepoint = std::chrono::steady_clock::now();
		}

		~InstrumentationTimer()
		{
			if (!m_Stopped)
				Stop();
		}

		std::chrono::milliseconds GetElapsedTime() const
		{
			// Convert microseconds to milliseconds
			return std::chrono::duration_cast<std::chrono::milliseconds>(m_elapsedTime);
		}

		void Stop()
		{
			auto endTimepoint = std::chrono::steady_clock::now();
			auto highResStart = FloatingPointMicroseconds{ m_StartTimepoint.time_since_epoch() };
			m_elapsedTime = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch() - std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch();

			Instrumentor::Get().WriteProfile({ m_Name, highResStart, m_elapsedTime, std::this_thread::get_id() });

			m_Stopped = true;
		}
	private:
		const char* m_Name;
		std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
		bool m_Stopped;
		std::chrono::microseconds m_elapsedTime;
	};

	namespace InstrumentorUtils {

		template <size_t N>
		struct ChangeResult
		{
			char Data[N];
		};

		template <size_t N, size_t K>
		constexpr auto CleanupOutputString(const char(&expr)[N], const char(&remove)[K])
		{
			ChangeResult<N> result = {};

			size_t srcIndex = 0;
			size_t dstIndex = 0;
			while (srcIndex < N)
			{
				size_t matchIndex = 0;
				while (matchIndex < K - 1 && srcIndex + matchIndex < N - 1 && expr[srcIndex + matchIndex] == remove[matchIndex])
					matchIndex++;
				if (matchIndex == K - 1)
					srcIndex += matchIndex;
				result.Data[dstIndex++] = expr[srcIndex] == '"' ? '\'' : expr[srcIndex];
				srcIndex++;
			}
			return result;
		}
	}
}

#define EE_PROFILE 1
#if EE_PROFILE
	// Resolve which function signature macro will be used. Note that this only
	// is resolved when the (pre)compiler starts, so the syntax highlighting
	// could mark the wrong one in your editor!
	
		#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
			#define EE_FUNC_SIG __PRETTY_FUNCTION__
		#elif defined(__DMC__) && (__DMC__ >= 0x810)
			#define EE_FUNC_SIG __PRETTY_FUNCTION__
		#elif (defined(__FUNCSIG__) || (_MSC_VER))
			#define EE_FUNC_SIG __FUNCSIG__
		#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
			#define EE_FUNC_SIG __FUNCTION__
		#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
			#define EE_FUNC_SIG __FUNC__
		#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
		#define EE_FUNC_SIG __func__
		#elif defined(__cplusplus) && (__cplusplus >= 201103)
		#define EE_FUNC_SIG __func__
		#else
			#define EE_FUNC_SIG "EE_FUNC_SIG unknown!"
		#endif
	

	#define EE_PROFILE_BEGIN_SESSION(name, filepath) ::Engine::Instrumentor::Get().BeginSession(name, filepath)
	#define EE_PROFILE_END_SESSION() ::Engine::Instrumentor::Get().EndSession()
	#define EE_PROFILE_SCOPE_LINE2(name, line) constexpr auto fixedName##line = ::Engine::InstrumentorUtils::CleanupOutputString(name, "__cdecl ");\
												   ::Engine::InstrumentationTimer timer##line(fixedName##line.Data)
	#define EE_PROFILE_SCOPE_LINE(name, line) EE_PROFILE_SCOPE_LINE2(name, line)
	#define EE_PROFILE_SCOPE(name) EE_PROFILE_SCOPE_LINE(name, __LINE__)
	#define EE_PROFILE_FUNCTION() EE_PROFILE_SCOPE(EE_FUNC_SIG)
#else
	#define EE_PROFILE_BEGIN_SESSION(name, filepath)
	#define EE_PROFILE_END_SESSION()
	#define EE_PROFILE_SCOPE(name)
	#define EE_PROFILE_FUNCTION()
#endif