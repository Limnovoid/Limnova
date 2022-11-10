project (Volk)

add_library(${PROJECT_NAME} STATIC
    "volk.c"
    "include/volk.h"
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/Limnova/thirdparty/vulkan/include"
)

set_property(TARGET ${PROJECT_NAME} PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
