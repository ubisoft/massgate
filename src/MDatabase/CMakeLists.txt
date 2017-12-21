find_package(MySQL REQUIRED)

add_library(MDatabase
	mdb_connectionpinger.cpp
	mdb_connectionpinger.h
	mdb_dataitem.h
	mdb_mysqlconnection.cpp
	mdb_mysqlconnection.h
	mdb_prettyprinter.cpp
	mdb_prettyprinter.h
	mdb_querylogger.cpp
	mdb_querylogger.h
	mdb_stringconversion.cpp
	mdb_stringconversion.h
	mdb_timeable.cpp
	mdb_timeable.h
	stdafx.cpp
	stdafx.h
)

add_library(Lib::MDatabase ALIAS MDatabase)

target_link_libraries(MDatabase
	PRIVATE
		Lib::MCommon2
		Lib::MLogger
		Lib::MNetwork
	PUBLIC
		MySQL::MySQL
)

target_include_directories(MDatabase
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(MDatabase
	PROPERTIES
		CXX_STANDARD 98
		FOLDER "lib"
)
