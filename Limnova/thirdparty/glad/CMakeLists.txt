project(Glad)

add_library(${PROJECT_NAME} STATIC
	"include/glad/glad.h"
	"include/KHR/khrplatform.h"
	"src/glad.c"
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		"${PROJECT_SOURCE_DIR}/include"
)

# Build properties
set_property(TARGET ${PROJECT_NAME} PROPERTY
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)