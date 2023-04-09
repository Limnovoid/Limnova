#pragma once

// Platform detection using predefined macros
#ifdef _WIN32
    // Windows x64/x86
    #ifdef _WIN64
        // Window x64
        #define LV_PLATFORM_WINDOWS
    #else
        #error "x86 builds are not supported!"
    #endif
#else
    #error "Non-Windows build are not supported!"
#endif

#ifdef LV_PLATFORM_WINDOWS
    #ifdef LV_DYNAMIC_LINKING
        #ifdef LV_BUILD_DLL
            #define LIMNOVA_API __declspec(dllexport)
        #else
            #define LIMNOVA_API __declspec(dllimport)
        #endif
    #else
        #define LIMNOVA_API
    #endif
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


//#define LV_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)
#define LV_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

#define LV_DIR "C:/Programming/source/Limnova"
#define LV_ASSET_DIR "C:/Programming/source/Limnova/Limnova/assets"


namespace Limnova
{

    template<typename T>
    using Ref = std::shared_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Ref<T> CreateRef(Args&& ... args)
    {
        return std::make_shared<T>(std::forward<Args>(args) ... );
    }

    template<typename T>
    using Scope = std::unique_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Scope<T> CreateScope(Args&& ... args)
    {
        return std::make_unique<T>(std::forward<Args>(args) ... );
    }

}
