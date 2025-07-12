#include "bearpch.h"
#include "Log.h"
#include "spdlog/sinks/stdout_color_sinks.h"
namespace Bear {

	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;
	void Log::Init() {
		// 设置日志格式：带颜色的时间戳、日志名称和实际消息
		spdlog::set_pattern("%^[%T] %n: %v%$");
		// 创建引擎核心日志记录器，输出到彩色控制台
		s_CoreLogger = spdlog::stdout_color_mt("BEAR");
		// 设置日志级别为trace(最低级别，显示所有日志)
		s_CoreLogger->set_level(spdlog::level::trace);

		s_ClientLogger = spdlog::stdout_color_mt("APP");
		s_ClientLogger->set_level(spdlog::level::trace);
	}
}