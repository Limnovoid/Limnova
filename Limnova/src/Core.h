#ifdef LV_PLATFORM_WINDOWS
	#ifdef LV_BUILD_DLL
		#define LIMNOVA_API __declspec(dllexport)
	#else
		#define LIMNOVA_API __declspec(dllimport)
	#endif
#else
	#error Limnova only supports Windows!
#endif