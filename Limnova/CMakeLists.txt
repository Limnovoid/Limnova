project(Limnova)

add_library(${PROJECT_NAME} SHARED
	"${PROJECT_SOURCE_DIR}/src/Application.cpp"
	"${PROJECT_SOURCE_DIR}/src/Application.h"
	"${PROJECT_SOURCE_DIR}/src/EntryPoint.h"
	"${PROJECT_SOURCE_DIR}/src/Log.cpp"

	"${PROJECT_SOURCE_DIR}/src/Events/Event.h"
	"${PROJECT_SOURCE_DIR}/src/Events/KeyEvent.h"
	"${PROJECT_SOURCE_DIR}/src/Events/MouseEvent.h"
	"${PROJECT_SOURCE_DIR}/src/Events/ApplicationEvent.h"

	"${PROJECT_SOURCE_DIR}/src/Platform/Windows/WindowsWindow.cpp"
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		"${PROJECT_SOURCE_DIR}/src"
)

# Precompiled header
target_precompile_headers(${PROJECT_NAME}
	PRIVATE
		<iostream>
		<memory>
		<utility>
		<algorithm>
		<functional>

		<string>
		<sstream>
		<vector>
		<unordered_map>
		<unordered_set>

		$<${MSVC}:<Windows.h$<ANGLE-R>>

		"${PROJECT_SOURCE_DIR}/src/Log.h"
)

# Definitions
add_compile_definitions(
	LV_BUILD_DLL
)
configure_file("${PROJECT_SOURCE_DIR}/src/Core.h.in" "${PROJECT_SOURCE_DIR}/src/Core.h")


# Third party:
target_include_directories(${PROJECT_NAME}
	PRIVATE
		"${PROJECT_SOURCE_DIR}/thirdparty/spdlog/include"
		"${PROJECT_SOURCE_DIR}/thirdparty/glfw/include"
)

add_subdirectory(thirdparty/glfw)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		glfw
)


# Copy Limnova.dll to dependent executable directories.
add_custom_command(TARGET Limnova POST_BUILD COMMAND
	${CMAKE_COMMAND} -E copy_if_different
	${PROJECT_BINARY_DIR}/Limnova.dll
	${CMAKE_BINARY_DIR}/DevTool
)