find_package(Curl)

add_library(MNetwork
	MN_ClientConnectionFactory.cpp
	MN_ClientConnectionFactory.h
	MN_Connection.cpp
	MN_Connection.h
	MN_ConnectionErrorType.h
	mn_dnscache.cpp
	mn_dnscache.h
	MN_FlagSet.cpp
	MN_FlagSet.h
	MN_Format.cpp
	MN_Format.h
	mn_icmpsocket.cpp
	mn_icmpsocket.h
	MN_InducedLatencyHandler.cpp
	MN_InducedLatencyHandler.h
	mn_inethandler.cpp
	mn_inethandler.h
	MN_IWriteableDataStream.h	
	MN_LoopbackConnection.h
	MN_Message.cpp
	MN_Message.h
	mn_netrequester.cpp
	mn_netrequester.h
	mn_netstats.cpp
	mn_netstats.h
	MN_Packet.cpp
	MN_Packet.h
	MN_Platform.h
	MN_ReadMessage.cpp
	MN_ReadMessage.h
	MN_Resolver.cpp
	MN_Resolver.h
	MN_SocketUtils.cpp
	MN_SocketUtils.h
	MN_TcpClientConnectionFactory.cpp
	MN_TcpClientConnectionFactory.h
	MN_TcpConnection.cpp
	MN_TcpConnection.h
	MN_TcpSocket.cpp
	MN_TcpSocket.h
	MN_UdpSocket.cpp
	MN_UdpSocket.h
	MN_WinsockNet.cpp
	MN_WinsockNet.h
	MN_WriteMessage.cpp
	MN_WriteMessage.h
	StdAfx.cpp
	StdAfx.h
)

add_library(Lib::MNetwork ALIAS MNetwork)

target_link_libraries(MNetwork
	PRIVATE
		Lib::MCommon2
		iphlpapi
		ws2_32
	PUBLIC
		Curl::Curl
)

target_include_directories(MNetwork
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(MNetwork
	PROPERTIES
		CXX_STANDARD 98
		FOLDER "lib"
)