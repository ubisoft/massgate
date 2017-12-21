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
#ifndef MSB_MESSAGEHANDLER_H_
#define MSB_MESSAGEHANDLER_H_

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_Types.h"

class MSB_Socket_Win;
class MSB_ReadMessage;
class MSB_WriteMessage;

class MSB_MessageHandler
{
public:
					MSB_MessageHandler(MSB_Socket_Win& aSocket);
	virtual			~MSB_MessageHandler();

	virtual	bool	OnInit(
						MSB_WriteMessage&	aWriteMessage) = 0;
	virtual	bool	OnMessage(
						MSB_ReadMessage&	aReadMessage,
						MSB_WriteMessage&	aWriteMessage) = 0;
	virtual	void	OnClose() = 0;
	virtual	void	OnError(
						MSB_Error			anError) = 0;

	void			Close();
	void			Send(
						MSB_WriteMessage&	aWriteMessage);
	const char*		GetRemoteAddressString();
	uint32			GetRemoteIp(); 
	

	// Unless you know what you're doing, leave this alone
	virtual bool	OnSystemMessage(
						MSB_ReadMessage&	aReadMessage,
						MSB_WriteMessage&	aWriteMessage,
						bool&				aUsed) { return true; }

private:
	MSB_Socket_Win&		mySocket;

	virtual void	DoSocketSend(
						MSB_Socket_Win&		aSocket,
						MSB_WriteMessage&	aWriteMessage);
};

#else // IS_PC_BUILD

class MSB_MessageHandler
{
public:
	virtual			~MSB_MessageHandler() {}

	virtual	bool	OnInit(
						MSB_WriteMessage&	aWriteMessage) = 0;

	virtual	bool	OnMessage(
						MSB_ReadMessage&	aReadMessage,
						MSB_WriteMessage&	aWriteMessage) = 0;

	virtual	void	OnClose() = 0;

	virtual	void	OnError(
						MSB_Error			anError) = 0;

	// Unless you know what you're doing, leave this alone
	virtual bool	OnSystemMessage(
						MSB_ReadMessage&	aReadMessage,
						MSB_WriteMessage&	aWriteMessage,
						bool&				aUsed) { return true; }

private:

};

#endif // IS_PC_BUILD

#endif //MSB_MESSAGEHANDLER_H_

