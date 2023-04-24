/*****************************************************************************
* (c) Copyright 2012-2013 F.Hoffmann-La Roche AG                             *
* Contact: bioinfoc@bioinfoc.ch, Detlef.Wolf@Roche.com.                      *
*                                                                            *
* This file is part of BIOINFO-C. BIOINFO-C is free software: you can        *
* redistribute it and/or modify it under the terms of the GNU Lesser         *
* General Public License as published by the Free Software Foundation,       *
* either version 3 of the License, or (at your option) any later version.    *
*                                                                            *
* BIOINFO-C is distributed in the hope that it will be useful, but           *
* WITHOUT ANY WARRANTY; without even the implied warranty of                 *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU          *
* Lesser General Public License for more details. You should have            *
* received a copy of the GNU Lesser General Public License along with        *
* BIOINFO-C. If not, see <http://www.gnu.org/licenses/>.                     *
*****************************************************************************/
/** @file hash.h
    @brief Hash table routines.
    Module prefix hash_
*/
#ifndef HASH_H
#define HASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "avlTree.h"

/// the HashSubTable structure
typedef struct {
  AvlTree tablePt; //!< the AvlTree
}HashSubTable;

/// the HashTable object
typedef struct _HashTableStruct_ {
  int tableSize; //!< size of the table
  HashSubTable *subTable; //!< pointer to sub table
  int numElements; //!< number of elements
  int (*tableSelectorFunction) (void *,int); //!< function to be used to select table
  int (*intraTableOrderFunction) (void *,void *); //!< function to be used to order the tables
  void (*cleanFunction) (void *); //!< function to be used to clean the table
}*HashTable;

/// the HashIter object
typedef struct _HashIteratorStruct_ {
  HashTable table; //!< the table to iterate over
  AvlIterator currIterator; //!< the underlying AvIterator
  int currTableIndex; //!< index into the table
}*HashIterator;

extern HashTable hash_tableCreate (int tableSize,
                                   int (*tableSelectorFunction)(void *,int),
                                   int (*intraTableOrderFunction)(void *,void *),
                                   void (*cleanFunction)(void *));
extern void hash_tableDestroyFunc (HashTable this1);
/// use this macro, do not use hash_tableDestroyFunc()
#define hash_tableDestroy(x) ((x) ? hash_tableDestroyFunc(x),x=NULL,1:0)
extern int hash_tableInsert (HashTable this1,void *valueInsert,
                             void (*modifyDuplicate)(void *,void *));
extern int hash_tableFind (HashTable this1,void *valueToFind,void **valueFound);
extern int hash_tableDelete (HashTable this1,void *valueToDelete);
extern void hash_tablePrintStats (HashTable this1,FILE *outFile);
extern void hash_tableApplyFunc (HashTable this1,void *applyFunc(),
                                 int nargs,...);
extern int hash_tableNumElem (HashTable this1);

extern HashIterator hash_itCreate (HashTable table);
extern void hash_itDestroyFunc (HashIterator iterator);
/// use this macro, do not use hash_itDestroyFunc()
#define hash_itDestroy(x) ((x) ? hash_itDestroyFunc(x),x=NULL,1 : 0)
extern void *hash_itNextValue (HashIterator iterator);

#ifdef __cplusplus
}
#endif

#endif
