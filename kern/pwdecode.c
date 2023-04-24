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
/** @file pwdecode.c
    @brief Module 'encode/decode via C-string constant'.
    Module prefix endec_
*/
#include <string.h>
#include "log.h"
#include "pwdecode.h"

/// length of buffer
#define RLEN 16
/// version of encoder/decoder
#define ENDEC_VERSION '\01'

char *endec_decode1 (char *ebi) {
  /**
     @param[in] ebi - encoded buffer as produced by cry_encode1()
                      binary string of RLEN bytes
     @return string encoded in 'ebi' as plain text
             memory belongs to this routine; stable until next call.
  */
  static char s[RLEN+1];
  char b[RLEN];
  signed char *eb = (signed char *)b;
  int len;
  int i;
  unsigned char c;
  unsigned char p;
  memcpy (b,ebi,RLEN);
  if (eb[0] != ENDEC_VERSION)
    die ("endec_decode1: version mismatch");
  p = eb[1];
  for (i=2;i<RLEN;i++) {
    c = eb[i];
    eb[i] -= p;
    p = c;
  }
  len = (unsigned char)eb[RLEN-1];
  if (len < 1 || len > RLEN)
    die ("endec_decode1: failed");
  strncpy (s,b + RLEN - len - 1,len);
  s[len] = '\0';
  return s;
}
