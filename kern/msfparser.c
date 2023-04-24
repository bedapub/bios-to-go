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
/** @file msfparser.c
    @brief MSF (GCG multiple sequence file) parser,
    seems to work for ClustalW output as well.
    Module prefix msfp_
*/
#include <ctype.h>
#include "log.h"
#include "format.h"
#include "sequtil.h"
#include "msfparser.h"

/* --------------- begin module msfparser --------------------

implementation idea:
  first the user of this module registers functions that should
  be called when interesting points in the msf file are found.
  Then the user starts the parser, which in turn drives the functions
  the user registered before, feeding these functions with
  the current data (e.g. query name, subject name, ...).

public functions:
   void msfp_init()
   void msfp_run(linestream)
   void msfp_register_<name>(function); (returns 1 parsing should continue,
                                         else 0 to stop parser)
     where <name> is:
------------------------------------------------------------
name          when called  /
              what delivered
------------------------------------------------------------
seq           at the end of the stream: pointer to names and sequences
------------------------------------------------------------

private functions: see below, declared 'static'
*/
/// structure to hold results
typedef struct {
  char *name; //!< name of sequence
  Stringa seq; //!< sequence
  int order; //!< order of sequence in alignment
}Seq;

static int sortSeqs0 (Seq *s1,Seq *s2) {
  return strcmp (s1->name,s2->name);
}

static int sortSeqs1 (Seq *s1,Seq *s2) {
  return s1->order - s2->order;
}

static Array seqs = NULL;
static Texta seqPtrs = NULL;
static Texta namePtrs = NULL;

static int (*seq_hook) (int algIsNuc,int numSeq,char **names,char **sequences);

void msfp_register_seq (int (*f)(int algIsNuc,int numSeq,
                                 char **names,char **sequences)) {
  /**
     Registers a function to be called when the sequences in the alignment
     have been found
     @param[in] f - the function
  */
  seq_hook = f;
}

void msfp_init (void) {
  /**
     Initializes the MSF parser
  */
  seq_hook = NULL;
}

void msfp_run (LineStream ls) {
  /**
     Parses the MSF output and call registered functions.<br>
     Postcondition: the functions registered have been called
     @param[in] ls - input line stream with msf/clustal output
  */
  char *line;
  char name[100];
  int i,k,lenLine;
  int index;
  Seq *currSeq,newSeq;
  char progName[100];
  int algIsNuc = 0;
  int charRead;
  int order = 0;

  if (seq_hook != NULL) {
    if (seqs == NULL)
      seqs = arrayCreate (10,Seq);
    else {
      for (i=0;i<arrayMax (seqs);i++) {
        currSeq = arrp (seqs,i,Seq);
        hlr_free (currSeq->name);
        stringClear (currSeq->seq);
      }
      arrayClear (seqs);
    }
    if (seqPtrs == NULL)
      seqPtrs = textCreate (10);
    else
      textClear (seqPtrs);
    if (namePtrs == NULL)
      namePtrs = textCreate (10);
    else
      textClear (namePtrs);
  }
  while ((line = ls_nextLine (ls)) != NULL) {
    if (line[0] == '!' || line[0] == '\0')
      continue;
    sscanf (line,"%s",progName);
    if (strNCaseEqual (progName,"CLUSTAL",7))
      break;
    if (strstr (line,"Type: P"))
      algIsNuc = 0;
    else if (strstr (line,"Type: N"))
      algIsNuc = 1;
    if (strStartsWithC (line," Name:")) {
      sscanf (line+6,"%s",name);
      newSeq.name = hlr_strdup (name);
      if (arrayFindInsert (seqs,&newSeq,&index,(ARRAYORDERF)sortSeqs0)) {
        currSeq = arrp (seqs,index,Seq);
        currSeq->seq = stringCreate (100);
        currSeq->order = order++;
      }
      else
        hlr_free (newSeq.name);
    }
    if (strStartsWithC (line,"//"))
      break;
  }
  if (line == NULL)
    return;
  while ((line = ls_nextLine (ls)) != NULL) {
    if (line[0] == '\0')
      continue;
    sscanf (line,"%s%n",name,&charRead);
    newSeq.name = hlr_strdup (name);
    if (arrayFind (seqs,&newSeq,&index,(ARRAYORDERF)sortSeqs0))
      currSeq = arrp (seqs,index,Seq);
    else {
      currSeq = NULL;
      if (isalpha (line[0])) {
        // clustal and name not yet registered
        currSeq = arrayp (seqs,arrayMax (seqs),Seq);
        currSeq->seq = stringCreate (100);
        currSeq->name = hlr_strdup (name);
        currSeq->order = order++;
      }
    }
    hlr_free (newSeq.name);
    if (currSeq == NULL)
      continue;
    lenLine = strlen (line);
    for (k=charRead;k<lenLine;k++) {
      if (isSeqOrGapChar (line[k]))
        stringCatChar (currSeq->seq,line[k]);
    }
  }
  arraySort (seqs,(ARRAYORDERF)sortSeqs1);
  if (seq_hook) {
    for (i=0;i<arrayMax (seqs);i++) {
      currSeq = arrp (seqs,i,Seq);
      textAdd (seqPtrs,string (currSeq->seq));
      textAdd (namePtrs,currSeq->name);
    }
    if (strNCaseEqual (progName,"CLUSTAL",7)) { // try to guess type
      algIsNuc = 1;
      for (i=0;i<arrayMax (seqs);i++) {
        currSeq = arrp (seqs,i,Seq);
        if (getSeqType (string (currSeq->seq)) == 'P') {
          algIsNuc = 0;
          break;
        }
      }
    }
    (*seq_hook)(algIsNuc,arrayMax (seqs),
                &textItem (namePtrs,0),&textItem (seqPtrs,0));
  }
}
