cmake_minimum_required(VERSION 3.21)

project(LimnovaEngine)


# Platform specifics.
if (MSVC)
	add_compile_definitions(
		LV_PLATFORM_WINDOWS
	)

else ()

endif ()


# .exe and .dll files to bin/
set(EXECUTABLE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(Limnova)
add_subdirectory(DevTool)