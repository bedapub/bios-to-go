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
/** @file alphatrans.c
    @brief Encode/decode byte stream for transmission over channel.
    Supporting only a restricted alphabet.
    Module prefix altr_
*/
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "log.h"

/*
  Assumptions:
  \n and \0 are never allowed chars for the transmission channel
*/

/*
  configure allowed char set here;
  this should be the only place needing modification when adapting
  to the char set allowed on a specific transmission channel
  since 256 different values for a byte exist, but the number of allowed
  values is e.g. only 64, several escape characters may be necessary.
*/
static unsigned char *allowed = (unsigned char *)"abcdefghijklmnopqrstuvwxyzABCDEFGHIKJLMNOPQRSTUVWXYZ0123456789 _";
static unsigned char *escapes = (unsigned char *)"JZQX";

/// # of escape chars used, always = strlen(escapes)
#define ESCAPE_COUNT 4

static char encode[256][2]; /* for each possible input char, its encoding:
                               encode[x][0] -- first char of code
                               encode[x][1] -- second char of code,
                               0 if encoding uses only one char
                            */
static unsigned char chartype[256]; /* for decoding:
                                       0: ignore this char
                                       1: use it as is
                                       2: escape char #1
                                       3: escape char #2
                                       ...
                                    */

static char decode[ESCAPE_COUNT][256]; // for each escape char the decoding function

void altr_init (void) {
  /**
     Initialize this module.<br>
     Postcondition: all other functions can be called
  */
  unsigned char *cp;
  unsigned char c;
  unsigned char *escape; // current escape character
  int i;
  int j;

  // produce encoding for chars allowed on the transmission channel
  for (i=0;i<256;i++)
    encode[i][0] = encode[i][1] = '\0';
  cp = allowed - 1;
  while ((c = *++cp) != '\0') // all the allowed chars code for themselves, except ...
    encode[c][0] = c;
  cp = escapes - 1;
  while ((c = *++cp) != '\0') // ... for the escape chars
    encode[c][0] = '\0';
  // produce escaped encoding for chars not allowed on the transmission channel
  escape = escapes;
  cp = allowed;
  for (i=0;i<256;i++) {
    if (encode[i][0] != '\0')
      continue;
    encode[i][0] = *escape;
    encode[i][1] = *cp;
    if (*++cp == '\0') {
      cp = allowed;
      if (*++escape == '\0')
        die ("not enough escape chars defined. fix source code.");
    }
  }
  /* print the encoding table for debugging / porting to Java
     printf ("encode[0][]=\"",j);
     for (i=0;i<256;i++)
       putchar (encode[i][0]);
     printf ("\"\n");
     printf ("encode[1][]=\"",j);
     for (i=0;i<256;i++)
       printf ("\\%o",encode[i][1]);
     printf ("\"\n");
  */
  // setup decoding function
  for (i=0;i<256;i++)
    chartype[i] = '\0';
  cp = allowed - 1;
  while ((c = *++cp) != '\0') // all allowed chars code for themselves, except ...
    chartype[c] = 1;
  cp = escapes - 1;
  while ((c = *++cp) != '\0') // ... for the escape chars
    chartype[c] = cp - escapes + 2;
  for (j=0;j<ESCAPE_COUNT;j++)
    for (i=0;i<256;i++)
      decode[j][i] = '?'; // should never be used
  for (j=0;j<ESCAPE_COUNT;j++) {
    c = escapes[j];
    for (i=0;i<256;i++) {
      if (encode[i][0] == c) { // current escape char
        decode[j][(int)encode[i][1]] = i;
      }
    }
  }
}

static unsigned char *gB = NULL; // output buffer
static int gBlen; // allocated output buffer length

static void balloc (int maxlen) {
  /**
     (re)allocate encode/decode buffer
  */
  if (gB != NULL && gBlen < maxlen) {
    free (gB);
    gB = 0;
  }
  if (gB == NULL) {
    gB = (unsigned char *)malloc (maxlen);
    gBlen = maxlen;
  }
}

void altr_end (void) {
  /**
     End activity of altr module; deallocate resources
  */
  if (gB != NULL) {
    free (gB);
    gB = 0;
  }
}

char *altr_bytes (char *s,int len) {
  /**
     Encode the first 'len' bytes from 's'.<br>
     Precondition: altr_init()
     @return encoded version of s; the returned memory is managed by
             this function; it may not be free'd or realloc'd by
             the user of this function
  */
  int i;
  int maxlen = len * 2 + 1; // maximum output length needed
  unsigned char *to;
  unsigned char *from;
  unsigned char c;

  balloc (maxlen);
  to = gB;
  from = (unsigned char*)s;
  for (i=0;i<len;i++) {
    *to++ = encode[*from][0];
    if ((c = encode[*from][1]) != '\0')
      *to++ = c;
    from++;
  }
  *to = '\0';
  return (char *)gB;
}

char *altr_read (char *from,int *outLen) {
  /**
     Decode null-terminated string 'from'
     @param[in] from - null-terminated string
     @param[out] outLen - length of returnd string, without the extra
                          trailing '\0'
     @return decoded version of 'from'; an additional trailing '\0' is
             appended; the returned memory is managed by
             this function; it may not be free'd or realloc'd by
             the user of this function
  */
  int maxlen = strlen (from) + 1;
  unsigned char *cp = (unsigned char *)from - 1;
  unsigned char c;
  unsigned char *to; // current position in output buffer gB
  unsigned char ct; // char type, see declaration of chartype[]

  balloc (maxlen);
  to = gB;
  while ((c = *++cp) != '\0') {
    ct = chartype[c];
    if (ct == 0)
      continue;
    if (ct == 1) {
      *to++ = c;
      continue;
    }
    ct -= 2;
    if (ct > ESCAPE_COUNT - 1)
      die ("altr: decode error. abort.");
    if (! (c = *++cp))
      die ("altr: premature end of input. abort.");
    *to++ = decode[ct][c];
  }
  *outLen = to - gB;
  *to = '\0';
  return (char *)gB;
}

void altr_file (FILE *f,int linewidth,int blocksize,int (*callback)(char *)) {
  /**
     Encode file f, inserting line breaks every 'linewidth' output chars
     and calling 'callback' every 'blocksize' output chars
     @param[in] f - the file
     @param[in] linewidth - every linewidth output chars, a \n is inserted
     @param[in] blocksize - every blocksize output chars, callback() is called
     @param[in] callback - function called when blocksize reached; callback
                           should return 1 to continue processing and 0
                           to make altr_file() terminate immediately.
                           the encoded block is passed to callback().
  */
  int c;
  int goOn = 1;
  unsigned char *linestart;
  unsigned char e;
  unsigned char *to;

  balloc (blocksize+5);
  to = gB;
  linestart = gB;
  while ((c = getc (f)) != EOF) {
    *to++ = encode[c][0];
    if ((e = encode[c][1]) != '\0')
      *to++ = e;
    if (to - gB >= blocksize) {
      *to++ = '\n';
      *to = '\0';
      if (!(goOn = callback ((char *)gB)))
        break;
      to = gB;
      linestart = gB;
      continue;
    }
    if (to - linestart >= linewidth) {
      *to++ = '\n';
      linestart = to;
    }
  }
  if (goOn && to != gB) { // send rest
    *to++ = '\n';
    *to = '\0';
    callback ((char*)gB);
  }
}
