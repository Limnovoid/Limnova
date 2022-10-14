cmake_minimum_required(VERSION 3.21)
project(LimnovaEngine)

# Definitions.
add_compile_definitions(
	# Platform
	$<${MSVC}:LV_PLATFORM_WINDOWS>

	# Build configuration
	$<$<CONFIG:Debug>:LV_DEBUG>
	$<$<CONFIG:Release>:LV_RELEASE>

	LV_ENABLE_ASSERTS
)

# Projects.
add_subdirectory(Limnova)
add_subdirectory(DevTool)