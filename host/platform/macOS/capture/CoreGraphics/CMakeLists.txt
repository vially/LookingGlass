cmake_minimum_required(VERSION 3.0)
project(capture_CoreGraphics LANGUAGES C)

find_library(COREFOUNDATION CoreFoundation)
find_library(IOSURF IOSurface)
find_library(COCOA Cocoa)

add_library(capture_CoreGraphics STATIC
	src/coregraphics.m
)

target_link_libraries(capture_CoreGraphics
	${COREFOUNDATION}
	${IOSURF}
	${COCOA}
	lg_common
)

target_include_directories(capture_CoreGraphics
	PRIVATE
		src
)
