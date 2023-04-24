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
/*****************************************************************************
* This file is derived from the ACEDB genome database package (as of 1995)   *
* written by Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and         *
* Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr, which is    *
* Copyright (C) J Thierry-Mieg and R Durbin, 1991                            *
*****************************************************************************/
/** @file array.h
    @brief Module for handling dynamic arrays.
*/
#ifndef ARRAY_H
#define ARRAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* #define ARRAY_CHECK either here or in a single file to
   check the bounds on arru() and arrp() calls
   if defined here can be removed from specific C files by defining
   ARRAY_NO_CHECK
*/

/* #define ARRAY_CHECK */

//! the Array structure
typedef struct ArrayStruct {
  char* base; //!< char* since need to do pointer arithmetic in bytes
  int   dim;  //!< length of alloc'ed space, counted in elements
  int   size; //!< size of one array element
  int   max;  //!< number of elements in array
}*Array;

/* NB we need the full definition for arru() for macros to work
   do not use it in user programs - it is private.
*/

extern Array uArrayCreate (int n,int size); // do not use
/**
   Create an Array
   @param[in] n - number of initial elements
   @param[in] type - type of elements
*/
#define arrayCreate(n,type) uArrayCreate(n,sizeof(type)) // use this

extern void uArrayDestroy (Array a); // do not use
/**
   Destroy an Array
   @param[in] x - Array to be destroyed
*/
#define arrayDestroy(x) ((x) ? uArrayDestroy(x), x=NULL, 1 : 0) // use this

extern void arrayClear (Array a);
extern int arrayNumber (void);
extern char *uArray (Array a,int i);
/**
   Return a pointer to the i.th element of an Array. If the Array contains
   fewer than i elements, memory is allocated so that the element with index i
   is valid.
   @param[in] ar - the Array
   @param[in] i - which element
   @param[in] type - type of th elements
   @return pointer to the i.th element
*/
#define arrayp(ar,i,type) ((type*)uArray(ar,i))

/**
   Return the i.th element of an Array. If the Array contains fewer than i
   elements, memory is allocated so that the element with index i is valid.
   @param[in] ar - the Array
   @param[in] i - which element
   @param[in] type - type of th elements
   @return the i.th element
*/
#define array(ar,i,type) (*(type*)uArray(ar,i))

/**
   How many elements are currently in the Array
   @param[in] ar - the Array
   @return number of elements
*/
#define arrayMax(ar) ((ar)->max)

/**
   Sets the number of elements currently in the Array.<br>
   If fewer elements are present, allocates memory to hold at least the
   requested number of elements.<br>
   Note: use with care - arguments are evaluated twice (macro).
   @param[in] ar - the Array
   @param[in] j - requested size of Array
*/
#define arraySetMax(ar,j) (uArray((ar),(j)),(ar)->max=(j))

extern char *uArrCheck (Array a,int i);
#if (defined(ARRAY_CHECK) && !defined(ARRAY_NO_CHECK))
/**
   Return a pointer to the i.th element of an Array. If the Array contains
   fewer than i elements, this is an error.
   @param[in] ar - the Array
   @param[in] i - which element
   @param[in] type - type of th elements
   @return pointer to the i.th element
*/
#define arrp(ar,i,type) ((type*)uArrCheck(ar,i))

/**
   Return the i.th element of an Array. If the Array contains fewer than i
   elements, this is an error.
   @param[in] ar - the Array
   @param[in] i - which element
   @param[in] type - type of th elements
   @return the i.th element
*/
#define arru(ar,i,type) (*(type*)uArrCheck(ar,i))
#else
/**
   Return a pointer to the i.th element of an Array. If the Array contains
   fewer than i elements, this is an error.
   @param[in] ar - the Array
   @param[in] i - which element
   @param[in] type - type of th elements
   @return pointer to the i.th element
*/
#define arru(ar,i,type) ((*(type*)((ar)->base + (long int)(i)*(long int)(ar)->size)))

/**
   Return the i.th element of an Array. If the Array contains fewer than i
   elements, this is an error.
   @param[in] ar - the Array
   @param[in] i - which element
   @param[in] type - type of th elements
   @return the i.th element
*/
#define arrp(ar,i,type) (((type*)((ar)->base + (long int)(i)*(long int)(ar)->size)))
#endif

extern char *uArrPop (Array a);
extern Array arrayCopy (Array a);
extern void arrayMove (Array from,int start,int end,Array to);

/**
   A macro to cast order functions to what is expected by qsort
*/
#define ARRAYORDERF int(*)(void *,void *)

extern void arrayByteUniq (Array a);
extern void arrayUniq (Array a,Array b,int (*order)(void*,void*));
extern void arraySort (Array a,int (*order)(void*,void*));
extern int arrayFind (Array a,void *s,int *ip,int (*order)(void*,void*));
extern int arrayFindInsert (Array a,void *s,int *ip,int (*order)(void*,void*));

/**
   A macro to insert an element into a sorted Array in case we are not
   interested whether the element was alreadt there or at what position it is
   inserted
*/
#define arrayInsert(a,elem,order) arrayFindInsert(a,elem,NULL,order)

extern int arrayRemove (Array a,void * s,int (*order)(void*,void*));
extern int arrayRemoveD (Array a,int i);
extern Array arrayIntersect (Array a,Array b,int (*order)(void*,void*));
extern Array arrayMerge (Array a,Array b,int (*order)(void*,void*));
extern Array arrayUnion (Array a,Array b,int (*order)(void*,void*));
extern Array arrayMinus (Array a,Array b,int (*order)(void*,void*));

extern int arrayStrcmp (char **s1,char **s2);
extern int arrayStrcmp2 (char **s1,char **s2);
extern int arrayIntcmp (int *ip1,int *ip2);
extern int arrayFloatcmp (float *fp1,float *fp2);
extern int arrayDoublecmp (double *dp1,double *dp2);

/**
   Using an Array as a push/pop stack<br>
   Boundary checking: arrayTop() and arrayTopp() check for
   empty Array only if ARRAY_CHECK is in effect.<br>
   arrayPop() always checks for stack underflow
*/
#define Stacka Array

/**
   Like arrayCreate ()
*/
#define stackCreate arrayCreate

/**
   Like arrayDestroy ()
*/
#define stackDestroy arrayDestroy

/**
   Like arrayMax ()
*/
#define stackDepth arrayMax

/**
   Adds an element to the stack
   @param[in] a - the stack
   @param[in] elem - the element
   @param[in] type - type of the stack elements
*/
#define stackPush(a,elem,type) (array(a,arrayMax(a),type)=(elem))

/**
   Returns a pointer to the top element of the stack
   @param[in] a - the stack
   @param[in] type - type of the stack elements
   @return pointer to the top element of the stack
*/
#define stackTopp(a,type) (arrp(a,arrayMax(a)-1,type))

/**
   Returns the top element of the stack
   @param[in] a - the stack
   @param[in] type - type of the stack elements
   @return top element of the stack
*/
#define stackTop(a,type) (arru(a,arrayMax(a)-1,type))

/**
   Returns the top element of the stack and removes it from the stack
   @param[in] a - the stack
   @param[in] type - type of the stack elements
   @return top element of the stack
*/
#define stackPop(a,type) ((*(type*)uArrPop(a)))

#ifdef __cplusplus
}
#endif

#endif
