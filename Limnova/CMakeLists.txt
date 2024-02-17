project(Limnova)

add_library(${PROJECT_NAME} STATIC
    "src/Core/Application.cpp"
    "src/Core/EntryPoint.h"
    "src/Core/Log.cpp"
    "src/Core/Layer.cpp"
    "src/Core/LayerStack.cpp"
    "src/Core/Timer.h"
    "src/Core/Typedefs.h"
    "src/Core/UUID.cpp"

    "src/Events/Event.h"
    "src/Events/KeyEvent.h"
    "src/Events/MouseEvent.h"
    "src/Events/ApplicationEvent.h"

    "src/ImGui/ImGuiBuild.cpp"
    "src/ImGui/ImGuiLayer.h"
    "src/ImGui/ImGuiLayer.cpp"

    #"src/Profiling/DataRecording.h"
    "src/Profiling/Instrumentor.h"

    "src/Orbital/OrbitalScene.cpp"

    # TODO - platform specific
    "src/Platform/OpenGL/OpenGLBuffer.cpp"
    "src/Platform/OpenGL/OpenGLContext.cpp"
    "src/Platform/OpenGL/OpenGLFramebuffer.cpp"
    "src/Platform/OpenGL/OpenGLRendererAPI.cpp"
    "src/Platform/OpenGL/OpenGLShader.cpp"
    "src/Platform/OpenGL/OpenGLTexture.cpp"
    "src/Platform/OpenGL/OpenGLVertexArray.cpp"
    #"src/Platform/Vulkan/VulkanContext.cpp"
    #"src/Platform/Vulkan/vktostring.h"
    #"src/Platform/Vulkan/vktostring.cpp"
    #"src/Platform/Vulkan/vkinit.cpp"
    "src/Platform/Windows/WindowsInput.cpp"
    "src/Platform/Windows/WindowsUtils.cpp"
    "src/Platform/Windows/WindowsWindow.cpp"

    "src/Renderer/Buffer.cpp"
    "src/Renderer/Camera.cpp"
    "src/Renderer/EditorCamera.cpp"
    "src/Renderer/Framebuffer.cpp"
    "src/Renderer/PlanarCameraController.cpp"
    "src/Renderer/PointCameraController.cpp"
    "src/Renderer/RenderCommand.cpp"
    "src/Renderer/Renderer.cpp"
    "src/Renderer/Renderer2D.cpp"
    "src/Renderer/RendererAPI.cpp"
    "src/Renderer/RenderingContext.h"
    "src/Renderer/Shader.cpp"
    "src/Renderer/SubTexture.cpp"
    "src/Renderer/Texture.cpp"
    "src/Renderer/VertexArray.cpp"

    "src/Scene/Components.cpp"
    "src/Scene/Entity.cpp"
    "src/Scene/Reflection.cpp"
    "src/Scene/Scene.cpp"
    "src/Scene/Script.h"
    "src/Scene/SceneSerializer.cpp"

    "src/Scripting/ScriptEngine.cpp"
    "src/Scripting/ScriptLibrary.cpp"

    "src/Structures/Queue.h"

    "src/Utils/InputUtils.cpp"

    "thirdparty/stb_image/stb_image.cpp"
)


set_property(TARGET ${PROJECT_NAME}
    PROPERTY CXX_STANDARD 20
)

add_compile_definitions(
    LV_REFL_MAX_NUM_FIELDS=128 # TODO - make work
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${PROJECT_SOURCE_DIR}/src"
        "C:/Dev/Limnova/OrbitalPhysics/include"

        "${PROJECT_SOURCE_DIR}/thirdparty"
        "${PROJECT_SOURCE_DIR}/thirdparty/entt/include"
        "${PROJECT_SOURCE_DIR}/thirdparty/glad/include"
        "${PROJECT_SOURCE_DIR}/thirdparty/glfw/include"
        "${PROJECT_SOURCE_DIR}/thirdparty/imgui"
        "${PROJECT_SOURCE_DIR}/thirdparty/imguizmo"
        "${PROJECT_SOURCE_DIR}/thirdparty/mono/include"
        "${PROJECT_SOURCE_DIR}/thirdparty/pcg-cpp/include"
        "${PROJECT_SOURCE_DIR}/thirdparty/spdlog/include"
        "${PROJECT_SOURCE_DIR}/thirdparty/stb_image"
        "${PROJECT_SOURCE_DIR}/thirdparty/yaml-cpp/include"

        ${Vulkan_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        OrbitalPhysics
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
        <iostream>
        <memory>
        <utility>
        <algorithm>
        <functional>
        <optional>
        <limits>
        <cstdarg>
        <filesystem>

        <string>
        <sstream>
        <fstream>
        <array>
        <vector>
        <map>
        <unordered_map>
        <unordered_set>

        #$<${MSVC}:<Windows.h$<ANGLE-R>>

        "${PROJECT_SOURCE_DIR}/src/Core/Log.h"
        "${PROJECT_SOURCE_DIR}/src/Profiling/Instrumentor.h"
)


# Core.h config
configure_file("${PROJECT_SOURCE_DIR}/src/Core/Core.h.in" "${PROJECT_SOURCE_DIR}/src/Core/Core.h")


# Third party libs:
#target_include_directories(${PROJECT_NAME}
#    PRIVATE
#        "${PROJECT_SOURCE_DIR}/thirdparty"
#        "${PROJECT_SOURCE_DIR}/thirdparty/entt/include"
#        "${PROJECT_SOURCE_DIR}/thirdparty/glad/include"
#        "${PROJECT_SOURCE_DIR}/thirdparty/glfw/include"
#        "${PROJECT_SOURCE_DIR}/thirdparty/imgui"
#        "${PROJECT_SOURCE_DIR}/thirdparty/imguizmo"
#        "${PROJECT_SOURCE_DIR}/thirdparty/mono/include"
#        "${PROJECT_SOURCE_DIR}/thirdparty/pcg-cpp/include"
#        "${PROJECT_SOURCE_DIR}/thirdparty/spdlog/include"
#        "${PROJECT_SOURCE_DIR}/thirdparty/stb_image"
#        "${PROJECT_SOURCE_DIR}/thirdparty/yaml-cpp/include"
#
#    PRIVATE
#        ${Vulkan_INCLUDE_DIR}
#)

add_subdirectory(thirdparty)
add_subdirectory(thirdparty/glad)
add_subdirectory(thirdparty/glfw)
add_subdirectory(thirdparty/yaml-cpp)

set_property(TARGET ${PROJECT_NAME}
    PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        opengl32.lib
        Glad
        GLFW

        "${PROJECT_SOURCE_DIR}/thirdparty/mono/lib/$<IF:$<CONFIG:Debug>,Debug,Release>/libmono-static-sgen.lib"
        $<${WIN32}:Ws2_32 Winmm Version Bcrypt> # Mono Windows
    PUBLIC
        ImGui
        ImGuizmo
        yaml-cpp
)

add_compile_definitions(
    GLFW_INCLUDE_NONE
    _CRT_SECURE_NO_WARNINGS
)

# Getting yaml to play nice
set_property(TARGET yaml-cpp
    PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)
set_property(TARGET yaml-cpp-sandbox
    PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)
set_property(TARGET yaml-cpp-parse
    PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)
set_property(TARGET yaml-cpp-read
    PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)

# Vulkan
find_package(Vulkan 1.3 REQUIRED
    volk
    shaderc_combined
    SPIRV-Tools
    glslc
)

#target_include_directories(${PROJECT_NAME}
#    PRIVATE
#        ${Vulkan_INCLUDE_DIR}
#)

get_filename_component(Vulkan_Lib ${Vulkan_LIBRARY} DIRECTORY)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${Vulkan_LIBRARY}
        ${Vulkan_volk_LIBRARY}
        "${Vulkan_Lib}/shaderc_sharedd.lib"
        "${Vulkan_Lib}/spirv-cross-cored.lib"
        "${Vulkan_Lib}/spirv-cross-glsld.lib"
        "${Vulkan_Lib}/SPIRV-Toolsd.lib"
)

# Copy Limnova.dll to dependent executable directories.
#add_custom_command(TARGET Limnova POST_BUILD COMMAND
#    ${CMAKE_COMMAND} -E copy_if_different
#    ${PROJECT_BINARY_DIR}/Limnova.dll   # File to copy
#    ${CMAKE_BINARY_DIR}/DevTool         # Destination
#)
