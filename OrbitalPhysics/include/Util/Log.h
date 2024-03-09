#ifndef LV_LOG_H
#define LV_LOG_H

#include "Format.h"

#include <iostream>

#if defined(LV_CXX20)
#include <source_location>
#include <format>
#endif

namespace Limnova
{

// Solution for macro overloading (https://stackoverflow.com/questions/11761703/overloading-macro-on-number-of-arguments)
#define LV_ARG_N( \
      _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
     _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
     _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
     _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
     _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
     _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
     _61,_62,_63,N,...) N
#define LV_RSEQ_N() \
     63,62,61,60,                   \
     59,58,57,56,55,54,53,52,51,50, \
     49,48,47,46,45,44,43,42,41,40, \
     39,38,37,36,35,34,33,32,31,30, \
     29,28,27,26,25,24,23,22,21,20, \
     19,18,17,16,15,14,13,12,11,10, \
     9,8,7,6,5,4,3,2,1,0
#define LV_NARG_I_(...) LV_ARG_N(__VA_ARGS__)
#define LV__NARG__(...)  LV_NARG_I_(__VA_ARGS__,LV_RSEQ_N())
#define LV__VFUNC__(name, n) name##n
#define LV_VFUNC_(name, n) LV__VFUNC__(name, n)
#define LV_VFUNC(func, ...) LV_VFUNC_(func, LV__NARG__(__VA_ARGS__)) (__VA_ARGS__)

// ---------------------------------------------------------------------------------------------------------------------------------

#if defined(LV_MSVC)

#define LV_DEBUG_BREAK __debugbreak();

#else

#define LV_DEBUG_BREAK while(true) {}

#endif // defined(LV_MSVC)

// ---------------------------------------------------------------------------------------------------------------------------------

#if defined(LV_CXX20)

#define LV_SOURCE_LOCATION_STRING \
	Fmt::Format("{}:{}:{}", std::source_location::current().file_name(), std::source_location::current().line(), std::source_location::current().function_name())

#elif defined(LV_MSVC)

#define LV_SOURCE_LOCATION_STRING __FUNCTION__

#else

#define LV_SOURCE_LOCATION_STRING __func__

#endif // if defined(LV_CXX20)

// ---------------------------------------------------------------------------------------------------------------------------------


#define LV_LOG(...) std::cerr << "Warning: " << Fmt::Format(__VA_ARGS__) << "\n    from: " << LV_SOURCE_LOCATION_STRING << std::endl;

#define LV_WARN(...) std::cerr << "Warning: " << Fmt::Format(__VA_ARGS__) << "\n    from: " << LV_SOURCE_LOCATION_STRING << std::endl;

// ---------------------------------------------------------------------------------------------------------------------------------

#define LV_ASSERT(expression, ...) \
	if (!(expression)) { LV_LOG(__VA_ARGS__); LV_DEBUG_BREAK; }


}

#endif // ifndef LV_LOG_H
