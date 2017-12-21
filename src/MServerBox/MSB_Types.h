// Massgate
// Copyright (C) 2017 Ubisoft Entertainment
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#ifndef MSB_TYPES_H
#define MSB_TYPES_H

typedef uint16 MSB_DelimiterType;

typedef enum
{
	MSB_CONSOLE_CMD_HELLO_REQ = 1, 
	MSB_CONSOLE_CMD_HELLO_RSP, 
	MSB_CONSOLE_CMD_SHOW_WRITE_BUFFER_IN_POOL_REQ,
	MSB_CONSOLE_CMD_SHOW_WRITE_BUFFER_IN_POOL_RSP,
	MSB_CONSOLE_CMD_FREE_UNUSED_WRITE_BUFFERS_REQ,
	MSB_CONSOLE_CMD_FREE_UNUSED_WRITE_BUFFERS_RSP, 
	MSB_CONSOLE_CMD_LIST_STATS_REQ,
	MSB_CONSOLE_CMD_LIST_STATS_RSP,
	MSB_CONSOLE_CMD_STAT_REQ,
	MSB_CONSOLE_CMD_STAT_RSP
} MSB_ConsoleDelimiters;

#define MSB_PACKET_HEADER_LENGTH sizeof(uint16)
#define	MSB_PACKET_HEADER_FLAGS sizeof(uint8)
#define MSB_PACKET_HEADER_TOTSIZE (MSB_PACKET_HEADER_LENGTH + MSB_PACKET_HEADER_FLAGS)
#define MSB_MIN_VALID_PACKET_SIZE (MSB_PACKET_HEADER_TOTSIZE + sizeof(MSB_DelimiterType))

typedef enum 
{
	MSB_TT_DELIMITER = 1,
	MSB_TT_BOOL,
	MSB_TT_INT8, 
	MSB_TT_UINT8,
	MSB_TT_INT16,
	MSB_TT_UINT16,
	MSB_TT_INT32,
	MSB_TT_UINT32,
	MSB_TT_INT64,
	MSB_TT_UINT64,
	MSB_TT_FLOAT32,
	MSB_TT_FLOAT64,
	MSB_TT_STRING,
	MSB_TT_LOCSTRING,
	MSB_TT_RAWDATA,
	MSB_TT_VECTOR3F32,
	MSB_TT_VECTOR2F32,
	MSB_TT_ANGLE32,
	MSB_TT_BITVECTOR,
	MSB_TT_CRYPTDATA,
	MSB_TT_CRYPTSTRING,
	MSB_TT_CRYPTLOCSTRING
} MSB_TypecheckType;

typedef enum 
{
	MSB_PACKET_FLAGS_TYPECHECKED	= 0x01,
	MSB_PACKET_FLAGS_SYSTEM			= 0x02
} MSB_PacketFlags;

typedef enum 
{
	MSB_TS_FALSE = 0,
	MSB_TS_TRUE, 
	MSB_TS_UNKNOWN
} MSB_TriState; 

typedef enum
{
	MSB_SYSTEM_MESSAGE_AUTH_TOKEN			= 1,
	MSB_SYSTEM_MESSAGE_AUTH_ACK,
	MSB_SYSTEM_MESSAGE_TIME_SYNC_REQ,
	MSB_SYSTEM_MESSAGE_TIME_SYNC_RSP,
	MSB_SYSTEM_MESSAGE_UDP_PORT_CHECK,
	MSB_SYSTEM_MESSAGE_UDP_PORT_OK
} MSB_SystemMessage;

#define MSB_WRITEBUFFER_DEFAULT_SIZE 4000
#define MSB_MAX_DELIMITER_DATA_SIZE (0x7FFF - sizeof(MSB_DelimiterType))

typedef uint32 MSB_UDPClientID;

#define	MSB_LISTENER_BACKLOG INT_MAX

typedef enum
{
	MSB_NO_ERROR = 0,

	MSB_ERROR_INVALID_DATA,			//Internal interpretation error
	MSB_ERROR_INVALID_SYSTEM_MSG,	//Data error on a system message
	MSB_ERROR_HOST_NOT_FOUND,		//The resolver failed or Connect() failed
	MSB_ERROR_SYSTEM,				//Some system call failed, like socket(), connect(), bind() etc.
	MSB_ERROR_CONNECTION,			//Socket error, general error, Handler::OnError usually()
	MSB_ERROR_RECONNECT_FAILED,		//Reconnect on a already failed connection failed
	MSB_ERROR_OUTGOING_BUFFER_FULL	//
} MSB_Error;

const char*		MSB_GetErrorString(
					const MSB_Error			anError);


#if IS_ENDIAN(ENDIAN_LITTLE)

#define MSB_SWAP_TO_BIG_ENDIAN(X)\
	MC_Endian::Swap(X)

#define MSB_SWAP_TO_NATIVE(X)\
	MC_Endian::Swap(X)

#else 

#define MSB_SWAP_TO_BIG_ENDIAN(VALUES) (VALUES)
#define MSB_SWAP_TO_NATIVE(VALUES) (VALUES)

#endif

//Network types
typedef struct in_addr		MSB_IP;
typedef uint16				MSB_Port;
typedef enum
{
	MSB_Proto_ERROR = 0,
	MSB_Proto_TCP = 6,
	MSB_Proto_UDP = 17
} MSB_Proto;

class MSB_PortRange
{
public:
							MSB_PortRange(
								MSB_Port	aStartPort,  //First port to use
								MSB_Port	aEndPort,
								int32 		aDelta = 1);  //Next port to use = aStartPort + aDelta
	bool					HasNext() const;
	const MSB_Port&			GetNext();

	const MSB_Port&			GetCurrent() const; //Need to call GetNextPort() once first
	void					Restart();

private:
	MSB_Port				myStartPort;
	MSB_Port				myEndPort;
	int32					myDelta;
	MSB_Port				myCurrentPort;
};

#endif /* MSB_TYPES_H */
