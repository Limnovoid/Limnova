project(Limnova)

add_library(${PROJECT_NAME} SHARED
	"src/Application.cpp"
	"src/EntryPoint.h"
	"src/Log.cpp"
)

# Definitions
add_compile_definitions(
	LV_BUILD_DLL
)
configure_file("src/Core.h.in" "${PROJECT_SOURCE_DIR}/src/Core.h")

# Third party
target_include_directories(${PROJECT_NAME}
	PRIVATE
		"${CMAKE_SOURCE_DIR}/Limnova/thirdparty/spdlog/include"
)


# Copy Limnova.dll to dependent executable directories.
add_custom_command(TARGET Limnova POST_BUILD COMMAND
	${CMAKE_COMMAND} -E copy_if_different
	${PROJECT_BINARY_DIR}/Limnova.dll
	${CMAKE_BINARY_DIR}/DevTool
	)