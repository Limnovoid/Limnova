cmake_minimum_required(VERSION 3.8.2)

project(LimnovaScriptCore
    LANGUAGES CSharp
)

add_library(${PROJECT_NAME} STATIC
    "Source/Main.cs"
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/LimnovaEditor/Resources/Scripts"
        OUTPUT_NAME ${PROJECT_NAME}
)
