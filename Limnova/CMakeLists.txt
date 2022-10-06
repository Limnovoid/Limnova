project(Limnova)

add_library(${PROJECT_NAME} SHARED "src/Application.cpp" "src/EntryPoint.h")

# Core.h
add_compile_definitions(
	LV_BUILD_DLL
)
configure_file("src/Core.h.in" "${PROJECT_SOURCE_DIR}/src/Core.h")
target_include_directories(${PROJECT_NAME}
	PRIVATE
		"${PROJECT_BINARY_DIR}/src"
)