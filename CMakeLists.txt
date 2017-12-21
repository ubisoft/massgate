cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(INSERT CMAKE_PREFIX_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(Massgate VERSION 0.0.0 LANGUAGES CXX)

macro(find_package)
	if (NOT TARGET "${ARGV0}")
		_find_package(${ARGV})
	endif()
endmacro()

add_subdirectory(3rdparty)
add_subdirectory(src)
