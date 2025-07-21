#pragma once
#include "Core.h"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h> // for std::string support
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
#define BEAR_CORE_TRACE(...) ::Bear::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define BEAR_CORE_INFO(...)  ::Bear::Log::GetCoreLogger()->info(__VA_ARGS__)
#define BEAR_CORE_WARN(...)  ::Bear::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define BEAR_CORE_ERROR(...) { ::Bear::Log::GetCoreLogger()->error(__VA_ARGS__); __debugbreak(); }

// Client log
#define BEAR_CLIENT_TRACE(...) ::Bear::Log::GetClientLogger()->trace(__VA_ARGS__)
#define BEAR_CLIENT_INFO(...)  ::Bear::Log::GetClientLogger()->info(__VA_ARGS__)
#define BEAR_CLIENT_WARN(...)  ::Bear::Log::GetClientLogger()->warn(__VA_ARGS__)
#define BEAR_CLIENT_ERROR(...) ::Bear::Log::GetClientLogger()->error(__VA_ARGS__)

#ifdef BEAR_ENABLE_ASSERTS
#define BEAR_CORE_ASSERT(x, ...) { if(!(x)) { BEAR_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#define BEAR_CLIENT_ASSERT(x, ...) { if(!(x)) { BEAR_CLIENT_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
// 如果没有启用断言，则定义为一个空宏，这样在代码中使用断言时不会有任何影响
#define BEAR_CORE_ASSERT(x, ...)
#define BEAR_CLIENT_ASSERT(x, ...)
#endif