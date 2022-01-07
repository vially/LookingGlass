cmake_minimum_required(VERSION 3.0)
project(platform_macOS LANGUAGES C)

include_directories(
	${PROJECT_SOURCE_DIR}/include
)

add_library(platform_macOS STATIC
	src/platform.c
)

add_subdirectory("capture")

target_link_libraries(platform_macOS
	m
	capture
	pthread
)

target_include_directories(platform_macOS
	PRIVATE
		src
)
