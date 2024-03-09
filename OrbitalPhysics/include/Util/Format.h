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
		template<typename... Args>
		static std::string Format(char const* pFormat, Args... args);

		template<typename... Args>
		static std::string Format(std::string const& string, Args... args);
	};

// ---------------------------------------------------------------------------------------------------------------------------------

	template<typename... Args>
	std::string Fmt::Format(char const* pFormat, Args... args)
	{
#if defined(LV_CXX20)
		return std::vformat(pFormat, std::make_format_args(std::forward<Args>(args)...));
#else
		std::cerr << "Fmt::Format is not supported below C++20." << std::endl;
		return std::string(pFormat);
#endif
	}

// ---------------------------------------------------------------------------------------------------------------------------------

	template<typename... Args>
	std::string Fmt::Format(std::string const& string, Args... args)
	{
		return Format<Args...>(string.c_str(), std::forward(args)...);
	}

}

#endif // ifndef LV_FORMAT_H
