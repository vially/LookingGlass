cmake_minimum_required(VERSION 3.0)
project(capture_CoreGraphics LANGUAGES C)

add_library(capture_CoreGraphics STATIC
	src/coregraphics.c
)

target_link_libraries(capture_CoreGraphics
	lg_common
)

target_include_directories(capture_CoreGraphics
	PRIVATE
		src
)
