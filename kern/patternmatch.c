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
/** @file patternmatch.c
    @brief Simple pattern match routines.
    Module prefix pm_
*/
#include "log.h"
#include "format.h"
#include "hlrmisc.h"

/*
simple pattern match, e.g. for sequence name or filename match

general idea:
the query string consists of the following ...

componnts: x -- string, all chars ok except *,?
           s -- string, all chars ok except *
           e -- like s, but empty ok

for check a match on ...
s --> loop char-by-char, allow mismatch at ?

s*  --> cut |s| chars from left, then compare
*s  --> cut |s| chars from right, then compare

s*x*s --> dissect into s,x,s then
          treat left an right like above;
          for the middle x iterate of the pieces between
          two '*'s and check like s
*/

static int pm_fmatch (char *query,char *subject,int longerSubjectOk) {
  /**
     Check for a fixed-position match between query and subject
     characters in the query must exactly match the characters
     from the subject in the same positions except that a ?
     in the query matches any subject character.
     query and subject must be of the same length to match
     when longerSubjectOk is false; else the subject sequence
     can have additional characters and still match.
     if both query and subject are empty, this is a match.
     @param[in] query - only '?' is special ('*' is a normal char)
     @param[in] subject
     @param[in] longerSubjectOk - 0 if the subject must have the
                                  same length as the query;
                                  1 if the subject may be longer than
                                  the query and still match
     @return 1 for match, 0 for mismatch
  */
  char qc;
  char sc;
  query--;
  subject--;
  while ((qc = *++query),(sc = *++subject),qc && sc)
    if (qc != sc && qc != '?')
      return 0;
  if (qc == '\0')
    if (longerSubjectOk || sc == '\0')
      return 1;
  return 0;
}

static char *pm_smatch (char *query,char *subject) {
  /**
     Search for query anywhere in subject
     @param[in] query - wildcard '?' recognized ('*' is a normal char)
     @param[in] subject
     @return position in subject after query match, if any;
             NULL if query not contained in subject
  */
  int qlen = strlen (query);
  int slen = strlen (subject);
  int i;
  if (qlen == 0)
    die ("pm_match with empty query");
  if (slen == 0)
    return NULL;
  for (i=0;i<=slen-qlen;i++)
    if (pm_fmatch (query,subject+i,1))
      return subject + i + qlen;
  return NULL;
}

int pm_match (char *query,char *subject) {
  /**
     Check for a UNIX filename matching like agreement between
     'query' and 'subject'.<br>
     To be considered a match, all chars in query must exactly match
     the corresponding character in subject except that a '*' in
     the query matches any number of arbitrary characters in subject
     and '?' in the query matches exactly one arbitrary character in subject.
     An empty query matches an empty subject.
     @param[in] query
     @param[in] subject
     @return 1 if query matches subject, 0 else
  */
  char *q; // work query string
  int qlen; // length of q, varying
  char *s; // work subject string
  int slen; // length of s, varying
  char *qbuf; // memory buffer start for q
  char *sbuf; // memory buffer start for s
  char *starp; // position of leftmost star
  char *laststarp; // position of rightmost star
  char *from;
  char *to;
  char lastc;
  int ok = 1; // set to 0 as soon as the routine is sure that this is no match
  if (*query == '\0' && *subject == '\0')
    return 1; // empty matches empty
  if (*subject == '\0')
    return 0; // filled query, empty subject
  q = hlr_strdup (query);
  s = hlr_strdup (subject);
  qbuf = q; // remember for freeing later
  sbuf = s; // remember for freeing later
  starp = strchr (q,'*'); // get leftmost '*', if any
  if (starp) {
    // squeeze consequtive multiple occurences of '*' into single occurences
    from = q-1;
    to = from;
    lastc = '\0';
    while (*++from) {
      if (lastc == '*' && *from == '*')
        continue; // skip this '*'
      lastc = *from;
      *++to = lastc;
    }
    *++to = '\0';
    /* loop over the stars trying to match pieces
       in between; special are pieces at the start and end
       of the query */
    /* no star at the beginning of the query?
       --> anchor match on left side */
    if (*q != '*') {
      qlen = starp-q;
      if (qlen < 1)
        die ("oops");
      *starp = '\0'; // temporarily cut query at star
      ok = pm_fmatch (q,s,1);
      *starp = '*';
      q = starp;
      s += qlen; // advance by number of chars processed
    }
    // no star at end of query? --> tail anchor match
    if (ok) {
      // find position of rightmost *
      laststarp = strrchr (q,'*');
      if (!laststarp)
        die ("oops2");
      qlen = strlen (laststarp+1);
      if (qlen) { // rightmost star not at end of query
        slen = strlen (s);
        if (qlen > slen)
          ok = 0;
        else {
          ok = pm_fmatch (laststarp+1,s+slen-qlen,0);
          // remove tail of query and subject
          *(laststarp+1) = '\0';
          s[slen-qlen] = '\0';
        }
      }
      // else: rightmost star is at end of query
    }
    if (ok) {
      // now first and last char of query must be '*'
      // printf ("q='%s' s='%s'\n",q,s);
      qlen = strlen (q);
      if (qlen < 1 || qlen == 2 || *q != '*' || q[qlen-1] != '*')
        die ("oops3");
      if (qlen != 1) {
        if (*s) {
          // query is of the form *...* : loop over the pieces
          q++; // skip first '*'
          while (ok && *q && (starp = strchr (q,'*'))) {
            *starp = '\0';
            s = pm_smatch (q,s);
            if (!s)
              ok = 0;
            q = starp+1;
          }
        }
        else
          ok = 0; // empty subject cannot match
      }
      // else q is just '*' and matches any subject
    }
  }
  else // no variable-size wildcard in query
    ok = pm_fmatch (q,s,0);
  hlr_free (qbuf);
  hlr_free (sbuf);
  return ok;
}
