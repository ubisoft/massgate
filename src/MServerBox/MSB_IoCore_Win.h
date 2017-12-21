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
#ifndef MSB_IOCORE_H_
#define MSB_IOCORE_H_

#include "MC_Base.h"

#if IS_PC_BUILD

#include "MSB_HybridArray.h"

#include "MC_EggClockTimer.h"
#include "MT_Thread.h"
#include "MT_Mutex.h"

#include <winsock2.h> 

class MSB_ComboSocket;
class MSB_Socket_Win;
class MSB_TCPSocket;
class MSB_WorkerThread;

class MSB_IoCore
{
public:
	typedef struct {
		WSAOVERLAPPED	overlapped;
		bool			write;
	}OverlappedHeader;

	int32				Start(
							uint32				aMaxThreadCount = UINT_MAX,
							uint32				aNumThreadsPerCPU = 8);

	void				Stop();

	void				WaitCompletion(
							int					aTimeout = INFINITE);

	int32				RegisterSocket(
							MSB_Socket_Win*		aSocket);

	void				UnregisterSocket(
							MSB_Socket_Win*		aSocket);

	void				StartGracePeriod(
							MSB_TCPSocket*		aSocket);
	void				StopGracePeriod(
							MSB_TCPSocket*		aSocket);

	void				StartUdpPortCheck(
							MSB_ComboSocket*	aSocket,
							uint32				aMaxAttempts,
							const struct sockaddr_in&	aTargetAddress);
	void				StopUdpPortCheck(
							MSB_ComboSocket*	aSocket);

	uint32				GetNumWorkerThreads() const { return myWorkerCount; };

	static MSB_IoCore&	GetInstance();

private:
	class Periodical : public MT_Thread
	{
	public:
						Periodical(
							MSB_IoCore&		aServer);
		
		void			StartGracePeriod(
							MSB_TCPSocket*		aConnection);
		void			StopGracePeriod(
							MSB_TCPSocket*		aSocket);
		void			StartUdpCheck(
							MSB_ComboSocket*	aSocket,
							uint32				aMaxAttempts,
							const struct sockaddr_in&	aTargetAddress);
		void			StopUdpCheck(
							MSB_ComboSocket*	aSocket);
		
	protected:

		void			Run();

	private:
		class GraceEntry 
		{
		public:
			bool operator == (const GraceEntry& aRhs)
			{
				return mySocket == aRhs.mySocket; 
			}

			uint32			myStartTime;
			MSB_TCPSocket*	mySocket;
		};

		class UdpCheckEntry
		{
		public:
			bool operator == (const UdpCheckEntry& aRhs)
			{
				return mySocket == aRhs.mySocket; 
			}			

			uint32				myAttempts;
			uint32				myLastSend;
			uint32				myMaxAttempts;
			MSB_ComboSocket*	mySocket;
			struct sockaddr_in	myTargetAddress;
		};

		MSB_IoCore&			myIocp;
		MT_Mutex			myLock;
 		MC_EggClockTimer	myPurgeTimeout;
 		MSB_HybridArray<GraceEntry, 100>	myNewConnections;
		MSB_HybridArray<UdpCheckEntry, 100>	myUdpConnections;

		void			PrivPurgeStaleConnections();
		void			PrivCheckUdpSockets();
	};

	static MSB_IoCore	ourServer;

	Periodical*			myPeriodical;
	HANDLE				myServerHandle;
	uint32				myWorkerCount;
	uint32				myConnectGraceTime;
	bool				myIsStarted;
	bool				myIsStopped;
	
	MSB_WorkerThread**	myWorkerList;
	

						MSB_IoCore();
						~MSB_IoCore();

	void				PrivAddConsoleCommands();
};

#endif // IS_PC_BUILD

#endif /* MSB_IOCORE_H_ */
