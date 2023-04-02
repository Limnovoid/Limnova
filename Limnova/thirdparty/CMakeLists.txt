project(ImGui) #--------------------------------------------------------

add_library(${PROJECT_NAME} STATIC
    "imgui/imconfig.h"
    "imgui/imgui.h"
    "imgui/imgui.cpp"
    "imgui/imgui_demo.cpp"
    "imgui/imgui_draw.cpp"
    "imgui/imgui_internal.h"
    "imgui/imgui_tables.cpp"
    "imgui/imgui_widgets.cpp"
    "imgui/imstb_rectpack.h"
    "imgui/imstb_textedit.h"
    "imgui/imstb_truetype.h"
)

set_property(TARGET ${PROJECT_NAME} PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

add_compile_definitions(
    IMGUI_IMPL_OPENGL_LOADER_CUSTOM
)


project(ImGuizmo) #-----------------------------------------------------

add_library(${PROJECT_NAME} STATIC
    "imguizmo/ImGuizmo.h"
    "imguizmo/ImGuizmo.cpp"
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${PROJECT_SOURCE_DIR}/imgui"
)

set_property(TARGET ${PROJECT_NAME} PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
