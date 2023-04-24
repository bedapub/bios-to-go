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
/** @file stringlist.h
    @brief Should be good for<br>
    1 - a simple sorted list of unique strings<br>
    2 - basic set operations on associative arrays<br>
    3 - associative arrays (i.e. arrays indexed by string),
        optionally referencing any set of (complex) objects.<br>
    4 - lists of name/value pairs, where name and value are char*<br>
    Module prefices stringmap, stringpair, sl_
*/
#ifndef STRINGLIST_H
#define STRINGLIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "array.h"

/* --- part 1: StringPair/StringMap/StringMapIter for name/value pairs ----- */

//! the StringPair structure
typedef struct StringPairStruct {
  char *name; //!< name of the pair
  char *value; //!< value of the pair
}*StringPair;

extern StringPair stringpairCreate (char *name,char *value);
extern void stringpairDestroyFunc (StringPair this1);

/// Use this instead of stringpairDestroyFunc
#define stringpairDestroy(x) ((x) ? stringpairDestroyFunc(x),x=NULL,1 : 0)

/// StringMap is really an Array of StringPair
#define StringMap Array

extern void stringmapClear (StringMap this1);

/// convenience
#define stringmapCreate(initialSize) arrayCreate(initialSize,StringPair)
/// convenience
#define stringmapCreateClear(s,n) {if(s!=NULL)stringmapClear(s);else s=stringmapCreate(n);}
/// convenience
#define stringmapCreateOnce(s,n) {if(s==NULL) s=stringmapCreate(n);}
extern void stringmapDestroyFunc (StringMap this1);
/// call this macro instead of stringmapDestroyFunc()
#define stringmapDestroy(x) ((x) ? stringmapDestroyFunc(x),x=NULL,1:0)

extern StringPair stringmapAdd (StringMap this1,char *name,char *value);
extern StringPair stringmapGet (StringMap this1,char *name);
extern char *stringmapGetValue (StringMap this1,char *name);

//! the StringMapIter structure
typedef struct StringMapIterStruct {
  StringMap sm; //!< the StringMap
  int i; //!< the current position
}*StringMapIter;

extern StringMapIter stringmapiterCreate (StringMap sm);
extern StringPair stringmapiterNext (StringMapIter this1);
extern void stringmapiterDestroyFunc (StringMapIter this1);
/// call this macro instead of stringmapiterDestroyFunc()
#define stringmapiterDestroy(x) ((x) ? stringmapiterDestroyFunc(x),x=NULL,1:0)

/* --------- part 2: StringList data structure, sl_ functions -------- */

/* Don't be mislead by the ugly declaration below,
   stringlist is easy to use: see stringlist.c and stringlist.txt
*/

//! the StringListElem structure
typedef struct {
  char *s; //!< the string identifying this StringListElem
  Array pl; //!< pointer list: list of pointers associated with 's'
}StringListElem;

/// for casting
#define SLELEMORDERF(n) int(*n)(StringListElem*,StringListElem*)
/// useful for explicit casting of third argument to sl_createG()
#define SLORDERF int(*)(StringListElem*,StringListElem*)

//! the StringList structure
typedef struct StringListStruct {
  Array sl; //!< of StringListElem
  int byReference; //!< 1 = memory of strings is managed outside of this module, 0 = 'byValue': adding makes copies
  int hasPointers; //!< 1 if at least one element of 'sl' uses a pointer, 0 if no data pointers exist
  int cardinality; //!< number of pointers one 's' ususally has
  SLELEMORDERF (orderF); //!< pointer to ordering function
  int pIterElemI; //!< index of StringElem in current pIter, -1 if not active
  int pIterPlI; //!< index within 'pl' of current StringElem in pIter, -1 if not active
}*StringList;

extern StringList sl_createG (int initialSize,int byReference,
                              SLELEMORDERF (orderF));
/**
   Simple interface for StringList creation
   can be used if default behaviour i.e. working with copies of strings
   instead of references and using case-sensitive comparison is ok
*/
/// convenience
#define sl_create(size) sl_createG(size,0,NULL)

extern void sl_clear (StringList this1);
/// convenience
#define sl_createClear(s,n) {if(s!=NULL) sl_clear(s);else s=sl_create(n);}
/// convenience
#define sl_createOnce(s,n) {if(s==NULL) s=sl_create(n);}

extern void sl_destroy_func (StringList this1);
/**
   Destroy a StringList object
   @param[in] this1 - created by sl_create()/ sl_createG()
*/
/// use this macro instead of sl_destroy_func()
#define sl_destroy(this1) (sl_destroy_func(this1),this1=NULL)

extern int sl_addP (StringList this1,char *s,void *p);
/**
   Simple interface for adding strings to StringList if
   no pointers associated to strings
*/
/// convenience
#define sl_add(this1,s) sl_addP(this1,s,NULL)
/// convenience
#define sl_count(this1) arrayMax(this1->sl)

extern int sl_getP (StringList this1,char *s,int *ip,Array *pl /* of void* */);
/// returns: 1 if 's' found in StringList 'this1', else 0
#define sl_get(this1,s) sl_getP(this1,s,NULL,NULL)

// define SL_CHECK to enable range checking for StringList access
#ifdef SL_CHECK
char *sl_getStrIfunc (StringList this1,int i);
void sl_pIterInitIfunc (StringList this1,int i);
/// version with alloc check
#define sl_getStrI sl_getStrIfunc
/// version with alloc check
#define sl_pIterInitI sl_pIterInitIfunc
#else
/// version without alloc check
#define sl_getStrI(this1,i) (arrp((this1)->sl,(i),StringListElem)->s)
/// version without alloc check
#define sl_pIterInitI(this1,i) ((this1)->pIterElemI=(i),(this1)->pIterPlI=0)
#endif

extern int sl_pIterInit (StringList this1,char *s);
extern void *sl_pIterNext (StringList this1);
extern void *sl_getFirstP (StringList this1,char *s);

extern void sl_ref2val (StringList this1);
extern StringList sl_union (StringList sl1,StringList sl2);
extern StringList sl_intersect (StringList sl1,StringList sl2);
extern StringList sl_diff (StringList sl1,StringList sl2);

#ifdef __cplusplus
}
#endif

#endif
