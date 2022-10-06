project(DevTool)

add_executable(${PROJECT_NAME} "DevTool.cpp")


target_include_directories(${PROJECT_NAME}
	PRIVATE
		"${CMAKE_SOURCE_DIR}/Limnova"
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		Limnova
)