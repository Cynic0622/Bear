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
#define BEAR_CORE_TRACE(...) ::Bear::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define BEAR_CORE_INFO(...)  ::Bear::Log::GetCoreLogger()->info(__VA_ARGS__)
#define BEAR_CORE_WARN(...)  ::Bear::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define BEAR_CORE_ERROR(...) ::Bear::Log::GetCoreLogger()->error(__VA_ARGS__)
// Client log
#define BEAR_CLIENT_TRACE(...) ::Bear::Log::GetClientLogger()->trace(__VA_ARGS__)
#define BEAR_CLIENT_INFO(...)  ::Bear::Log::GetClientLogger()->info(__VA_ARGS__)
#define BEAR_CLIENT_WARN(...)  ::Bear::Log::GetClientLogger()->warn(__VA_ARGS__)
#define BEAR_CLIENT_ERROR(...) ::Bear::Log::GetClientLogger()->error(__VA_ARGS__)