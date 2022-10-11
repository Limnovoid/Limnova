cmake_minimum_required(VERSION 3.21)

project(LimnovaEngine)


# Platform specifics.
if (MSVC)
	add_compile_definitions(
		LV_PLATFORM_WINDOWS
	)

else ()

endif ()


add_subdirectory(Limnova)
add_subdirectory(DevTool)