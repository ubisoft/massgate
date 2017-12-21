# FindMySQL
# ---------
#
# Find MySQL library
#
# Imported Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the following :prop_tgt:`IMPORTED` targets:
#
# ``MySQL::MySQL``
#   The MySQL imported library.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module will set the following variables in your project:
#
# ``MYSQL_FOUND``
#   System has the MySQL library.
# ``MYSQL_INCLUDE_DIR``
#   The MySQL include directory.
# ``MYSQL_LIBRARY``
#   The MySQL library.
# ``MYSQL_LIBRARIES.``
#   MySQL libraries to be linked.
# ``MYSQL_VERSION``
#   The MySQL version.

find_path(MYSQL_INCLUDE_DIR
	NAMES
		mysql.h
	PATH
		"$ENV{SYSTEMDRIVE}/MySQL/*/include"
		"$ENV{MYSQL_HOME}/include"
		"$ENV{PROGRAMFILES} (x86)/MySQL/*/include"
		"$ENV{PROGRAMFILES}/MySQL/*/include"
)

if(DEFINED MYSQL_LIBRARIES AND NOT DEFINED MYSQL_LIBRARY)
	set(MYSQL_LIBRARY ${MYSQL_LIBRARIES})
else()
	find_library(MYSQL_LIBRARY
		NAMES
			libmysql
		PATHS
			"$ENV{MYSQL_HOME}/lib"
			"$ENV{PROGRAMFILES} (x86)/MySQL/*/include"
			"$ENV{PROGRAMFILES}/MySQL/*/lib"
			"$ENV{SYSTEMDRIVE}/MySQL/*/lib"
	)	
endif()

if(MYSQL_INCLUDE_DIR AND EXISTS "${MYSQL_INCLUDE_DIR}/mysql_version.h")
	file(STRINGS "${MYSQL_INCLUDE_DIR}/mysql_version.h" _mysql_version_str
		REGEX "^#define[\t ]+MYSQL_SERVER_VERSION[\t ]+\".*\"")
	string(REGEX REPLACE "^#define[\t ]+MYSQL_SERVER_VERSION[\t ]+\"([^\"]*)\".*" "\\1"
		MYSQL_VERSION "${_mysql_version_str}")
	unset(_mysql_version_str)
endif()

set(MYSQL_LIBRARIES ${MYSQL_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQL
	REQUIRED_VARS
		MYSQL_LIBRARY
		MYSQL_INCLUDE_DIR
	VERSION_VAR
		MYSQL_VERSION
)

mark_as_advanced(MYSQL_INCLUDE_DIR MYSQL_LIBRARY)

if (MYSQL_FOUND AND NOT TARGET MySQL::MySQL)
	add_library(MySQL::MySQL UNKNOWN IMPORTED)
	set_target_properties(MySQL::MySQL
		PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${MYSQL_INCLUDE_DIR}"
			IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
			IMPORTED_LOCATION "${MYSQL_LIBRARY}"
	)
endif()
