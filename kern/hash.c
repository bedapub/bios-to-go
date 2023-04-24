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
/** @file hash.c
    @brief Hash table routines.
    Module prefix hash_
*/
#include "log.h"
#include "hlrmisc.h"
#include "hash.h"

/// default value for hash table size
#define DEFAULT_HASH_SIZE 4096

HashTable hash_tableCreate (int tableSize,
                            int (*tableSelectorFunction)(void *,int),
                            int (*intraTableOrderFunction)(void *,void *),
                            void (*cleanFunction)(void *)) {
  /**
     Creates a hash table, registers pointers to cleanFunction(),
     tableSelectorFunction() and intraTableOrderFunction() and
     returns a pointer to the HashTable structure.<br>
     Postcondition: User is responsible to free the memory allocated
     by calling the function hash_tableDestroy().
     @param[in] tableSize -  an integer specifying the number of subtables,
                             value of -1 assignes a default value of 4096
     @param[in] tableSelectorFunction - subtable selector function
     @param[in] intraTableOrderFunction - compare function to order two entries
                                          in a subtable
     @param[in] cleanFunction - clean function to describe how memory allocated
                                to each entry should be freed
     @return new HashTable
  */
  int i;
  HashTable newTable = (HashTable)hlr_malloc (sizeof (struct _HashTableStruct_));
  if (tableSize < 1)
    newTable->tableSize = DEFAULT_HASH_SIZE;
  else
    newTable->tableSize = tableSize;
  if (tableSelectorFunction == NULL)
    die ("Mandatory to supply TableSelectorFunction in hash_create() arguments");
  newTable->tableSelectorFunction = tableSelectorFunction;
  if (intraTableOrderFunction == NULL)
    die ("Mandatory to supply IntraTableOrderFunction in hash_create() arguments");
  newTable->intraTableOrderFunction = intraTableOrderFunction;
  if (cleanFunction == NULL)
    die ("Mandatory to supply cleanFunction in hash_create() arguments");
  newTable->cleanFunction = cleanFunction;
  newTable->subTable = (HashSubTable *)hlr_malloc (newTable->tableSize*sizeof (HashSubTable));
  for (i=0;i<newTable->tableSize;i++) {
    HashSubTable *tableTmp = newTable->subTable+i;
    tableTmp->tablePt = avl_treeCreate (newTable->intraTableOrderFunction,
                                        newTable->cleanFunction);
  }
  newTable->numElements = 0;
  return newTable;
}

void hash_tableDestroyFunc (HashTable this1) {
  /**
    Destroys the HashTable and memory allocated to all the entries as
    defined by the cleanFunc() registered through hash_tableCreate()<br>
    Precondition: hash_tableCreate() should be called before
                  calling this function<br>
    Postcondition: Pointer this1 can not be used once this function
                   has been called.<br>
    Note: This function is only for internal use. Function hash_tableDestroy()
          should be used instead from outside the package.
    @param[in] this1 - HashTable that should be destroyed
  */
  int i;
  for (i=0;i<this1->tableSize;i++) {
    HashSubTable *tableTmp = this1->subTable+i;
    avl_treeDestroy (tableTmp->tablePt);
  }
  hlr_free (this1->subTable);
  hlr_free (this1);
}

int hash_tableInsert (HashTable this1,void *valueInsert,
                      void (*modifyDuplicate)(void *,void *)) {
  /**
     Inserts a new value in the HashTable.<br>
     Precondition: hash_tableCreate() should be called before
                   calling this function
     Postcondition: User is responsible for freeing the memory allocated to
                    new node by calling the hash_tableDestroy() function.
     @param[in] this1 - a HashTable
     @param[in] valueInsert - a pointer to new value to insert
     @param[in] modifyDuplicate - modifyFunc() to define what should be done
                                  when the valueToInsert is already found on
                                  the table.
     @return 0 if value found, else 1 if sucessfully inserted
  */
  int t;
  t = this1->tableSelectorFunction (valueInsert,this1->tableSize);
  HashSubTable *tableTmp = this1->subTable+t;
  int insertFlag = avl_treeInsert (tableTmp->tablePt,valueInsert,
                                   modifyDuplicate);
  if (insertFlag == 0)
    return 0;
  this1->numElements++;
  return 1;
}

int hash_tableFind (HashTable this1,void *valueToFind,void **valueFound) {
  /**
     Finds the value correponding to unique id of valueToFind on the HashTable.<br>
     Precondition: hash_tableCreate() should be called before
                   calling this function
     @param[in] this1 - a HashTable
     @param[in] valueToFind - a pointer to value to search for. valueToFind
                              should at least have the unique ID that are
                              used to distinguish two different values stored
                              on the table
     @param[in] valueFound - double pointer to which the value found on the
                             avlTree is copied.
     @return 0 if not found, else 1 if found
  */
  int t;
  t = this1->tableSelectorFunction (valueToFind,this1->tableSize);
  HashSubTable *tableTmp = this1->subTable+t;
  if (avl_treeIsEmpty (tableTmp->tablePt))
    return 0;
  int foundFlag = avl_treeFind (tableTmp->tablePt,valueToFind,valueFound);
  if (foundFlag == 1)
    return 1;
  return 0;
}

int hash_tableDelete (HashTable this1,void *valueToDelete) {
  /**
     Deletes the entry corresponding to input value from the HashTable.<br>
     Precondition: hash_tableCreate() should be called before
                   calling this function
     @param[in] this1 - a HashTable
     @param[in] valueToDelete - a pointer to value to delete
     @return 0 if not found, else 1 if sucessfully deleted
  */
  int t;
  t = this1->tableSelectorFunction (valueToDelete,this1->tableSize);
  HashSubTable *tableTmp = this1->subTable+t;
  if (avl_treeIsEmpty (tableTmp->tablePt))
    return 0;
  int foundFlag = avl_treeDelete (tableTmp->tablePt,valueToDelete);
  if (foundFlag)
    this1->numElements--;
  return foundFlag;
}

void hash_tablePrintStats (HashTable this1,FILE *outFile) {
  /**
     prints statistics about a HashTable.<br>
     Precondition: hash_tableCreate() should be called before
                   calling this function
     @param[in] this1 - a HashTable
     @param[in] outFile - file stream where the statistics should be printed.
                          Use stdout to print on terminal window.
  */
  int i;
  for (i=0;i<this1->tableSize;i++) {
    HashSubTable *tableTmp = this1->subTable+i;
    if (!avl_treeIsEmpty (tableTmp->tablePt))
      fprintf (outFile,"Table:%d\tNum_Elem:%ld\theight:%d\n",
               i,avl_treeNumNodes (tableTmp->tablePt),
               avl_treeHeight (tableTmp->tablePt));
  }
}

void hash_tableApplyFunc (HashTable this1,void *applyFunc (),int nargs,...) {
  /**
     Iterates through all the entries and applies the function
     specified by the function pointer applyFunc() to the value stored
     on every entry in the table.<br>
     Note: Number of arguments can be at max 6. For more arguments, define
     applyFunc() as applyFunc (value,int nargs,va_list args) in the main
     program. See avl_nodeCallFunction() for more details.<br>
     Precondition: hash_tableCreate() should be called before
                   calling this function
     @param[in] this1 - a HashTable
     @param[in] applyFunc - a pointer to function
     @param[in] nargs - number of variable arguments as integer,
                        variable arguments type casted to (void *)
  */
  int i;

  for (i=0;i<this1->tableSize;i++) {
    HashSubTable *tableTmp = this1->subTable+i;
    if (avl_treeIsEmpty (tableTmp->tablePt))
      continue;
    va_list args;
    va_start (args,nargs);
    avl_treeApplyFuncFixedArgs (tableTmp->tablePt,(void (*)())applyFunc,
                                nargs,args);
    va_end (args);
  }
}

int hash_tableNumElem (HashTable this1) {
  /**
     Returns the number of elements in the hash table
     @param[in] this1 - a HashTable
     @return number of elements
  */
  return this1->numElements;
}

//----- Functions for hash Table Iterator

HashIterator hash_itCreate (HashTable table) {
  /**
     Creates an iterator to access elements stored on HashTable.<br>
     Precondition: hash_tableCreate() should be called before
                   calling this function
     Postcondition: User is responsible to free the memory allocated
                    by calling the function hash_itDestroy().
     @param[in] table - a HashTable
     @return Pointer to new HashTable iterator
  */
  int i;
  HashIterator newIterator = (HashIterator)hlr_malloc (sizeof (struct _HashIteratorStruct_));
  newIterator->table = table;
  newIterator->currIterator = NULL;
  newIterator->currTableIndex = -1;
  for (i=0;i<table->tableSize;i++) {
    HashSubTable *tableTmp = table->subTable+i;
    if (!avl_treeIsEmpty (tableTmp->tablePt)) {
      newIterator->currIterator = avl_itCreate (tableTmp->tablePt);
      newIterator->currTableIndex = i;
      break;
    }
  }
  return newIterator;
}

void hash_itDestroyFunc (HashIterator iterator) {
  /**
     Destroys the HashTable iterator.<br>
     Precondition: hash_itCreate() should be called before calling this function
     Postcondition: iterator  can not be used once this function has been called.
     Note: This function is only for internal use. Function hash_itDestroy()
           should be used instead from outside the package.
     @param[in] iterator - a HashTable iterator that should be destroyed
  */
  avl_itDestroy (iterator->currIterator);
  hlr_free (iterator);
}

void *hash_itNextValue (HashIterator iterator) {
  /**
     Returns the next value and increments the iterator.<br>
     Precondition: hash_itCreate() should be called before
                   calling this function
     @param[in] iterator - a hashTable iterator
     @return Pointer to next value stored on the HashTable;
             NULL if no next element is found
  */
  int i;
  if (iterator->currIterator == NULL)
    die ("hash_itCreate() should be used before using hash_itNextValue.");
  void *returnVal = avl_itNextValue (iterator->currIterator);
  if (returnVal == NULL) {
    avl_itDestroy (iterator->currIterator);
    for (i=iterator->currTableIndex+1;i<iterator->table->tableSize;i++) {
      HashSubTable *tableTmp = iterator->table->subTable+i;
      if (!avl_treeIsEmpty (tableTmp->tablePt)) {
        iterator->currIterator = avl_itCreate (tableTmp->tablePt);
        iterator->currTableIndex = i;
        returnVal = avl_itNextValue (iterator->currIterator);
        break;
      }
    }
  }
  return returnVal;
}
