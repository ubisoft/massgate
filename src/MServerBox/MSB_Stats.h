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
#ifndef MSB_STATS_H
#define MSB_STATS_H

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MC_EggClockTimer.h"
#include "MT_Thread.h"

#include "MSB_HashTable.h"
#include "MSB_IArray.h"
#include "MSB_MessageHandler.h"
#include "MSB_IStream.h"
#include "MSB_StatsContext.h"
#include "MSB_WriteMessage.h"

#include "MT_Mutex.h"

class MSB_IntegerStat;
class MSB_RawUDPSocket;
class MSB_ReadMessage;
class MSB_StringStat;

class MSB_Stats 
{
public:
	enum {
		GET_STATS_REQ = 1,
		GET_STATS_RSP,
		GET_METADATA_REQ,
		GET_METADATA_RSP
	};

	int32					Start(
								uint16				aPort,
								uint16				aNumberOfAttempts = 1,
								int16				aPortDelta = 0);
	
	void					Stop();


	MSB_StatsContext&		CreateContext(
								const char*			aName);
	bool					HasContext(
								const char*			aName);
	MSB_StatsContext&		FindContext(
								const char*			aName);
	void					GetNames(
								MSB_IArray<MC_String>& someNames);

	static MSB_Stats&		GetInstance();

private:
	class ExportHandler : public MSB_MessageHandler
	{
	public:
							ExportHandler(
								MSB_Stats&			aStatsInstance,
								MSB_Socket_Win&			aSocket);
								
		virtual	bool		OnInit(
								MSB_WriteMessage&	aWriteMessage);
		virtual	bool		OnMessage(
								MSB_ReadMessage&	aReadMessage,
								MSB_WriteMessage&	aWriteMessage);
		virtual	void		OnClose();
		virtual	void		OnError(
								MSB_Error			anError);

	private:
		MT_Mutex			myLock;
		MSB_Stats&			myStats;
		const char*			myIdentifier;

		bool				PrivHandleGetStatsReq(
								MSB_ReadMessage&	aReadMessage,
								MSB_WriteMessage&	aWriteMessage);
		bool				PrivHandleGetMetaDataReq(
								MSB_ReadMessage&	aContextName,
								MSB_WriteMessage&	aWriteMessage);
	};
	
	class Periodical : public MT_Thread
	{
	public:
							Periodical(
								MSB_Stats&			aStats);

	protected:

		void				Run();

	private:
		MSB_Stats&			myStats;
		MC_EggClockTimer	myRecreateTimer;
	};

	typedef MSB_HashTable<const char*, MSB_StatsContext*>	ContextTable;
	typedef MSB_HashTable<const char*, MSB_WriteMessage*>	ContextCache;
	typedef MSB_HashTable<const char*, MSB_WriteMessage*>	MetaCache;

	MT_Mutex				myLock;
	ContextTable			myContextTable;
	ContextCache			myContextCache;
	MetaCache				myMetaCache;
	MSB_RawUDPSocket*		myExportSocket;
	ExportHandler*			myStatHandler;
	Periodical*				myPeriodicalThread;
	
							MSB_Stats();
							~MSB_Stats();

	void					PrivGetStatsResponse(
								const char*			aContextName,
								MSB_WriteMessage&	aWriteMessage);
	void					PrivGetMetaDataResponse(
								const char*			aContextName,
								MSB_WriteMessage&	aWriteMessage);
	void					PrivRecreateStats();
	void					PrivRebuildContext(
								const char*			aKey,
								MSB_StatsContext*	aContext);

	friend class MSB_StatsDeleter;

};

#endif // IS_PC_BUILD

#endif /* MSB_STATS_H */
