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
//mn_clientconnectionfactory.h
//abstract interface to a client connection factory

#ifndef MN_CLIENTCONNECTIONFACTORY_H
#define MN_CLIENTCONNECTIONFACTORY_H

class MN_Connection;

class MN_ClientConnectionFactory
{
public:

	//destructor
	virtual ~MN_ClientConnectionFactory();

	//is a connection attempt in progress?
	virtual bool IsAttemptingConnection() = 0;

	//init a connection
	virtual bool CreateConnection(const char* anAddress, const unsigned short aPort) = 0;

	//await completion of the connection process
	virtual bool AwaitConnection(MN_Connection** aConnectionTarget) = 0;

	//cancel a connection in progress
	virtual void CancelConnection() = 0;

protected:

	//constructor
	MN_ClientConnectionFactory();
};

#endif