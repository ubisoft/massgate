cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

project(Zlib LANGUAGES C)

add_library(Zlib
	src/adler32.cpp
	src/compress.cpp
	src/crc32.cpp
	src/crc32.h
	src/deflate.cpp
	src/deflate.h
	src/inffast.cpp
	src/inffast.h
	src/inffixed.h
	src/inflate.cpp
	src/inflate.h
	src/inftrees.cpp
	src/inftrees.h
	src/trees.cpp
	src/trees.h
	src/uncompr.cpp
	src/zconf.h
	src/zlib.h
	src/zutil.cpp
	src/zutil.h
)

add_library(Zlib::Zlib ALIAS Zlib)

set_target_properties(Zlib
	PROPERTIES
		DEBUG_POSTFIX d
		PUBLIC_HEADER "${sources}"
)

target_include_directories(Zlib PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)

set_target_properties(Zlib
	PROPERTIES
		FOLDER "3rdparty"
)
