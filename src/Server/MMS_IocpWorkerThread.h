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
#ifndef MMS_IocpWORKERTHREAD____H__
#define MMS_IocpWORKERTHREAD____H__

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <WinSock2.h>
#include <mswsock.h>

#include "MN_ReadMessage.h"
#include "MN_WriteMessage.h"
#include "MDB_MySqlConnection.h"

#include "MT_Thread.h"

#include "MMS_IOCPServer.h"

#include "MMG_ProtocolDelimiters.h"

class MMS_IocpWorkerThread : public MT_Thread
{
public:

	class FederatedHandler
	{
	public:
		virtual bool HandleMessage(MMG_ProtocolDelimiters::Delimiter theDelimiter, MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer) = 0;
	};


	MMS_IocpWorkerThread( MMS_IocpServer* aServer);
	virtual ~MMS_IocpWorkerThread();

	virtual void Run();

	virtual bool HandleMessage(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer) = 0;

	// This function WILL CLEAR the recipients that it could successfully send to!
	// return false means input message failure or other lethal fatality. return true means that message was sent, but not to the
	// ones still remaining in theRecipients.
	bool	MulticastMessage(MN_WriteMessage& theOutgoingMessage, const MC_GrowingArray<MMS_IocpServer::SocketContext*>& theRecipients);
	bool	SendMessageTo(MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* theRecipient);
	void	CloseSocket(MMS_IocpServer::SocketContext* &aContext);
	
	int		GetThreadNum() { return myThreadNum; }

	virtual void OnSocketClosed(MMS_IocpServer::SocketContext* aContext);
protected:

	void Heartbeat();

	void RealRun();

	void PostMortemDebug(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer); 
	virtual void ServicePostMortemDebug(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer) { }

	virtual void OnReadyToStart();
	virtual void OnSocketConnected(MMS_IocpServer::SocketContext* aContext);
	virtual void OnIdleTimeout() { }

	virtual DWORD GetTimeoutTime() { return 100; }

	virtual bool HandleIncomingData(MMS_IocpServer::SocketContext* aContext);

	virtual void CheckSanity() { } 

	MMS_IocpServer* myServer;

private:
	bool		PostReadRequest(MMS_IocpServer::SocketContext* aContext);
	bool		PostWriteRequest(MMS_IocpServer::SocketContext* aContext);

	HANDLE		myCompletionPort;
	int			myThreadNum;

	MC_GrowingArray<MMS_IocpServer::SocketContext*> myTempContexts;
	MC_HybridArray<MMS_IocpServer::SocketContext*, 1024> mySocketsToCloseFromThisThread;
};

#endif
