option(MG_WITH_CURL "Build cURL" ON)
if (MG_WITH_CURL)
	add_subdirectory(curl)
endif()

option(MG_WITH_ZLIB "Build Zlib" ON)
if (MG_WITH_ZLIB)
	add_subdirectory(zlib)
endif()
