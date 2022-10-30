project(Limnova)

add_library(${PROJECT_NAME} SHARED
	"src/Application.cpp"
	"src/EntryPoint.h"
	"src/Log.cpp"
	"src/Layer.cpp"
	"src/LayerStack.cpp"

	"src/Events/Event.h"
	"src/Events/KeyEvent.h"
	"src/Events/MouseEvent.h"
	"src/Events/ApplicationEvent.h"

	"src/Math/glm.h"
	"src/Math/Math.cpp"
	"src/Math/Vector3.cpp"
	"src/Math/Quaternion.cpp"

	"src/Platform/Windows/WindowsWindow.cpp"
	"src/Platform/Windows/WindowsInput.cpp"
    

	"src/ImGui/ImGuiBuild.cpp"
	"src/ImGui/ImGuiLayer.h"
	"src/ImGui/ImGuiLayer.cpp"
)


target_include_directories(${PROJECT_NAME}
	PRIVATE
		"${PROJECT_SOURCE_DIR}/src"
)


add_compile_definitions(
	LV_BUILD_DLL
)


set_property(TARGET ${PROJECT_NAME}
	PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)
set_property(TARGET ${PROJECT_NAME}
	PROPERTY CXX_STANDARD 17	
)


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


# Core.h config
configure_file("${PROJECT_SOURCE_DIR}/src/Core.h.in" "${PROJECT_SOURCE_DIR}/src/Core.h")


# Third party libs:
target_include_directories(${PROJECT_NAME}
	PRIVATE
		"${PROJECT_SOURCE_DIR}/thirdparty"
		"${PROJECT_SOURCE_DIR}/thirdparty/glad/include"
		"${PROJECT_SOURCE_DIR}/thirdparty/glfw/include"
		"${PROJECT_SOURCE_DIR}/thirdparty/imgui"
		"${PROJECT_SOURCE_DIR}/thirdparty/spdlog/include"
)

add_subdirectory(thirdparty/glad)
add_subdirectory(thirdparty/glfw)
add_subdirectory(thirdparty/imgui)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		Glad
		GLFW
		opengl32.lib
	PUBLIC
		ImGui
)

add_compile_definitions(
	GLFW_INCLUDE_NONE
)


# Copy Limnova.dll to dependent executable directories.
add_custom_command(TARGET Limnova POST_BUILD COMMAND
	${CMAKE_COMMAND} -E copy_if_different
	${PROJECT_BINARY_DIR}/Limnova.dll	# File to copy
	${CMAKE_BINARY_DIR}/DevTool			# Destination
)