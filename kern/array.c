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
/** @file array.c
    @brief Module for handling dynamic arrays.
*/
#include <string.h>
#include <stdlib.h>
#include "log.h"
#include "array.h"

static char* mallocErrorMsg = "array: malloc/realloc/calloc failed.";

static int nArrays = 0;

Array uArrayCreate (int n,int size) {
  /**
     Create an Array of n elements having 'size' bytes.<br>
     Postcondition: arrayNumber() increased by 1<br>
     Use arrayDestroy() to destory the Array after use.<br>
     NOTE: Do not call this function in any program.
           Use the macro arrayCreate() instead
     @param[in] n - initial number of elements;
                     if n <=0, space for one element is allocated
     @param[in] size - element size
     @return new Array, each of its bytes initialized to 0;
  */
  Array new1 = (Array) malloc (sizeof (struct ArrayStruct));
  if (new1 == NULL)
    die (mallocErrorMsg);
  if (size <= 0)
    die ("negative size %d in uArrayCreate",size);
  if (n < 1)
    n = 1;
  new1->base = (char *)calloc (n,size);
  if (new1->base == NULL)
    die (mallocErrorMsg);
  new1->dim = n;
  new1->max = 0;
  new1->size = size;
  nArrays++;
  return new1;
}

static void arrayExtend (Array a,int n) {
  /**
     Extend an existing array to hold n emements.
     @param[in] a - the Array
     @param[in] n - the new size
   */
  char *new1;
  long int oldsize,newsize;

  if (a == NULL || n < a->dim)
    return;
  if (a->dim < 1 << 20) // 1 megabyte
    a->dim *= 2;
  else
    a->dim += 1 << 20;
  if (n >= a->dim)
    a->dim = n + 1;
  newsize = (long int)a->dim * (long int)a->size;
  oldsize = (long int)a->size*(long int)a->max;
  if (newsize <= oldsize)
    die ("arrayExtend: oldsize %d, newsize %d",oldsize,newsize);
  new1 = (char *)malloc (newsize);
  if (new1 == NULL)
    die (mallocErrorMsg);
  memcpy (new1,a->base,oldsize);
  memset (new1+oldsize,0,newsize-oldsize);
  free (a->base);
  a->base = new1;
}

void uArrayDestroy (Array a) {
  /**
     Destroy an array.<br>
     Postcondition: if a != NULL then arrayNumber() decreased by 1
     @param[in] a -- the Array
     NOTE: Do not call this function in any program.
           Use the macro arrayDestroy() instead
  */
  if (a == NULL)
    return;
  free (a->base);
  free (a);
  nArrays--;
}

void arrayClear (Array a) {
  /**
     Clear contents of Array a. Result: contents of a modified:<br>
     1) arrayMax(a) == 0;<br>
     2) extending the used part of a by e.g.
        array(a,10,type) and then accessing
        arru(a,9,type) will find element 9 filled with binary zeros
     @param[in] a -- the Array
  */
  memset (a->base,0,(size_t)((long int)a->dim * (long int)a->size));
  a->max = 0;
}

int arrayNumber (void) {
  /**
     @return number of Arrays currently allocated
  */
  return nArrays;
}

char *uArray (Array a,int i) {
  /**
     Extend Array if necessary; die() if i<0.
     @param[in] a -- the Array
     @param[in] i -- which element
     @return pointer to beginning of i.th element;
  */
  if (i >= a->max) {
    if (i >= a->dim)
      arrayExtend (a,i);
    a->max = i+1;
  }
  else if (i < 0)
    die ("array: referencing element %d < 0",i);
  return a->base + (long int)i*(long int)a->size;
}

char *uArrCheck (Array a,int i) {
  /**
     Checks whether element i is valid; if not dies
     @param[in] a -- the Array
     @param[in] i -- which element
     @return pointer to ith element
   */
  if (i >= a->max || i < 0)
    die ("array index %d out of bounds [0,%d]",
         i,a->max - 1);
  return a->base + (long int)i*(long int)a->size;
}

char *uArrPop (Array a) {
  /**
     Return last element of Array a and decrease
     Array length by one element.
     @param[in] a -- the array
     @return pointer to last element
  */
  int i = --a->max;
  if (i < 0)
    die ("stackPop: empty stack");
  return a->base + (long int)i*(long int)a->size;
}

Array arrayCopy (Array a) {
  /**
     Create a shallow copy of 'a'<br>
     Note: The user is responsible for freeing array returned after use
     @param[in] a - the Array -- can be NULL
     @return new Array or NULL if a was NULL
  */
  Array b;

  if (a == NULL || a->size == 0)
    return NULL;
  b = uArrayCreate (a->max,a->size);
  memcpy (b->base,a->base,(long int)a->max * (long int)a->size);
  b->max = a->max;
  return b;
}

void arrayMove (Array from,int start,int end,Array to)  {
  /**
     move elements from[start..end] to to[m..m+end-start+1]
     @param[in] from - Array to cut elements out of
     @param[in] start - start index in 'from'
     @param[in] end - end index in 'from'
     @param[in] to - Array to append cut elements to;
                     NULL if this is not needed (only a good idea
                     if elements don't own memory via pointers)
     @param[out] from - elements removed; formerly used space
                        after end of data filled with binary zeros.
     @param[out] to - if not NULL: moved elements from 'from' appended
  */
  int i;
  int mf; // number of elements in 'from'
  int n;  // number of elements to move
  long int nb; // number of bytes to move from 'from' to 'to'
  long int mb; // number of bytes to move down within 'from'
  char *fromp; // pointer to start position in 'from'
  char *fromp2; // beginning of area to shift in 'from'
  char *fromp3; // beginning of area to clear in 'from'
  char *top;   // pointer to target position in 'to'

  if (from == NULL || start < 0 || end >= arrayMax(from) || start > end)
    die ("arrayMove: max=%d start=%d end=%d",
         from ? arrayMax(from) : -1,start,end);
  if (to != NULL && to->size != from->size)
    die ("arrayMove: size mismatch %d/%d",
         from->size,to->size);
  if (to == from)
    die ("arrayMove: from and to are the same.");

  mf = arrayMax (from);
  n = end - start + 1; // number of elements to move
  nb = (long int)n * (long int)from->size; // number of bytes to move and set to zero
  fromp = from->base + (long int)start*(long int)from->size;
  if (to != NULL) {
    int mt = arrayMax (to); // number of elements in 'to'
    uArray (to, mt + n - 1); // allocate target space
    top = to->base + (long int)mt*(long int)to->size;
    memcpy (top,fromp,nb);
  }
  mb = (long int)(mf - end -1) * (long int)from->size;
  if (mb != 0) {
    fromp2 = from->base + (long int)(end+1)*(long int)from->size;
    i = mb;
    while (i-- > 0)
      *fromp++ = *fromp2++; // shuffle down
  }
  fromp3 = from->base + (long int)(mf - n)*(long int)from->size;
  memset (fromp3,0,nb);
  from->max = mf - n;
}

void arrayByteUniq (Array a) {
  /**
     Remove byte-wise identical duplicates;<br>
     Note:<br> (1) byte-wise identical can be different from contents-wise;<br>
     (2) use arrayUniq() for duplicate removal
     based on equality defined by order() function
     @param[in] a - Array sorted such that byte-wise identical entries are
                    adjacent
     @param[out] a - the Array but byte-wise identical duplicates removed
  */
  int i,j,k,as;
  char *x,*y,*ab;

  if (a == NULL || a->size == 0 || arrayMax (a) < 2)
    return;
  ab = a->base;
  as = a->size;
  for (i = 1,j = 0;i<arrayMax (a);i++) {
    x = ab + (long int)i * (long int)as;
    y = ab + (long int)j * (long int)as;
    for (k = a->size;k--;)
      if (*x++ != *y++)
        goto different;
    continue;
    different:
    if (i != ++j) {
      x = ab + (long int)i * (long int)as;
      y = ab + (long int)j * (long int)as;
      for (k=a->size;k--;)
        *y++ = *x++;
    }
  }
  arrayMax (a) = j + 1;
}

void arrayUniq (Array a,Array b,int (*order)(void*,void*))
{ /**
     Make a sorted Array unique.<br>
     Note: if byte-wise equality is sufficient and array elements
           do not own memory outside the Array, one can use
           arrayByteUniq() instead of this function
           to save a few CPU cycles.
     @param[in] a - Array sorted by the same order function that is
                    specified as the second argument
     @param[in] b - Array of same type as a, NULL ok (if one is not
                    interested in getting the duplicates)
     @param[in] order - the order function
     @param[out] a - all duplicates which are identical according
                     to the specified order function are moved to b
     @param[out] b - if not NULL: duplicates appended
  */
  int i,j,k;
  char *to;
  char *from;
  char *r;

  if (a == NULL || a->size == 0 || (b && a->size != b->size))
    die ("arrayUniq: bad input");
  if (arrayMax (a) < 2)
    return;
  j = 0;
  to = a->base;
  for (i=1;i<arrayMax (a);i++) {
    from = a->base + (long int)i * (long int)a->size;
    if (order (to,from) != 0) { // differ: stay in a
      to += a->size;
      if (to != from) {
        r = to;
        k = a->size;
        while (k-- > 0)
          *r++ = *from++;
      }
      j++;
    }
    else {  // equal: move to b
      if (b != NULL) {
        r = uArray (b,b->max);
        k = a->size;
        while (k-- > 0)
          *r++ = *from++;
      }
    }
  }
  arrayMax (a) = j+1;
}

void arraySort (Array a,int (*order)(void*,void*)) {
  /**
     Sorting an array
     @param[in] a - the Array
     @param[in] order - the order function
  */
  unsigned int n = a->max,s = a->size;
  void *v = a->base;

  if (n > 1)
    qsort (v,n,s,(int (*)(const void *,const void *))order);
}

int arrayFind (Array a,void *s,int *ip,int (* order)(void*, void*)) {
  /**
     Finds entry s in Array a sorted in ascending order of order()
     @param[in] a - the Array
     @param[in] s - element to be found
     @param[in] ip - if NULL: no output
     @param[out] ip - if input was not NULL,
                      if found sets *ip to index found
                      else sets *ip one step left of expected index
     @param[in] order - the order function
     @return 1 if found, else 0
  */

  int ord;
  int i=0,j=arrayMax (a),k;

  if (j == 0 || (ord = order (s,uArray (a,0))) < 0) { // array empty or s smaller than first element
    if (ip != NULL)
      *ip = -1;
    return 0; // not found
  }
  if (ord == 0) { // exact match on first element
    if (ip != NULL)
      *ip = 0;
    return 1; // found
  }
  if ((ord = order (s,uArray (a,--j))) > 0) { // s larger than last element of array
    if (ip != NULL)
      *ip = j;
    return 0;
  }
  if (ord == 0) { // exact match on last element
    if (ip != NULL)
      *ip = j;
    return 1;
  }
  for (;;) {
    k = i + ((j-i) >> 1); // midpoint
    if ((ord = order (s,uArray (a,k))) == 0) {
      if (ip != NULL)
        *ip = k;
      return 1;
    }
    if (ord > 0)
      i = k;
    else
      j = k;
    if (i == (j-1))
      break;
  }
  if (ip != NULL)
    *ip = i;
  return 0;
}

int arrayFindInsert (Array a,void * s,int *ip,int (*order)(void*,void*)) {
  /**
     Try to find element s in Array a; insert it if not there
     @param[in] a - the Array
     @param[in] s - the element to be found or inserted
     @param[in] ip - where to put the index value; if ip is NULL, the index
                     value is not returned
     @param[out] ip - index of s (where found or inserted)
     @param[in] order - the order function
     @return 1 if inserted, 0 if found
  */
  int i,j;
  long int k;
  int *ip2 = ip ? ip : &i;
  char *cp,*cq;

  if (arrayFind (a,s,ip2,order))
    return 0;  // no douplicates
  j = arrayMax (a) + 1;
  cp = uArray (a,arrayMax (a)); // to create space, avoid memcpy for same reasons as above
  {
    cp = cp + a->size - 1;  // cp is now end of last element
    cq = cp - a->size;
    k = (long int)(j - *ip2 - 2)*(long int)(a->size); // k = # of bytes to move
    while (k-- > 0)
      *cp-- = *cq--;
    cp = arrp (a,*ip2+1,char);
    cq = (char *)s;
    k = a->size;
    while (k-- > 0)
      *cp++ = *cq++;
  }
  ++*ip2;
  return 1;
}

int arrayRemove (Array a,void *s,int (* order)(void*,void*)) {
  /**
     Remove entry with contents 's' from Array 'a'.<br>
     Precondition: 'a' is sorted according to order()
  @param[in] a -- the array
  @param[in] s -- an element
  @param[in] order -- the order functions
  @return 1 if s was contained at least once in a (one s removed);
          0 if s was not contained in a (no change)
  */
  int i;

  if (arrayFind (a,s,&i,order)) { // memcpy would be faster but regions overlap and memcpy is said to fail with some compilers
    char *cp = uArray (a,i),*cq = cp + a->size;
    long int j = (long int)(arrayMax (a) - i)*(long int)(a->size);
    while (j-- > 0)
      *cp++ = *cq++;
    arrayMax (a)--;
    return 1;
  }
  else
    return 0;
}

int arrayRemoveD (Array a, int i) {
  /**
     Remove entry with index i from Array a
     @param[in] a - the Array
     @param[in] i - index, i>=0
     @return 1 if i<arrayMax(a) (element removed);<br>
             0 if i>=arrayMax(a) (no change)
  */
  if (i<arrayMax (a)) { // memcpy would be faster but regions overlap and memcpy is said to fail with some compilers
    char *cp = uArray (a,i);
    char *cq = cp + a->size;
    long int j = (long int)(arrayMax(a) - i)*(long int)(a->size);
    while (j-- > 0)
      *cp++ = *cq++;
    arrayMax (a)--;
    return 1;
  }
  else
    return 0;
}

Array arrayIntersect (Array a,Array b,int (*order)(void*,void*)) {
  /**
     Intersects two arrays.<br>
     Dies if either argument is NULL or if Arrays a and b are not comparable;
     The memory returned belongs to the caller of this function.<br>
     Note1: performs a shallow copy, i.e. if the data elements in the arrays
     contain pointers, these are not followed.<br>
     Note2: if the input arrays are sets with respect to the order() function,
     the result array will be a set. It is legal for the input arrays to
     contain duplicates (i.e. not be sets) - in that case the result will be
     reasonable, but not necessaril a set. See example below.<br>
     Array 1:<br>
     1 1 1 2 2 5 7 7 13 15 67 <br>
     Array 2:<br>
     0 1 1 2 3 3 4 4 5 5 5 8 8 9 9 12 12 15 <br>
     Intersection:<br>
     1 1 2 5 15 <br>
     Union:<br>
     0 1 2 3 4 5 7 8 9 12 13 15 67 <br>
     Merge:<br>
     0 1 1 1 1 1 2 2 2 3 3 4 4 5 5 5 5 7 7 8 8 9 9 12 12 13 15 15 67 <br>
     Minus a-b:<br>
     1 2 7 7 13 67 <br>
     Minus b-a:<br>
     0 3 3 4 4 5 5 8 8 9 9 12 12 <br>
     @param[in] a - Array sorted by the 'order' function
     @param[in] b - Array of same type as a, sorted by the same 'order'
                    function
     @param[in] order - the order function
     @return new Array with the elements shared by both a and b;
  */
  Array result;
  int i,j;
  int min;
  char *aEntry;
  char *bEntry;
  int orderResult;

  if (a == NULL || b == NULL)
    die ("arrayIntersect: null argument");
  if (a->size != b->size)
    die ("arrayIntersect: arguments of different types");
  min = (a->max < b->max) ? a->max : b->max;
  result = uArrayCreate (min,a->size);
  i = 0;
  j = 0;
  aEntry = a->base;
  bEntry = b->base;

  while (i < a->max && j < b->max) {
    orderResult = order (aEntry,bEntry);
    if (orderResult == 0) {
      memcpy(result->base + (long int)result->max * (long int)result->size, aEntry, a->size);
      (result->max)++;
      i++;
      j++;
      aEntry = a->base + (long int)i * (long int)a->size;
      bEntry = b->base + (long int)j * (long int)b->size;
      continue;
    }
    if (orderResult == -1) {
      i++;
      aEntry = a->base + (long int)i * (long int)a->size;
      continue;
    }
    j++;
    bEntry = b->base + (long int)j * (long int)b->size;
  }
  return result;
}

Array arrayMerge (Array a,Array b,int (*order)(void*,void*)) {
  /**
     Merges two arrays.<br>
     Note1: Performs a shallow copy, i.e. if the data elements in the arrays
            contain pointers, these are not followed.<br>
     Note2: The result of the function will typically not be a set
            (i.e. with respect to function order() there will be duplicates).
            Use arrayUnion() if you want to get a set.<br>
     Dies if arrays a and b are NULL or not comparable.<br>
     The memory returned belongs to the caller of this function.
     @param[in] a - Array sorted by the order function
     @param[in] b - Array of same type as a, sorted by the same order
                    function
     @param[in] order - the order function
     @return new Array with the elements present in either a or b or both;
  */
  Array result;

  if (a == NULL || b == NULL)
    die ("arrayMerge: null argument");
  if (a->size != b->size)
    die ("arrayMerge: arguments of different types");
  result = uArrayCreate (a->max+b->max, a->size);
  // now copy the first array
  memcpy (result->base,a->base,(long int)a->max * (long int)a->size);
  // now append the second array
  memcpy (result->base + (long int)a->max * (long int)a->size,b->base,(long int)b->max * (long int)b->size);
  result->max = a->max+b->max;
  arraySort (result,(ARRAYORDERF)order);
  return result;
}

Array arrayUnion (Array a,Array b,int (*order)(void*,void*))  {
  /**
     Like arrayMerge, but returning a unique array as result<br>
     Dies if arrays a and b are NULL or not comparable.<br>
     The memory returned belongs to the caller of this function.<br>
     @param[in] a - Array sorted by the 'order' function
     @param[in] b - Array of same type as a, sorted by the same 'order'
                    function
     @param[in] order - the order function
     @return new array with the elements present in either a or b or both
  */
  Array result;
  result = arrayMerge (a,b,(ARRAYORDERF)order);
  arrayUniq (result,NULL,(ARRAYORDERF)order);
  return result;
}

Array arrayMinus (Array a,Array b,int (*order)(void*,void*)) {
  /**
     Subtracts elements contained in Array b from Array a<br>
     The memory returned belongs to the caller of this function.<br>
     Note1: performs a shallow copy, i.e. if the data elements in the arrays
            contain pointers, these are not followed.<br>
     Note2: if the input array 'a' is a set with respect to the order()
            function, the result array will be a set. It is legal for the input
            arrays to contain duplicates (i.e. not be sets) -- in that case the
            result will be reasonable, but not necessarily a set.<br>
     @param[in] a - Array sorted by the 'order' function
     @param[in] b - Array of same type as a, sorted by the same 'order'
                    function
     @param[in] order - the order function
     @return new Array with the elements present in a but not in b
  */
  Array result;
  int i, j;
  char *aEntry;
  char *bEntry;
  int orderResult;

  if (a == NULL)
    die ("arrayMinus: null argument");
  if (b == NULL)
    return arrayCopy (a);
  if (a->size != b->size)
    die ("arrayMinus(size %d,%d): type mismatch",a->size,b->size);
  result = uArrayCreate (a->max,a->size);
  i = 0;
  j = 0;
  aEntry = a->base;
  bEntry = b->base;
  while (i < a->max && j < b->max) {
    orderResult = order(aEntry,bEntry);
    if (orderResult == 0) { // skip this one
      i++;
      j++;
      aEntry = a->base + (long int)i * (long int)a->size;
      bEntry = b->base + (long int)j * (long int)b->size;
      continue;
    }
    if (orderResult == -1) { // a smaller, so this is uniq
      memcpy (result->base + (long int)result->max * (long int)result->size,aEntry,a->size);
      (result->max)++;
      i++;
      aEntry = a->base + (long int)i * (long int)a->size;
      continue;
    }
    j++;
    bEntry = b->base + (long int)j * (long int)b->size;
  }

  /* nothing left in b; rest of a is uniq */
  while (i < a->max) {
    aEntry = a->base + (long int)i * (long int)a->size;
    memcpy (result->base + (long int)result->max * (long int)result->size,aEntry,a->size);
    (result->max)++;
    i++;
  }
  return result;
}

int arrayStrcmp (char **s1,char **s2) {
  /**
     Good for building sorted Arrays of strings:
     char *cp = "hallo";
     Array text = arrayCreate (1, char*);
     arrayInsert (text,&cp,arrayStrcmp)
  */
  return strcmp (*s1,*s2);
}

static int strcmp2 (const char *s1,const char *s2) {
  /**
     String comparison 1. case-insensitive, 2. case-sensitive.
  */
  int c;

  if ((c = strcasecmp (s1,s2)) != 0)
    return c;
  return strcmp (s1, s2);
}

int arrayStrcmp2 (char **s1,char **s2) {
  /**
     Order function for sorting/finding strings in a text array.
     Sorting order: 1. case-insensitive, 2. case-sensitive.<br>
     sort with arrayStrcmp2: allows to find case-sensitive AND
     case-insensitive>br>
     find with arrayStrcmp2: finds case-sensitive
  */
  return strcmp2 (*s1,*s2);
}

int arrayIntcmp (int *ip1,int *ip2) {
  /**
     Order function for Array of integers
     @param[in] ip1 -- pointer to first integer
     @param[in] ip2 -- pointer to second integer
  */
  if (*ip1 == *ip2)
    return 0;
  return (*ip1 < *ip2) ? -1 : 1;
}

int arrayFloatcmp (float *fp1,float *fp2) {
  /**
     Order function for Array of floats
     @param[in] fp1 -- pointer to first float
     @param[in] fp2 -- pointer to second float
  */
  if (*fp1 == *fp2)
    return 0;
  return (*fp1 < *fp2) ? -1 : 1;
}

int arrayDoublecmp (double *dp1,double *dp2) {
  /**
     Order function for Array of doubles
     @param[in] dp1 -- pointer to first double
     @param[in] dp2 -- pointer to second double
  */
  if (*dp1 == *dp2)
    return 0;
  return (*dp1 < *dp2) ? -1 : 1;
}
