#pragma once

#include "Core.h"
#include "spdlog/spdlog.h"

#include <memory>


namespace Limnova
{

	class LIMNOVA_API Log
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


// Core log macros
#define LV_CORE_TRACE(...)  ::Limnova::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define LV_CORE_INFO(...)   ::Limnova::Log::GetCoreLogger()->info(__VA_ARGS__)
#define LV_CORE_WARN(...)   ::Limnova::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define LV_CORE_ERROR(...)  ::Limnova::Log::GetCoreLogger()->error(__VA_ARGS__)
#define LV_CORE_CRITICAL(...)  ::Limnova::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define LV_TRACE(...)       ::Limnova::Log::GetClientLogger()->trace(__VA_ARGS__)
#define LV_INFO(...)        ::Limnova::Log::GetClientLogger()->info(__VA_ARGS__)
#define LV_WARN(...)        ::Limnova::Log::GetClientLogger()->warn(__VA_ARGS__)
#define LV_ERROR(...)       ::Limnova::Log::GetClientLogger()->error(__VA_ARGS__)
#define LV_CRITICAL(...)       ::Limnova::Log::GetClientLogger()->critical(__VA_ARGS__)