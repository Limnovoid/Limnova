cmake_minimum_required(VERSION 3.21)

project(OrbitalPhysics
	LANGUAGES CXX
	)

add_library(${PROJECT_NAME} STATIC
	"source/OrbitalPhysics.cpp"

	"source/Math/Math.cpp"
	"source/Math/Matrix4.cpp"
	"source/Math/Vector2.cpp"
	"source/Math/Vector3.cpp"
	"source/Math/Vector4.cpp"
	"source/Math/Quaternion.cpp"

	"source/Util/Format.cpp"
	"source/Util/MappedStorage.cpp"
	"source/Util/NTree.cpp"
	"source/Util/Storage.cpp"
	"source/Util/Util.cpp"
	)

set_property(TARGET ${PROJECT_NAME}
	PROPERTY CXX_STANDARD 20
	)

target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		LV_CXX20
		LV_MSVC
	)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		"${PROJECT_SOURCE_DIR}/include"
		"${PROJECT_SOURCE_DIR}/source"
	)

target_precompile_headers(${PROJECT_NAME}
	PRIVATE
		<cmath>
		<limits>
		<utility>
		<iostream>
		<vector>
		<unordered_set>
	)

set_property(TARGET ${PROJECT_NAME}
	PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
	)

# ----------------------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------------------

project(OrbitalPhysicsTest
	LANGUAGES CXX
	)

add_executable(${PROJECT_NAME}
	"source/Test/Main.cpp"
	)

set_property(TARGET ${PROJECT_NAME}
	PROPERTY CXX_STANDARD 20
	)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		OrbitalPhysics
	)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		"${PROJECT_SOURCE_DIR}/include"
	)

target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		LV_CXX20
		LV_MSVC
	)
