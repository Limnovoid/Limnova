cmake_minimum_required(VERSION 3.21)
project(LimnovaEngine)

# Platform specifics.
if (MSVC)
	add_compile_definitions(
		LV_PLATFORM_WINDOWS
	)
else ()

endif ()

# Configuration specifics.
add_compile_definitions(
	$<$<CONFIG:Debug>:LV_DEBUG>
	$<$<CONFIG:Release>:LV_RELEASE>
)

# Binaries.
add_subdirectory(Limnova)
add_subdirectory(DevTool)