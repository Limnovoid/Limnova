project(LimnovaEditor)

add_executable(${PROJECT_NAME}
    "LimnovaEditorApp.cpp"
    "EditorLayer.cpp"

    "Panels/SceneHierarchyPanel.cpp"
)


set_property(TARGET ${PROJECT_NAME}
    PROPERTY CXX_STANDARD 20
)


target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${PROJECT_SOURCE_DIR}"

        "${CMAKE_SOURCE_DIR}/Limnova"
        "${CMAKE_SOURCE_DIR}/Limnova/src"
        "${CMAKE_SOURCE_DIR}/Limnova/thirdparty"
        "${CMAKE_SOURCE_DIR}/Limnova/thirdparty/entt/include"
        "${CMAKE_SOURCE_DIR}/Limnova/thirdparty/glm"
        "${CMAKE_SOURCE_DIR}/Limnova/thirdparty/spdlog/include"
)


set_property(TARGET ${PROJECT_NAME}
    PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Limnova
)

add_compile_definitions(
    GLFW_INCLUDE_NONE
    _CRT_SECURE_NO_WARNINGS
)

target_precompile_headers(${PROJECT_NAME} REUSE_FROM Limnova)
