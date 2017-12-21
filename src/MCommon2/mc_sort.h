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
#ifndef MC_SORT_H
#define MC_SORT_H


void* SortLinkedList(void* aFirstPtr, void* aNextPtr, const int aListLength, void* aCompareDword); // returns a ptr to the first item in the sorted array (all next ptrs are updated)
void SortPtrArray(void** anArray, const int aListLength, void* aCompareDword, void** anResultArray = NULL);
void SortArray(void* anArray, const int aStructSize, const int aListLength, void* aCompareDword, void* anResultArray); // warning: does not work well with classes, since copy constructors are ignored

void* SortDescendingLinkedList(void* aFirstPtr, void* aNextPtr, const int aListLength, void* aCompareDword); // returns a ptr to the first item in the sorted array (all next ptrs are updated)
void SortDescendingPtrArray(void** anArray, const int aListLength, void* aCompareDword, void** anResultArray = NULL);
void SortDescendingArray(void* anArray, const int aStructSize, const int aListLength, void* aCompareDword, void* anResultArray); // warning: does not work well with classes, since copy constructors are ignored

#endif // MC_SORT_H
