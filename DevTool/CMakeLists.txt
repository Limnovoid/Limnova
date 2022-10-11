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

# Copy Limnova.dll to the DevTool output directory before linking.
#add_custom_command(TARGET DevTool PRE_LINK COMMAND
#	${CMAKE_COMMAND} -E copy_if_different
#	${CMAKE_BINARY_DIR}/Limnova/Limnova.dll
#	${PROJECT_BINARY_DIR}
#	)