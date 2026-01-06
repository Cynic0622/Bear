#pragma once
#include "Core.h"
#include <spdlog/spdlog.h>
namespace Bear {


	class BEAR_API Log
	{
	public:
		static void Init();
		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

// Core log
#ifdef BEAR_DEBUG
#define BEAR_CORE_TRACE(...) ::Bear::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define BEAR_CORE_WARN(...)  ::Bear::Log::GetCoreLogger()->warn(__VA_ARGS__)
#else
#define BEAR_CORE_TRACE(...)
#define BEAR_CORE_WARN(...)
#endif
#define BEAR_CORE_INFO(...)  ::Bear::Log::GetCoreLogger()->info(__VA_ARGS__)
#define BEAR_CORE_ERROR(...) ::Bear::Log::GetCoreLogger()->error(__VA_ARGS__); __debugbreak()

// Client log
#define BEAR_CLIENT_TRACE(...) ::Bear::Log::GetClientLogger()->trace(__VA_ARGS__)
#define BEAR_CLIENT_INFO(...)  ::Bear::Log::GetClientLogger()->info(__VA_ARGS__)
#define BEAR_CLIENT_WARN(...)  ::Bear::Log::GetClientLogger()->warn(__VA_ARGS__)
#define BEAR_CLIENT_ERROR(...) ::Bear::Log::GetClientLogger()->error(__VA_ARGS__)

#define BEAR_CORE_ASSERT(x, ...) \
	do { \
		if (!(x)) { \
			BEAR_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); \
			__debugbreak(); \
		} \
		} while(0)
#define BEAR_CLIENT_ASSERT(x, ...) if(!(x)) { BEAR_CLIENT_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); }