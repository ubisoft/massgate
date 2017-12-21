cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

project(Curl LANGUAGES C VERSION 1.297)

add_library(Curl
	src/base64.c
	src/base64.h
	src/config-win32.h
	src/connect.c
	src/connect.h
	src/content_encoding.c
	src/content_encoding.h
	src/cookie.c
	src/cookie.h
	src/curl.h
	src/curlver.h
	src/curlx.h
	src/dict.c
	src/dict.h
	src/easy.c
	src/easy.h
	src/easyif.h
	src/escape.c
	src/escape.h
	src/file.c
	src/file.h
	src/formdata.c
	src/formdata.h
	src/ftp.c
	src/ftp.h
	src/getinfo.c
	src/getinfo.h
	src/gtls.c
	src/gtls.h
	src/hash.c
	src/hash.h
	src/hostares.c
	src/hostasyn.c
	src/hostip.c
	src/hostip.h
	src/hostip4.c
	src/hostsyn.c
	src/hostthre.c
	src/http.c
	src/http.h
	src/http_chunks.c
	src/http_chunks.h
	src/http_negotiate.c
	src/http_negotiate.h
	src/if2ip.c
	src/if2ip.h
	src/inet_ntoa_r.h
	src/inet_ntop.c
	src/inet_ntop.h
	src/inet_pton.c
	src/inet_pton.h
	src/krb4.h
	src/ldap.h
	src/llist.c
	src/llist.h
	src/md5.c
	src/md5.h
	src/memdebug.c
	src/memdebug.h
	src/memory.h
	src/mprintf.c
	src/mprintf.h
	src/multi.c
	src/multi.h
	src/multiif.h
	src/parsedate.c
	src/parsedate.h
	src/progress.c
	src/progress.h
	src/security.c
	src/select.c
	src/select.h
	src/sendf.c
	src/sendf.h
	src/setup.h
	src/share.c
	src/share.h
	src/sockaddr.h
	src/speedcheck.c
	src/speedcheck.h
	src/splay.c
	src/splay.h
	src/stdcheaders.h
	src/strequal.c
	src/strequal.h
	src/strerror.c
	src/strerror.h
	src/strtok.c
	src/strtok.h
	src/strtoofft.c
	src/strtoofft.h
	src/timeval.c
	src/timeval.h
	src/transfer.c
	src/transfer.h
	src/types.h
	src/url.c
	src/url.h
	src/urldata.h
	src/version.c
)

add_library(Curl::Curl ALIAS Curl)

target_include_directories(Curl PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)

set_target_properties(Curl
	PROPERTIES
		FOLDER "3rdparty"
)
