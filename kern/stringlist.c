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
/** @file stringlist.c
    @brief Should be good for<br>
    1 - a simple sorted list of unique strings<br>
    2 - basic set operations on associative arrays<br>
    3 - associative arrays (i.e. arrays indexed by string),
        optionally referencing any set of (complex) objects.<br>
    4 - lists of name/value pairs, where name and value are char*<br>
    Module prefices stringmap, stringpair, sl_
*/
/*
  This file houses two independent sets of functions and data structures:
  Item 1-3: StringList data structure, sl_ functions
  Item 4:   StringPair, StringMap, StringMapIter data structures,
            stringpair*, stringmap*, stringmapiter* functions

            In principle the StringList functions are sufficient also for
            name/value pairs, however they are not convenient to use (type
            safety, memory managment, sets of pointers add unneeded
            complexity). Therefore StringPair/Map/MapIter are provided
            for the frequent case of strings mapped to strings.

            Possible expansions for StringMap:
            - the current list is unsorted, allows duplicates and is only
              sequentially searched.
            - possibly add stringlistInsert()/stringlistFind() methods for
              operating
            StringMap in sorted mode.

            For more documentation see file stringlist.txt .
*/
/*
Usage examples:

Example 1 (StringList)
int main (int argc,char *argv[]) {
  StringList sl = sl_create (5);
  int i;
  for (i=1;i<argc;i++)
    sl_add (sl,argv[i]);
  puts ("print a sorted unique list of the input arguments");
  for (i=0;i<sl_count (sl);i++)
    puts (sl_getStrI (sl,i));
  sl_destroy (sl);
  return 0;
}

Example 2 (StringList)
StringList sl = sl_create (5);
...
if (sl_add (sl,"a"))
  ... new ...
else
  ... existing

Example 3 (StringMap)
what can not be done *elegantly* with StringList
is to associate a string with *one* data element (currently
it is always an array of pointers).
Here it is convenient to use StringPair/StringMap/StringMapIter:

int main (int argc,char *argv[]) {
  StringMap sm = stringmapCreate (5);
  StringMapIter smi;
  StringPair sp;
  int i;
  for (i=1;i<argc;i++)
    stringmapAdd (sm,argv[i],argv[argc-i]);
  smi = stringmapiterCreate (sm);
  while (sp = stringmapiterNext (smi))
    printf ("name: %s --> value %s\n",sp->name,sp->value);
  stringmapiterDestroy (smi);
  stringmapDestroy (sm);
  return 0;
}
*/

#include "log.h"
#include "format.h"
#include "hlrmisc.h"
#include "stringlist.h"

/* ------------ part 1: StringPair, StringMap, StringMapIter --------------- */

StringPair stringpairCreate (char *name,char *value) {
  /**
     Both name and value are copied into the new StringPair object,
     so they do not need to be kept stable by the caller.<br>
     Use stringpairDestory() to free the StringPair object after use.
     @param[in] name - must not be NULL
     @param[in] value - can be NULL
  */
  StringPair this1 = (StringPair)hlr_malloc (sizeof (struct StringPairStruct));
  hlr_assert (name,"stringpairCreate() needs name");
  this1->name = hlr_strdup (name);
  this1->value = (value ? hlr_strdup (value) : NULL);
  return this1;
}

void stringpairDestroyFunc (StringPair this1) {
  /**
     Destroys a StringPair. Do not call this function but the macro
     stringpairDestroy()
  */
  hlr_free (this1->name);
  hlr_free (this1->value);
  hlr_free (this1);
}

void stringmapClear (StringMap this1) {
  /**
     @param[in] this1 - created by stringmapCreate()
  */
  int i = arrayMax (this1);
  while (i--)
    stringpairDestroy (arru (this1,i,StringPair));
  arraySetMax (this1,0);
}

void stringmapDestroyFunc (StringMap this1) {
  /**
     Destroys a StringMap. Do not call this function, instead call the macro
     stringmapDestroy()
  */
  stringmapClear (this1);
  arrayDestroy (this1);
}

StringPair stringmapAdd (StringMap this1,char *name,char *value) {
  /**
     Add 'name'/'value' to this1 StringMap.<br>
     Postcondition: stringmapGetValue(this1, name) will return value,
                    if this was the first occurence of name;<br>
                    Adding the same name multiple times (possibly
                    with different values) is possible
     @param[in] this1 - the StringMap
     @param[in] name,value - exactly as in stringpairCreate()
  */
  StringPair sp = stringpairCreate (name,value);
  array (this1,arrayMax (this1),StringPair) = sp;
  return sp;
}

StringPair stringmapGet (StringMap this1,char *name) {
  /**
     Get the first occurence of 'name' in this1 StringMap.
     If there are multiple occurences, return the first one.
     @param[in] this1 - the StringMap
     @param[in] name
     @return StringPair if found, else NULL.<br>
             The StringPair returned is owned by the StringMap
             and must not be freed by the caller
  */
  int i = arrayMax (this1);
  while (i--)
    if (strEqual (arru (this1,i,StringPair)->name,name))
      return arru (this1,i,StringPair);
  return NULL;
}

char *stringmapGetValue (StringMap this1,char *name) {
  /**
     Like stringmapGet(), but directly returns char* value.
     The value returned is read-only to the caller.
  */
  StringPair sp = stringmapGet (this1,name);
  return sp ? sp->value : NULL;
}

StringMapIter stringmapiterCreate (StringMap sm) {
  /**
     Pepare a loop over the elements of the StringMap.<br>
     Postcondition: stringmapiterNext() can be called
  */
  StringMapIter this1 = (StringMapIter)hlr_malloc (sizeof (struct StringMapIterStruct));
  hlr_assert (sm,"stringmapiterCreate() needs StringMap");
  this1->sm = sm;
  this1->i = 0;
  return this1;
}

void stringmapiterDestroyFunc (StringMapIter this1) {
  /**
     Use stringmapiterDestory() macro, not this function
  */
  hlr_free (this1);
}

StringPair stringmapiterNext (StringMapIter this1) {
  /**
     @return next StringPair from map if possible, else NULL
  */
  if (this1->i >= arrayMax (this1->sm))
    return NULL;
  return arru (this1->sm,this1->i++,StringPair);
}

/* ---------------------- part 2: StringList --------------- */

static int stringElemOrder (StringListElem *e1,StringListElem *e2) {
  return strcmp (e1->s,e2->s);
}

StringList sl_createG (int initialSize,int byReference,SLELEMORDERF (orderF)) {
  /**
     Create a new StringList
     @param[in] initialSize - number of elements expected; it is no problem
                              if this is estimated too small -- it is just a
                              little less efficient
     @param[in] byReference - if 1, then sl_add() will assume that the pointers
                              added are stable and keep only references;
                              if 0, then sl_add() will make copies of the
                              strings added to the list
     @param[in] orderF - optional ordering function for type StringListElem,
                         should return -1, 0, 1 if first element is smaller,
                         equal, or greater than second (like strcmp()); if NULL
                         case-sensitive string comparison is used by default
     @return new StringList object; use sl_destroy() to get rid of it after use
  */
  StringList this1 = (StringList)hlr_malloc (sizeof (struct StringListStruct));
  this1->sl = arrayCreate (initialSize,StringListElem);
  this1->byReference = byReference;
  this1->hasPointers = 0;
  this1->cardinality = 1;
  this1->orderF = orderF ? orderF : stringElemOrder;
  this1->pIterElemI = -1;
  this1->pIterPlI = -1;
  return this1;
}

void sl_clear (StringList this1) {
  /**
     Remove all elements from 'this1' StringList.
  */
  if (this1->hasPointers) {
    int i = arrayMax (this1->sl);
    StringListElem *ep;
    while (i--) {
      ep = arrp (this1->sl,i,StringListElem);
      arrayDestroy (ep->pl);
    }
  }
  if (!this1->byReference) {
    int i = arrayMax (this1->sl);
    StringListElem *ep;
    while (i--) {
      ep = arrp (this1->sl,i,StringListElem);
      hlr_free (ep->s);
    }
  }
  arraySetMax (this1->sl,0);
  this1->hasPointers = 0;
  this1->pIterElemI = -1;
  this1->pIterPlI = -1;
}

void sl_destroy_func (StringList this1) {
  /**
     Don't call this from your programs, use sl_destroy() instead
  */
  if (this1 == NULL)
    return;
  sl_clear (this1);
  arrayDestroy (this1->sl);
  hlr_free (this1);
}

int sl_addP (StringList this1,char *s,void *p) {
  /**
     Add string 's' and its accompanying pointer 'p' to 'this1' StringList
     @param[in] this1 - created earlier sl_create() / sl_createG()
     @param[in]  s - a zero-terminated C string, not NULL;
                 if 'this1' was created 'byReference' then
                 s MUST be kept stable until sl_destroy(this1);
                 otherwise a copy of s is made and s may vary
                 after sl_add()
     @param[in] p pointer to additional data describing 's' or NULL if none;
     @param[out] this1 - with 's' (and 'p') added
     @return 1 if added, 0 if not added because already present
  */
  StringListElem e;
  StringListElem *ep;
  int i;
  int isNewElem;
  e.s = s;
  e.pl = NULL;
  isNewElem = arrayFindInsert (this1->sl,&e,&i,(ARRAYORDERF)this1->orderF);
  ep = arrp (this1->sl,i,StringListElem);
  if (isNewElem && !this1->byReference)
    ep->s = hlr_strdup (s);
  if (p != NULL) {
    if (ep->pl == NULL) {
      ep->pl = arrayCreate (this1->cardinality,void*);
      this1->hasPointers = 1;
    }
    array (ep->pl,arrayMax (ep->pl),void*) = p;
  }
  return isNewElem;
}

char *sl_getStrIfunc (StringList this1,int i) {
  /**
     Get the string value of the 'i'th element in the string list.<br>
     do not call in your programs directy, use sl_getStrI()
     @param[in] this1 - created by sl_create() / sl_createG()
     @param[in] i - 0 <= i < sl_count(this1) must hold
     @return string no 'i'; the memory is managed by this routine;
             it must not be written to by the user of this routine
  */
  if (i < 0 || i >= arrayMax (this1->sl))
    die ("sl_getStrI(%d), sl_count()=%d",i,arrayMax (this1->sl));
  return arrp (this1->sl,i,StringListElem)->s;
}

void sl_pIterInitIfunc (StringList this1,int i) {
  /**
     Initiate a scan over pointers of the 'i'th element.<br>
     Postcondition: sl_pIterNext() can be called.<br>
     Do not call in your programs directy, use sl_pIterInitI()
     @param[in] this1 - StringList created by sl_create() / sl_createG()
     @param[in] i - 0 <= i < sl_count(this1) must hold
  */
  if (i < 0 || i >= arrayMax (this1->sl))
    die ("sl_pIterInitI(%d), sl_count()=%d",i,arrayMax (this1->sl));
  this1->pIterElemI = i;
  this1->pIterPlI = 0;
}

int sl_pIterInit (StringList this1,char *s) {
  /**
     Initiate a scan over pointers associated with string s in
     StringList this1.<br>
     Postcondition: sl_pIterNext() can be called if search string found;
                    else unchanged until successfully initialized
     @param[in] this1 - created by sl_create()/sl_createG()
     @param[in] s -- search string
     @return 1 if ok; 0 if search string not found in StringList
  */
  StringListElem e;
  int i;
  e.s = s;
  if (arrayFind (this1->sl,&e,&i,(ARRAYORDERF)this1->orderF)) {
    this1->pIterElemI = i;
    this1->pIterPlI = 0;
    return 1;
  }
  return 0;
}

void *sl_pIterNext (StringList this1) {
  /**
     Return next pointer from current scan.<br>
     Precondition: sl_pIterInit()/sl_pIterInitI() has been called
     @param[in] this1 - created by sl_create() / sl_createG()
     @return void* - next pointer or NULL if no more pointers
  */
  StringListElem *ep;

  if (this1->pIterPlI == -1 || this1->pIterElemI == -1)
    die ("sl_pIterNext() without sl_pIterInit()");
  ep = arrp (this1->sl,this1->pIterElemI,StringListElem);
  if (!ep->pl || (this1->pIterPlI == arrayMax (ep->pl))) {
    this1->pIterElemI = -1;
    this1->pIterPlI = -1;
    return NULL;
  }
  return arru (ep->pl,this1->pIterPlI++,void*);
}

void *sl_getFirstP (StringList this1,char *s) {
  /**
     Get first pointer associated with 's' in 'this1' StringList.
     @param[in] this1 - created by sl_create()/sl_createG()
     @param[in] s - search string
     @return void* - pointer (added via sl_addP()) or
                     NULL if 's' not found or
                     NULL if 's' found, but not associated with pointer
  */
  StringListElem e;
  int i;

  e.s = s;
  if (arrayFind (this1->sl,&e,&i,(ARRAYORDERF)this1->orderF)) {
    StringListElem *ep;
    ep = arrp (this1->sl,i,StringListElem);
    if (ep->pl == NULL || arrayMax (ep->pl) == 0)
      return NULL;
    return arru (ep->pl,0,void*);
  }
  return NULL;
}

int sl_getP (StringList this1,char *s,int *ip,Array *pl /* of void* */) {
  /**
     Check if 's' has been added and get pointer to associated data
     @param[in] this1 - created by sl_create() / sl_createG()
     @param[in] s - search string
     @param[in] ip - place where to put an integer; NULL to indicate no interest
     @param[in] pl - where to place a pointer to an Array of pointers
                     to data associated with 's'; NULL to indicate no interest
     @return 1 if 's' found, else 0
     @param[out] pl - pointer Array for 's', only changed if 1 returned
     @param[out] ip - internal index of element found; can be used
                      in sl_getStrI(); only changed if 1 returned
  */
  StringListElem e;
  int i;

  e.s = s;
  if (arrayFind (this1->sl,&e,&i,(ARRAYORDERF)this1->orderF)) {
    if (pl != NULL)
      *pl = arrp (this1->sl,i,StringListElem)->pl;
    if (ip != NULL)
      *ip = i;
    return 1;
  }
  return 0;
}

void sl_ref2val (StringList this1) {
  /**
     Turn a 'byReference' StringList into a 'byValue' StringList
     @param[in] this1 - created by sl_createG() with byReference == 1
     @param[out] this1 - now a 'byValue' StringList
  */
  StringListElem *ep;
  int i = arrayMax (this1->sl);
  if (!this1->byReference)
    die ("sl_ref2val() on 'byValue' StringList");
  while (i--) {
    ep = arrp (this1->sl,i,StringListElem);
    ep->s = hlr_strdup (ep->s);
  }
  this1->byReference = 0;
}

StringList sl_union (StringList sl1,StringList sl2) {
  /**
     Determine union of two StringLists
     @param[in] sl1, sl2 - created by sl_create() / sl_createG(),
                           without any associated pointers
     @return a new StringList containing union of sl1 and sl2.
             user is responsible for destruction by calling sl_destroy().
  */
  StringList result;
  StringList bigList = sl1;
  StringList smallList = sl2;
  int i;
  if (sl1->hasPointers || sl2->hasPointers)
    die ("sl_union() cannot handle StringLists with pointers");
  if (sl1->orderF != sl2->orderF)
    die ("sl_union() cannot handle StringLists with different ordering functions");
  if (arrayMax (sl2->sl) > arrayMax (sl1->sl)) {
    bigList = sl2;
    smallList = sl1;
  }
  // efficient copying of larger StringList
  result = sl_createG (0,0,sl1->orderF);
  arrayDestroy (result->sl);
  result->sl = arrayCopy (bigList->sl);
  // addition of smaller StringList
  i = arrayMax (smallList->sl);
  while (i--)
    sl_add (result,sl_getStrI (smallList,i));
  return result;
}

StringList sl_intersect (StringList sl1,StringList sl2) {
  /**
     Determine intersection of two StringLists
     @param[in] sl1, sl2 - created by sl_create() / sl_createG(),
                           without any associated pointers
     @return a new StringList containing intersection of sl1 and sl2.
             user is responsible for destruction by calling sl_destroy().
  */
  StringList result;
  StringList bigList = sl1;
  StringList smallList = sl2;
  int i;
  StringListElem e;

  if (sl1->hasPointers || sl2->hasPointers)
    die ("sl_intersect() cannot handle StringLists with pointers");
  if (sl1->orderF != sl2->orderF)
    die ("sl_intersect() cannot handle StringLists with different ordering functions");
  if (arrayMax (sl2->sl) > arrayMax (sl1->sl)) {
    bigList = sl2;
    smallList = sl1;
  }
  e.pl = NULL;
  result = sl_createG (arrayMax (smallList->sl),0,sl1->orderF);
  for (i=0;i<arrayMax (smallList->sl);i++) {
    e.s = sl_getStrI (smallList,i);
    if (arrayFind (bigList->sl,(void *)&e,NULL,(ARRAYORDERF)bigList->orderF))
      array (result->sl,arrayMax (result->sl),StringListElem) = e;
  }
  return result;
}

StringList sl_diff (StringList sl1,StringList sl2) {
  /**
     Determine difference of two StringLists (sl1 - sl2)
     @param[in] sl1, sl2 - created by sl_create() / sl_createG(),
                           without any associated pointers
     @return a new StringList containing difference of sl1 and sl2.
             user is responsible for destruction by calling sl_destroy().
  */
  StringList result;
  int i;
  StringListElem e;
  if (sl1->hasPointers || sl2->hasPointers)
    die ("sl_diff() cannot handle StringLists with pointers");
  if (sl1->orderF != sl2->orderF)
    die ("sl_diff() cannot handle StringLists with different ordering functions");
  e.pl = NULL;
  result = sl_createG (arrayMax (sl1->sl),0,sl1->orderF);
  for (i=0;i<arrayMax (sl1->sl);i++) {
    e.s = sl_getStrI (sl1,i);
    if (!arrayFind (sl2->sl,(void *)&e,NULL,(ARRAYORDERF)sl2->orderF))
      array (result->sl,arrayMax (result->sl),StringListElem) = e;
  }
  return result;
}
