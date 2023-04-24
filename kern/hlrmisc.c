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
/** @file hlrmisc.c
    @brief Miscelleanous nice routines not worth making separate modules from.
    Module prefix hlr_
*/
#include <stdlib.h>

#include "hlrmisc.h"
#include "log.h"

/**
   Number of memory blocks allocated.<br>
   Do not use this but use the macro hlr_getAllocCnt() instead
*/
int hlr_allocCnt = 0;

char *hlr_strmcpyI (char *to,char *from,int toLength) {
  /**
     Documentation see hlrmisc.h, macro hlr_strmcpy()
     Do not use this, instead use the macro hlr_strmcpy
  */
  to[toLength-1] = '\0';
  strncpy (to,from,toLength-1);
  return to;
}

char *hlr_strcpysFunc (char *to,char *from,int toLengthMax) {
  /**
     Documentation see hlrmisc.h, macro hlr_strcpys()
     Do not use this, instead use the macro hlr_strcpys
  */
  if (strlen (from) > toLengthMax)
    die ("hlr_strcpys: overflow: toLengthMax=%d, strlen(from)=%d, from='%.50s...'",
         toLengthMax,strlen (from),from);
  strcpy (to,from);
  return to;
}

void *hlr_mallocs (size_t size) {
  /**
     Do not use this, use hlr_malloc() instead
  */
  void *p = malloc (size);
  if (p == NULL)
    die ("hlr_mallocs(%d)",size);
  return p;
}

void *hlr_callocs (size_t nelem,size_t elsize) {
  /**
     Do not use this, use hlr_calloc() instead
  */
  void *p = calloc (nelem,elsize);
  if (p == NULL)
    die ("hlr_callocs(%d,%d)",nelem,elsize);
  return p;
}

char *hlr_strdups (char *s1) {
  /**
     Do not use this, use hlr_strdup() instead
  */
  char *s2 = strdup (s1);
  if (s2 == NULL)
    die ("hlr_strdups([%d bytes]) failed.",strlen (s1)+1);
  return s2;
}

char *s0f (char *s) {
  /**
     Printf ("%s",NULL) works on SGI, but not on WinNT or Solaris.<br>
     Therefore to be safe, use printf ("%s",s0f (s)) instead of printf ("%s",s)<br>
     If you are desparate for speed or s is an expression without side effects,
     you may replace s0f(s) with s0(s)
     which is a macro, trading the overhead of a function call
     for evaluating 's' twice. Beware of side-effects when using the macro!
  */
  return (char *)s0 (s);
}

int hlr_system (char *cmd,int nonZeroOK) {
  /**
     Execute shell command.<br>
     Note: exit status -1 indicates a serious condition of the operating
           system therefore causes a die() even if nonZeroOK is 1.
     @param[in] cmd - the command
     @param[in] nonZeroOK - if 1, then a non-zero exit status
                            from system is tolerated.
                            if 0, then a non-zero exit status
                            from system leads to a die()
     @return exit status (0=OK);
             (a non-zero exit status is only to be returned if nonZeroOK=1)
  */
  int rc = system (cmd);
  if (rc == -1 || (rc != 0 && !nonZeroOK))
    die ("system(%s) exit status %d.",cmd,rc);
  return rc;
}
