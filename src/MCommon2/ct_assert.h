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
#pragma once 
#ifndef CT_ASSERT___H_
#define CT_ASSERT___H_

// compile-time asserts. use like this:
// ...
// ct_assert<sizeof(TheDataType) == 4>();
// ct_assert<false>(); // Implement this!
//

template<bool> struct ct_assert;
template<> struct ct_assert<true> {} ;

#ifdef _RELEASE_
#define CT_ASSERT(x) do{;}while(false)
#else
#define CT_ASSERT(x) do{ct_assert<(x)>();}while(false)
#endif

#endif