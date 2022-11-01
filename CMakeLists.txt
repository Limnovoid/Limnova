cmake_minimum_required(VERSION 3.21)

project(LimnovaEngine 
    LANGUAGES CXX
)


# Definitions
add_compile_definitions(
    $<${MSVC}:LV_PLATFORM_WINDOWS>

    $<$<CONFIG:Debug>:LV_DEBUG>
    $<$<CONFIG:Release>:LV_RELEASE>
)

# Projects.
add_subdirectory(Limnova)
add_subdirectory(DevTool)
