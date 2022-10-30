#ifdef LV_PLATFORM_WINDOWS
    #ifdef LV_BUILD_DLL
        #define LIMNOVA_API __declspec(dllexport)
    #else
        #define LIMNOVA_API __declspec(dllimport)
    #endif
#else
    #error Limnova only supports Windows!
#endif


#ifdef LV_DEBUG
    #define LV_ENABLE_ASSERTS
#endif

#ifdef LV_ENABLE_ASSERTS
    #define LV_ASSERT(x, ...) { if(!(x)) { LV_ERROR("Assertion failed: {0}", __VA_ARGS__); __debugbreak(); } }
    #define LV_CORE_ASSERT(x, ...) { if(!(x)) { LV_CORE_ERROR("Assertion failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
    #define LV_ASSERT(x, ...)
    #define LV_CORE_ASSERT(x, ...)
#endif


#define BIT(x) (1 << x)


#define LV_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)
