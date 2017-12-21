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
#ifndef MSB_RESOLVER_WIN_H
#define MSB_RESOLVER_WIN_H

#include "MC_Base.h"
#ifdef IS_PC_BUILD

// #include "MSB_BoundQueue.h"
#include "MSB_HybridArray.h"

#include "MT_Event.h"
#include "MT_Mutex.h"
#include "MT_Thread.h"

class MSB_Resolver
{
public:
	class Callback {
	public:
		virtual void		OnResolveComplete(
								const char*				aHostname,
								struct sockaddr*		anAddress,
								size_t					anAddressLength) = 0;
	};

	void					Resolve(
								const char*				aHostname,
								Callback*				aCallback);

	static MSB_Resolver&	GetInstance() { return ourInstance; }

protected:

	void					Run();

private:
	class ResolveThread : public MT_Thread
	{
	public:
		struct Request {
			const char*		myHostname;
			Callback*		myCallback;

			bool operator == (const Request& aRhs) const
			{
				return strcmp(myHostname, aRhs.myHostname) == 0 && myCallback == aRhs.myCallback;
			}

		};

		ResolveThread();
		virtual				~ResolveThread();

		void				Resolve(
								const char*				aHostname,
								Callback*				aCallback);

	protected:
		MT_Mutex			myLock;
		MT_Event			myQueueNotEmpty;
		MSB_HybridArray<Request, 10>	myRequestQueue;

		void				Run();
	};

	static MSB_Resolver		ourInstance;
	ResolveThread*			myResolver;
	MT_Mutex				myLock;

	MSB_Resolver();
	virtual					~MSB_Resolver();

};

#endif // IS_PC_BUILD

#endif /* MSB_RESOLVER_WIN_H */