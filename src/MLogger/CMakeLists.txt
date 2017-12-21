add_library(MLogger
	ML_Backend.cpp
	ML_Backend.h
	ML_ConsoleBackend.cpp
	ML_ConsoleBackend.h
	ML_FileBackend.cpp
	ML_FileBackend.h
	ML_Logger.cpp
	ML_Logger.h
	stdafx.cpp
	stdafx.h
)

add_library(Lib::MLogger ALIAS MLogger)

target_link_libraries(MLogger
	PRIVATE
		Lib::MCommon2
)

target_include_directories(MLogger
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(MLogger
	PROPERTIES
		CXX_STANDARD 98
		FOLDER "lib"
)
