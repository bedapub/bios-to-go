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
/** @file binalgparser.c
    @brief Parser for binary alignments (e.g. water, needle, prophet).
    Module prefix bap_
*/
#include <ctype.h>
#include "log.h"
#include "format.h"
#include "hlrmisc.h"
#include "sequtil.h"
#include "binalgparser.h"

/* --------------- begin module binalgparser --------------------

implementation idea:
  first the user of this module registers functions that should
  be called when interesting points in the blast output are found.
  Then the user starts the parser, which in turn drives the functions
  the user registered before, feeding these functions with
  the current data (e.g. sequence names, gapped sequences, ...).

  reads only the score (delimitter between alginments) and the alignment

public functions:
   void bap_init()
   void bap_run(filename)
   int  bap_register_<name>(function); (returns 1 parsing should continue,
                                        else 0 to stop parser)
     where <name> is:
------------------------------------------------------------
name          when called  /
              what delivered
------------------------------------------------------------
seqnames      after reading header /
              names,original length of both sequences
idsimi        after summary has been read /
              %id and %similarity #gaps
alignment     when the alignment ends /
              seq1,seq2,marker line,
              begin and end of both
------------------------------------------------------------
*/
/// maximum length of a line in the alignment
#define MAX_LEN 10000

static int (*seqnames_hook)(char *seq1Name,char *seq2Name);
static int (*idsimi_hook)(int isNuc,float score,float id,float simi,int gaps);
static int (*seq_hook)(char *s1,char *s2,char *match,
                       int beg1,int end1,int beg2,int end2);

void bap_init (void) {
  /**
     Initializes the parser
  */
  seqnames_hook = NULL;
  idsimi_hook = NULL;
  seq_hook = NULL;
}

void bap_register_seqnames (int (*f)(char *seq1Name,char *seq2Name)) {
  /**
     Registers a function to be called when the sequence names are known
     @param[in] f - the function
  */
  seqnames_hook = f;
}

void bap_register_idsimi (int (*f)(int isNuc,float score,
                                   float id,float simi,int gaps)) {
  /**
     Registers a function to be called when the % id and similarity and
     number of gapsare known
     @param[in] f - the function
  */
  idsimi_hook = f;
}

void bap_register_seq (int (*f)(char *s1,char *s2,char *match,
                                int beg1,int end1,int beg2,int end2)) {
  /**
     Registers a function to be called when the sequences are known
     @param[in] f - the function
  */
  seq_hook = f;
}

static Stringa seq1=NULL,seq2=NULL,match=NULL;
static float score;
static float id;
static float simi;
static int gaps;
static char *name1=NULL,*name2=NULL;
static char *testname1=NULL,*testname2=NULL;
static int isNuc;
static int beg1,end1,beg2,end2;

static void calcIdSimi (void) {
  int i,i1,s1,g1;

  i1 = 0;
  s1 = 0;
  g1 = 0;
  for (i=0;i<stringLen (seq1);i++) {
    if (string (match)[i] == '|') {
      i1++;
      s1++;
    }
    else if (string (match)[i] == ':')
      s1++;
    if (string (seq1)[i] == '-' || string (seq2)[i] == '-' ||
        string (seq1)[i] == '.' || string (seq2)[i] == '.' ||
        string (seq1)[i] == ' ' || string (seq2)[i] == ' ')
      g1++;
  }
  gaps = g1;
  id = 100.0 * i1 / stringLen (seq1);
  simi = 100.0 * s1 / stringLen (seq1);
}

static int reportAlignment (void) {
  int goOn = 1;

  calcIdSimi ();
  if (seqnames_hook)
    goOn = (*seqnames_hook)(name1,name2);
  if (!goOn)
    return 0;
  if (idsimi_hook)
    goOn = (*idsimi_hook)(isNuc,score,id,simi,gaps);
  if (!goOn)
    return 0;
  if (seq_hook) {
    int i;
    char *s1,*s2,*m;

    stringTranslate (seq1," ","-");
    stringTranslate (seq2," ","-");
    s1 = string (seq1);
    s2 = string (seq2);
    m = string (match);
    i = 0;
    while (s1[i] == '-' || s2[i] == '-') {
      if (s1[i] == '-') {
        if (beg2 < end2)
          beg2++;
        else
          beg2--;
      }
      else {
        if (beg1 < end1)
          beg1++;
        else
          beg1--;
      }
      i++;
    }
    s1 += i;
    s2 += i;
    m += i;
    i = strlen (s1)-1;
    while (s1[i] == '-' || s2[i] == '-') {
      if (s1[i] == '-') {
        if (beg2 < end2)
          end2--;
        else
          end2++;
      }
      else {
        if (beg1 < end1)
          end1--;
        else
          end1++;
      }
      i--;
    }
    s1[i+1] = '\0';
    s2[i+1] = '\0';
    m[i+1] = '\0';
    if (beg1 == 0) {
      beg1++;
    }
    if (beg2 == 0) {
      beg2++;
    }
    goOn = (*seq_hook)(s1,s2,m,
                       beg1,end1,beg2,end2);
  }
  if (!goOn)
    return 0;
  stringClear (seq1);
  stringClear (seq2);
  stringClear (match);
  hlr_free (name1);
  hlr_free (name2);
  hlr_free (testname1);
  hlr_free (testname2);
  gaps = 0;
  id = 0.0;
  simi = 0.0;
  score = 0.0;
  isNuc = -1;
  beg1 = end1 = beg2 = end2 = 0;
  return 1;
}

void bap_run (LineStream ls) {
  /**
     Parses the output of the programs and calls user defined functions.<br>
     Postcondition: the functions registered have been called
     @param[in] ls - input stream with alignment output
  */
  int goOn=1;
  char *pos;
  char *line;
  char *s;
  int containsSymbols;

  stringCreateOnce (seq1,100);
  stringCreateOnce (seq2,100);
  stringCreateOnce (match,100);
  gaps = 0;
  id = 0.0;
  simi = 0.0;
  score = 0.0;
  isNuc = -1;
  beg1 = end1 = beg2 = end2 = 0;
  while ((line = ls_nextLine (ls)) != NULL) {
    if (strStartsWithC (line,"# Matrix:")) {
      if (strstr (line+9,"BLOSUM"))
        isNuc = 0;
      else if (strstr (line+9,"DNA"))
        isNuc = 1;
    }
    else if (strStartsWithC (line,"# Aligned_sequences:")) {
      if (stringLen (seq1) > 0) {
        goOn = reportAlignment ();
        if (!goOn)
          return;
      }
    }
    else if (strStartsWithC (line,"Score:") ||
             strStartsWithC (line,"# Score:")) {
      pos = strstr (line,"Score:");
      if (!pos)
        die ("bap_run: Score not found");
      sscanf (pos+6,"%f",&score);
    }
    else if (strStartsWithC (line,"# 1: "))
      name1 = hlr_strdup (line+5);
    else if (strStartsWithC (line,"# 2: "))
      name2 = hlr_strdup (line+5);
    else if (strStartsWithC (line,"Local: ") ||
             strStartsWithC (line,"Global: ")) {
      char n1[100],n2[100];
      char *p;

      if (stringLen (seq1) > 0) {
        goOn = reportAlignment ();
        if (!goOn)
          return;
      }
      p = strchr (line,':');
      if (!p)
        die ("bap_run: expected : in %s",line);
      sscanf (p+1,"%99s %*s %99s",n1,n2);
      name1 = hlr_strdup (n1);
      name2 = hlr_strdup (n2);
      isNuc = 0; /* profit, prophet */
    }
    else if (line[0] == '\0')
      continue;
    else if (line[0] == '#')
      continue;
    else {
      char s1[MAX_LEN];
      char s2[MAX_LEN];
      char m[MAX_LEN];
      char n[MAX_LEN];
      int b1,b2;
      int p1,p2;
      int n1;
      int i,k;

      n1 = sscanf (line,"%10000s %d%n",n,&b1,&p1);
      if (n1 != 2)
        continue;
      if (!testname1)
        testname1 = hlr_strdup (n);
      else {
        if (!strEqual (testname1,n))
          die ("bap_run: sequence1 name mismatch %s vs %s on line %d",
               testname1,n,ls_lineCountGet (ls));
      }
      hlr_strmcpy (s1,line);
      line = ls_nextLine (ls);
      hlr_strmcpy (m,line);
      line = ls_nextLine (ls);
      n1 = sscanf (line,"%10000s %d%n",n,&b2,&p2);
      if (n1 != 2)
        continue;
      if (!testname2)
        testname2 = hlr_strdup (n);
      else {
        if (!strEqual (testname2,n))
          die ("bap_run: sequence2 name mismatch %s vs %s on line %d",
               testname2,n,ls_lineCountGet (ls));
      }
      if (p1 != p2) {
        if (abs (p1-p2) > 2)
          die ("bap_run: problem with begin coordinates on line %d",
               ls_lineCountGet (ls));
        if (p1 > p2)
          p2 = p1;
        else
          p1 = p2;
      }
      hlr_strmcpyI (s1,s1+p2,MAX_LEN);
      hlr_strmcpyI (m,m+p2,MAX_LEN);
      hlr_strmcpyI (s2,line+p2,MAX_LEN);
      i = 0;
      while (s1[i] == ' ' && s2[i] == ' ')
        i++;
      k = i+1;
      while (s1[k] != ' ' || s2[k] != ' ')
        k++;
      s1[k] = '\0';
      s2[k] = '\0';
      m[k] = '\0';
      stringCat (seq1,s1+i);
      stringCat (seq2,s2+i);
      stringCat (match,m+i);
      containsSymbols = 0;
      s = s1+i;
      while (*s != '\0') {
        if (*s != '-') {
          containsSymbols = 1;
          break;
        }
        s++;
      }
      if (beg1 == 0 && containsSymbols)
        beg1 = b1;
      containsSymbols = 0;
      s = s2+i;
      while (*s != '\0') {
        if (*s != '-') {
          containsSymbols = 1;
          break;
        }
        s++;
      }
      if (beg2 == 0 && containsSymbols)
        beg2 = b2;

      sscanf (s1+k+1,"%d",&end1);
      sscanf (s2+k+1,"%d",&end2);
    }
  }
  reportAlignment ();
}
