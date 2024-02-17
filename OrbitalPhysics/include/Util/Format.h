#ifndef LV_FORMAT_H
#define LV_FORMAT_H

#if defined(LV_CXX20)
#include <format>
#endif

namespace Limnova
{

	class Fmt
	{
	public:
		template<class... Args>
		static std::string Format(const char *pMessage, Args... args);
	};

// ---------------------------------------------------------------------------------------------------------------------------------

	template<class... Args>
	inline std::string Fmt::Format(const char *pMessage, Args... args)
	{
#if defined(LV_CXX20)
		return std::format(pMessage, args...);
#else
		LV_ASSERT(false, "Fmt::Format is not supported below C++20");
		return std::string();
#endif
	}

}

#endif // ifndef LV_FORMAT_H
