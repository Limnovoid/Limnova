cmake_minimum_required(VERSION 3.8.2)

project(LimnovaScriptCore
    LANGUAGES CSharp
)

set(CMAKE_CSharp_FLAGS "/langversion:default")

add_library(${PROJECT_NAME} STATIC
    "Source/Input.cs"
    "Source/Main.cs"
    "Source/Math.cs"
    "Source/Native.cs"

    "Source/Scene/Entity.cs"
    "Source/Scene/EntityReference.cs"
    "Source/Scene/Components.cs"

    "Source/Player.cs"
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/LimnovaEditor/Resources/lib/Scripting"
)
