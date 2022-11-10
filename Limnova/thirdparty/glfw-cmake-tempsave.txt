project(GLFW)

add_library(${PROJECT_NAME} STATIC
	"include/GLFW/glfw3.h"
	"include/GLFW/glfw3native.h"
	"src/context.c"
	"src/init.c"
	"src/input.c"
	"src/internal.h"
	"src/mappings.h"
	"src/monitor.c"
	"src/null_init.c"
	"src/null_joystick.c"
	"src/null_joystick.h"
	"src/null_monitor.c"
	"src/null_platform.h"
	"src/null_window.c"
	"src/platform.c"
	"src/platform.h"
	"src/vulkan.c"
	"src/window.c"

	$<${MSVC}:
		"src/win32_init.c"
		"src/win32_joystick.c"
		"src/win32_module.c"
		"src/win32_monitor.c"
		"src/win32_thread.c"
		"src/win32_time.c"
		"src/win32_window.c"
		"src/wgl_context.c"
		"src/egl_context.c"
		"src/osmesa_context.c"
	>
)


add_compile_definitions(
	$<${MSVC}:_GLFW_WIN32=1>
	$<${MSVC}:_CRT_SECURE_NO_WARNINGS>
)


set_property(TARGET ${PROJECT_NAME} PROPERTY
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
