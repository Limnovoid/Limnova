#ifndef LV_LOG_H
#define LV_LOG_H

#include <iostream>

#if defined(LV_CXX20)
#include <source_location>
#include <format>
#endif

namespace Limnova
{

#if defined(LV_CXX20)

#define LV_LOG(message) \
	const std::source_location location = std::source_location::current();	\
	std::cerr << "Log from " << location.file_name() << ":" << location.line() << ":" << location.function_name() << ": "	\
		<< message << std::endl;

#elif defined(LV_MSVC)

#define LV_LOG(message) \
	std::cerr << "Log from " << __FUNCTION__ << ": " << message << std::endl;

#else

#define LV_LOG(message) \
	std::cerr << "Log: " << message << std::endl;

#endif

// ---------------------------------------------------------------------------------------------------------------------------------

#if defined(LV_MSVC)

#define LV_ASSERT(expression) \
	if (!expression) __debugbreak();

#define LV_ASSERT(expression, message) \
	if (!expression) { LV_LOG(message); __debugbreak();

#else

#define LV_ASSERT(expression, message) \
	if (!expression) { LV_LOG(message); assert(false);

#endif

}

#endif // ifndef LV_LOG_H
