project(DevTool)

add_executable(${PROJECT_NAME} "DevTool.cpp")


target_include_directories(${PROJECT_NAME}
	PRIVATE
		"${CMAKE_SOURCE_DIR}/Limnova"
		"${CMAKE_SOURCE_DIR}/Limnova/src"
		"${CMAKE_SOURCE_DIR}/Limnova/thirdparty/spdlog/include"
)


set_property(TARGET ${PROJECT_NAME}
	PROPERTY CXX_STANDARD 17	
)


target_link_libraries(${PROJECT_NAME}
	PRIVATE
		Limnova
)


target_precompile_headers(${PROJECT_NAME} REUSE_FROM Limnova)