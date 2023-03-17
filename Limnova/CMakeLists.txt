project(Limnova)

add_library(${PROJECT_NAME} STATIC
    "src/Core/Application.cpp"
    "src/Core/EntryPoint.h"
    "src/Core/Log.cpp"
    "src/Core/Layer.cpp"
    "src/Core/LayerStack.cpp"

    "src/Events/Event.h"
    "src/Events/KeyEvent.h"
    "src/Events/MouseEvent.h"
    "src/Events/ApplicationEvent.h"

    "src/ImGui/ImGuiBuild.cpp"
    "src/ImGui/ImGuiLayer.h"
    "src/ImGui/ImGuiLayer.cpp"

    "src/Profiling/DataRecording.h"
    "src/Profiling/Instrumentor.h"

    "src/Math/glm.h"
    "src/Math/Math.cpp"
    "src/Math/BigFloat.cpp"
    "src/Math/BigVector2.cpp"
    "src/Math/Matrix4.cpp"
    "src/Math/Vector2.cpp"
    "src/Math/Vector3.cpp"
    "src/Math/Vector4.cpp"
    "src/Math/Quaternion.cpp"

    "src/Platform/OpenGL/OpenGLBuffer.cpp"
    "src/Platform/OpenGL/OpenGLContext.cpp"
    "src/Platform/OpenGL/OpenGLFramebuffer.cpp"
    "src/Platform/OpenGL/OpenGLRendererAPI.cpp"
    "src/Platform/OpenGL/OpenGLShader.cpp"
    "src/Platform/OpenGL/OpenGLTexture.cpp"
    "src/Platform/OpenGL/OpenGLVertexArray.cpp"
    "src/Platform/Vulkan/VulkanContext.cpp"
    "src/Platform/Vulkan/vktostring.h"
    "src/Platform/Vulkan/vktostring.cpp"
    "src/Platform/Vulkan/vkinit.cpp"
    "src/Platform/Windows/WindowsWindow.cpp"
    "src/Platform/Windows/WindowsInput.cpp"

    "src/Renderer/Buffer.cpp"
    "src/Renderer/Camera.h"
    "src/Renderer/Framebuffer.cpp"
    "src/Renderer/OrthographicCamera.cpp"
    "src/Renderer/PerspectiveCamera.cpp"
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

    "src/Scene/Components.h"
    "src/Scene/Entity.cpp"
    "src/Scene/Scene.cpp"
    "src/Scene/Script.h"

    "src/Structures/Queue.h"

    "thirdparty/stb_image/stb_image.cpp"
)


set_property(TARGET ${PROJECT_NAME}
    PROPERTY CXX_STANDARD 20
)


target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${PROJECT_SOURCE_DIR}/src"
)


#add_compile_definitions(
#    LV_BUILD_DLL
#)


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
target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${PROJECT_SOURCE_DIR}/thirdparty"
        "${PROJECT_SOURCE_DIR}/thirdparty/entt/include"
        "${PROJECT_SOURCE_DIR}/thirdparty/glad/include"
        "${PROJECT_SOURCE_DIR}/thirdparty/glfw/include"
        "${PROJECT_SOURCE_DIR}/thirdparty/imgui"
        "${PROJECT_SOURCE_DIR}/thirdparty/spdlog/include"
        "${PROJECT_SOURCE_DIR}/thirdparty/stb_image"
        "${PROJECT_SOURCE_DIR}/thirdparty/volk/include"
        "${PROJECT_SOURCE_DIR}/thirdparty/vulkan/include"
)

add_subdirectory(thirdparty)
add_subdirectory(thirdparty/glad)
add_subdirectory(thirdparty/glfw)
add_subdirectory(thirdparty/volk)

set_property(TARGET ${PROJECT_NAME}
    PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        opengl32.lib
        Glad
        GLFW
        Volk
    PUBLIC
        ImGui
)

add_compile_definitions(
    GLFW_INCLUDE_NONE
    _CRT_SECURE_NO_WARNINGS
)


# Copy Limnova.dll to dependent executable directories.
#add_custom_command(TARGET Limnova POST_BUILD COMMAND
#    ${CMAKE_COMMAND} -E copy_if_different
#    ${PROJECT_BINARY_DIR}/Limnova.dll   # File to copy
#    ${CMAKE_BINARY_DIR}/DevTool         # Destination
#)
