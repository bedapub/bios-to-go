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
/** @file hlrmisc.h
    @brief Miscelleanous nice routines not worth making separate modules from.
    Module prefix hlr_
*/
#ifndef HLRMISC_H
#define HLRMISC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>

extern char *hlr_strmcpyI (char *to,char *from,int toLength);

/**
   Copy string into a fixed sized array of char with truncation
   'strmcpy' mnemonic: 'string maximal copy'<br>
   usage:<br>
     char s[6];<br>
     hlr_strmcpy (s,"hello bufffer!");<br>
   Note: 'to' must not be an expression with side-effects.<br>
   See also: hlr_strcpys() below.
*/
#define hlr_strmcpy(to,from) hlr_strmcpyI(to,from, sizeof(to))

extern char *hlr_strcpysFunc (char *to,char *from,int toLengthMax);

/**
   Like strcpy, but save against overflow like hlr_strmcpy, but die()
   instead of silent truncation<br>
   Copy 'from' to 'to' while checking that 'from' fits into 'to'.<br>
   'to' must be of type char[].
*/
#define hlr_strcpys(to,from) hlr_strcpysFunc((to),(from),sizeof(to)-1)

extern int hlr_allocCnt;

/* Safe versions of standard memory allocation routines
   performing die() (from log.c) if allocation fails
*/
extern void *hlr_mallocs (size_t size);
extern void *hlr_callocs (size_t nelem,size_t elsize);
extern char *hlr_strdups (char *s1);

/* Macros for memory managment;
   please use those instead of the plain C library
   routines if you want to find memory leaks and verify allocation success
   if you compile with MALLOC_NOCHECK defined, the plain C routines
   are used.
   hlr_mallocExtern is provided to keep track of memory
   allocated by external routines like gdbm_fetch()
*/
#ifdef MALLOC_NOCHECK
/// free only when allocated
#define hlr_free(x) ((x) ? free(x), x=NULL, 1 : 0)

/// identical to strdup
#define hlr_strdup strdup

/// identical to malloc
#define hlr_malloc malloc

/// identical to calloc
#define hlr_calloc calloc

/// keep track of memory allocated by external routines like gdbm_fetch()
#define hlr_mallocExtern()

#else
// count allocations and check for allocation success

/// free only when allocated and keep track of number of allocations
#define hlr_free(x) ((x) ? free(x),--hlr_allocCnt,x=0,1 : 0)

/// like strdup but keep track of number of allocations
#define hlr_strdup(s) (++hlr_allocCnt,hlr_strdups(s))

/// like calloc but keep track of number of allocations
#define hlr_calloc(nelem,elsize) (++hlr_allocCnt,hlr_callocs(nelem,elsize))

/// like malloc but keep track of number of allocations
#define hlr_malloc(n) (++hlr_allocCnt,hlr_mallocs(n))

/// keep track of memory allocated by external routines like gdbm_fetch()
#define hlr_mallocExtern() (++hlr_allocCnt)
#endif

/// hlr_realloc identical to realloc
#define hlr_realloc realloc

/// Get the number of pending allocations
#define hlr_getAllocCnt() hlr_allocCnt

/**
   Be careful with all of the following macros:
   do not to use expressions with side effects as parameters,
   since they might be evaluated serveral times
*/
#define hlr_strdup0(s) ((s) ? hlr_strdup(s) : NULL)

/**
   Convert an integer into a string.<br>
   Usage:<br>
   char str[HLR_ITOA_SIZE];<br>
   int i=81062;<br>
   hlr_itoa (str,i);<br>
   The string must be at least 21 bytes long because<br>
   the result can be up to 20 bytes<br>
   (see ULONG_MAX in limits.h)<br>
*/
#define hlr_itoa(s,i) sprintf(s,"%d",i)
/// The maximum amount of memory meeded to to store an int
#define HLR_ITOA_SIZE 21

/// Define MIN only if not defined by C library
#ifndef MIN
#define MIN(a,b) ((a)>(b)?(b):(a))
#endif

/// Define MAX only if not defined by C library
#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif

/**
   How many elements are in a fixed array?<br>
   Usage:<br>
   char *values[2];<br>
   printf ("%d",NUMELE (values);<br>
   --> prints 2
*/
#define NUMELE(a) ((int)sizeof(a)/sizeof(a[0]))

extern char *s0f (char *s);
/** printf("%s", NULL) works on SGI, but not on WinNT or Solaris.<br>
    Therefore to be safe, use
    printf("%s", s0f(s)) instead of printf("%s", s)<br>
    If you are desparate for speed or s is an expression with side effects,
    you may replace s0f(s) with s0(s)
    which is a macro, trading the overhead of a function call
    for evaluating 's' twice. Beware of side-effects when using the macro!
*/
#define s0(s) ((s)?(s):"(null)")

/**
   A universal pointer to an illegal address; referencing it caused
   e.g. a BUS ERROR; use this to initialize local variable to enforce
   assigning a value before use
*/
#define ILLADR (void*)0xFFFFFFFF

extern int hlr_system (char *cmd,int nonZeroOK);

#ifdef __cplusplus
}
#endif

#endif
