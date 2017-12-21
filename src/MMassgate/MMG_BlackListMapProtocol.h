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
#ifndef MMG_BLACKLISTMAPPROTCOL_H
#define MMG_BLACKLISTMAPPROTCOL_H

class MN_ReadMessage; 
class MN_WriteMessage; 

class MMG_BlackListMapProtocol
{
public:

	// from client to massgate 
	class BlackListReq
	{
	public: 
		void	ToStream(
					MN_WriteMessage&	theStream);
		bool	FromStream(
					MN_ReadMessage&		theStream);

		unsigned __int64	myMapHash; 
	}; 

	// from massgate to ds
	class RemoveMapReq
	{
	public: 
		void	ToStream(
					MN_WriteMessage&	theStream);
		bool	FromStream(
					MN_ReadMessage&		theStream);

		MC_HybridArray<unsigned __int64, 32>	myMapHashes; 
	};
};

#endif // MMG_BLACKLISTMAPPROTCOL_H