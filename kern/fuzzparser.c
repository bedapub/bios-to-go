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
/** @file fuzzparser.c
    @brief Parses output of EMBOSS fuzz (nuc,pro,trans) programs.
    Module prefix fp_
*/
#include "log.h"
#include "format.h"
#include "linestream.h"
#include "fuzzparser.h"

/* --------------- begin module fuzzparser --------------------
secret: knows how to dissect the output of the fuzznuc,fuzzpro and
fuzztran programs

implementation idea:
  first the user of this module registers functions that should
  be called when interesting points in the pfs output are found.
  Then the user starts the parser, which in turn drives the functions
  the user registered before, feeding these functions with
  the current data (e.g. query name, subject name, ...).

public functions:
 void fup_init()
 void fup_run(filename)
 int  fup_register_<name>(function); (returns 1 parsing should continue,
                                      else 0 to stop parser)
   where <name> is:
------------------------------------------------------------
name          when called  /
              what delivered
------------------------------------------------------------
hit           after a hit has been read /
              seqname, sequence, pattern, begin, end, strand
------------------------------------------------------------
*/
/// structure to hold one pattern
typedef struct {
  char *name; //!< name of pattern
  int numMismatch; //!< number of mismatches
  char *pat; //!< the pattern
}Pat;

static Array pats = NULL;

static int (*hit_hook) (char *seqname,char *seq,char *pattern,
                        int numMismatch,int beg,int end,int frame);

void fup_register_hit (int (*f) (char *seqname,char *seq,char *pattern,
                                 int numMismatch,int beg,int end,int frame)) {
  /**
     Registers a function to be called when a hit is found
     @param[in] f - the function
  */
  hit_hook = f;
}

static void initPats (void) {
  if (pats != NULL) {
    int i;
    Pat *currPat;

    for (i=0;i<arrayMax (pats);i++) {
      currPat = arrp (pats,i,Pat);
      hlr_free (currPat->name);
      hlr_free (currPat->pat);
    }
    arrayClear (pats);
  }
  else
    pats = arrayCreate (10,Pat);
}

void fup_init (void) {
  /**
     Intialies the fuzz parser
  */
  hit_hook = NULL;
  initPats ();
}

void fup_run (LineStream ls) {
  /**
     Parses the output of the fuzz programs and calls user defined function.<br>
     Postcondition: the functions registered have been called
     @param[in] ls - input line stream with fuzz output
  */
  char *prog = NULL;
  char *line;
  char seqName[101];
  char *seq;
  int numMismatch;
  int start,end;
  int frame;
  Pat *currPat = malloc(sizeof(Pat));
  Texta toks;
  int i,f=0;
  char *p1;

  while ((line = ls_nextLine (ls)) != NULL) {
    if (strStartsWithC (line,"# Program: ")) {
      if (strStartsWithC (line+11,"fuzznuc"))
        prog = "fuzznuc";
      else if (strStartsWithC (line+11,"fuzzpro"))
        prog = "fuzzpro";
      else if (strStartsWithC (line+11,"fuzztran"))
        prog = "fuzztran";
      break;
    }
  }
  if (prog == NULL)
    return;
  while ((line = ls_nextLine (ls)) != NULL) {
    if (strStartsWithC (line,"# Sequence: ")) {
      initPats ();
      sscanf (line+12,"%100s",seqName);
    }
    else if (strEqual (line,"# Pattern_name Mismatch Pattern")) {
      while ((line = ls_nextLine (ls)) != NULL) {
        if (strlen (line) < 3)
          break;
        currPat = arrayp (pats,arrayMax (pats),Pat);
        toks = textStrtokP (line," ");
        currPat->name = hlr_strdup (textItem (toks,1));
        currPat->numMismatch = atoi (textItem (toks,2));
        currPat->pat = hlr_strdup (textItem (toks,3));
        textDestroy (toks);
      }
    }
    else if (strStartsWithC (line,"  Start     End")) {
      while ((line = ls_nextLine (ls)) != NULL) {
        if (line[0] == '\0')
          break;
        toks = textStrtokP (line," ");
        start = atoi (textItem (toks,0));
        end = atoi (textItem (toks,1));
        if (strEqual (prog,"fuzznuc") || strEqual (prog,"fuzztran")) {
          if (textItem (toks,2)[0] == '+')
            frame = 1;
          else if (textItem (toks,2)[0] == '-')
            frame = -1;
          else
            frame = 0;
        }
        else
          frame = 0;
        if (strEqual (prog,"fuzznuc"))
          f = 3;
        else if (strEqual (prog,"fuzzpro"))
          f = 2;
        else if (strEqual (prog,"fuzztran"))
          f = 4;
        if ((p1 = strchr (textItem (toks,f),':')) != NULL)
          *p1 = '\0';
        for (i=0;i<arrayMax (pats);i++) {
          currPat = arrp (pats,i,Pat);
          if (strEqual (textItem (toks,f),currPat->name))
            break;
        }
        if (i == arrayMax (pats))
          die ("pattern %s not found",textItem (toks,f));

        if (strEqual (prog,"fuzznuc"))
          f = 4;
        else if (strEqual (prog,"fuzzpro"))
          f = 3;
        else if (strEqual (prog,"fuzztran"))
          f = 5;
        if (textItem (toks,f)[0] == '.')
          numMismatch = 0;
        else
          numMismatch = atoi (textItem (toks,f));
        if (strEqual (prog,"fuzznuc"))
          f = 5;
        else if (strEqual (prog,"fuzzpro"))
          f = 4;
        else if (strEqual (prog,"fuzztran"))
          f = 9;
        seq = hlr_strdup (textItem (toks,f));
        textDestroy (toks);
        if (! (*hit_hook)(seqName,seq,currPat->pat,numMismatch,start,end,frame))
          return;
      }
    }
  }
}
