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
/** @file phraplightparser.c
   @brief Purpose: dissect the output of the PHRAP program in .ace files.
   Module prefix phrlp_
*/
#include "log.h"
#include "hlrmisc.h"
#include "phraplightparser.h"

/* ------------ begin module phraplightparser --------------------------------
   implementation idea:
      First the user of this module registers functions that should
      be called whenever interesting information in the program output
      has been found. Afterwards the user starts the parser, which in
      turn calls the functions the user registered before.

   public functions:
      void phrlp_init()
      void phrlp_run(LineStream)
      int phrlp_register_<name>(function); (returns 1 parsing should
                                            continue, else 0 to stop)
      where <name> is:
----------------------------------------------------------------------
      contig         after complete contig has been read
                     #reads, contig name, consensus sequence,
                     offsets of reads relative to consensus,
                     names of reads, gapped read sequences, for each
                     read the direction it appears in the contig
----------------------------------------------------------------------
*/

static int (*contig_hook)(int nReads,char *contigName,char *consensus,
                          int *offsets,char **readNames,char **segments,
                          int *revs);

static int (*contigQual_hook)(int nReads,char *contigName,char *consensus,
                              char *consensusQual,
                              int *offsets,char **readNames,char **segments,
                              int *revs);

void phrlp_register_contig (int (*f)(int nReads,char *contigName,
                                     char *consensus,
                                     int *offsets,char **readNames,
                                     char **segments,int *revs)) {
  /**
     Registers a function to be called when a contig is found
     @param[in] f - the function
  */
  contig_hook = f;
}

void phrlp_register_contigWithQual (int (*f)(int nReads,char *contigName,
                                             char *consensus,
                                             char *consensusQual,
                                             int *offsets,char **readNames,
                                             char **segments,int *revs)) {
  /**
     Registers a function to be called when a contig with quality is found
     @param[in] f - the function
  */
  contigQual_hook = f;
}

/// structure to hold one read
typedef struct {
  char *name; //!< name of read
  int offset; //!< offset of read relative to contig
  int rev; //!< whether read is reverse
  Stringa seq; //!< sequence of read
}Read;

static Array reads = NULL;
static char cName[20];
static Stringa cSeq = NULL;

void phrlp_init (void) {
  /**
     Initializes the parser
  */
  int i;
  Read *currRead;

  if (reads == NULL)
    reads = arrayCreate (10,Read);
  else {
    for (i=0;i<arrayMax (reads);i++) {
      currRead = arrp (reads,i,Read);
      hlr_free (currRead->name);
      stringClear (currRead->seq);
    }
    arrayClear (reads);
  }
  stringCreateClear (cSeq,1000);
  cName[0] = '\0';
  contig_hook = NULL;
  contigQual_hook = NULL;
}

static int returnContig (void) {
  int nReads;
  int *offsets;
  char **readNames;
  char **segments;
  int *revs;
  int i;
  Read *currRead;
  int goOn;

  nReads = arrayMax (reads);
  offsets = (int *)hlr_calloc (nReads,sizeof (int));
  readNames = (char **)hlr_calloc (nReads,sizeof (char *));
  segments = (char **)hlr_calloc (nReads,sizeof (char *));
  revs = (int *)hlr_calloc (nReads,sizeof (int));
  for (i=0;i<arrayMax (reads);i++) {
    currRead = arrp (reads,i,Read);
    offsets[i] = currRead->offset;
    readNames[i] = currRead->name;
    segments[i] = string (currRead->seq);
    revs[i] = currRead->rev;
  }
  goOn = (*contig_hook)(nReads,cName,string (cSeq),offsets,readNames,
                        segments,revs);
  hlr_free (offsets);
  hlr_free (readNames);
  hlr_free (segments);
  hlr_free (revs);
  return goOn;
}

void phrlp_run (LineStream ls) {
  /**
     Runs the parser and calls the registered functions
     @param[in] ls - line stream over the phrap output
  */
  int goOn;
  char *line;
  int nContigs,totReads;
  int cBases,cNumReads,cNumSegments;
  char cStrand;
  int i;
  Read *currRead = malloc(sizeof(Read));

  while ((line = ls_nextLine (ls)) != NULL) {
    if (strStartsWithC (line,"AS")) {
      if (2 != sscanf (line,"AS %i %i",&nContigs,&totReads))
        die ("%s(l.%i): Format error in line %i:'%s'",
             __FILE__,__LINE__,ls_lineCountGet(ls),line);
    }
    else if (strStartsWithC (line,"CO")) {
      if (contig_hook != NULL && reads != NULL && arrayMax (reads) > 0) {
        goOn = returnContig ();
        if (!goOn)
          break;
      }
      if (5 != sscanf (line,"CO %19s %i %i %i %c",
                       cName,&cBases,&cNumReads,&cNumSegments,&cStrand))
        die ("%s(l.%i): Format error in line %i:'%s'",
             __FILE__,__LINE__,ls_lineCountGet(ls),line);
      stringCreateClear (cSeq,1000);
      while ((line = ls_nextLine (ls)) != NULL) {
        if (line[0] == '\0')
          break;
        stringCat (cSeq,line);
      }
      if (stringLen (cSeq) != cBases)
        die ("Contig is %d bases long,should be %d",stringLen (cSeq),cBases);
      if (reads == NULL)
        reads = arrayCreate (10,Read);
      else {
        for (i=0;i<arrayMax (reads);i++) {
          currRead = arrp (reads,i,Read);
          hlr_free (currRead->name);
          stringClear (currRead->seq);
        }
        arrayClear (reads);
      }
    }
    else if (strStartsWithC (line,"BQ")) {
      while ((line = ls_nextLine (ls)) != NULL) {
        if (line[0] == '\0')
          break;
      }
    }
    else if (strStartsWithC (line,"AF")) {
      char tempStr[50];
      char oneChar;
      int offs;

      if (3 != sscanf (line,"AF %49s %c %i",tempStr,&oneChar,&offs))
        die ("%s(l.%i): Format error in line %i:'%s'",
             __FILE__,__LINE__,ls_lineCountGet(ls),line);
      currRead = arrayp (reads,arrayMax (reads),Read);
      currRead->name = hlr_strdup (tempStr);
      currRead->rev = (oneChar == 'U' ? 0 : 1);
      currRead->offset = offs;
    }
    else if (strStartsWithC (line,"BS") || strStartsWithC (line,"QA") ||
             strStartsWithC (line,"DS")) {
    }
    else if (strStartsWithC (line,"RD")) {
      char tempStr[50];
      int len;

      if (2 != sscanf (line,"RD %49s %i",tempStr,&len))
        die ("%s(l.%i): Format error in line %i:'%s'",
             __FILE__,__LINE__,ls_lineCountGet(ls),line);
      for (i=0;i<arrayMax (reads);i++) {
        currRead = arrp (reads,i,Read);
        if (strEqual (currRead->name,tempStr))
          break;
      }
      if (i==arrayMax (reads))
        die ("Unknown read %s",tempStr);
      if (currRead->seq == NULL)
        currRead->seq = stringCreate (100);
      while ((line = ls_nextLine (ls)) != NULL) {
        if (line[0] == '\0')
          break;
        stringCat (currRead->seq,line);
      }
    }
    else if (strStartsWithC (line,"WA") || strStartsWithC (line,"CT") ||
             strStartsWithC (line,"RT")) {
      while ((line = ls_nextLine (ls)) != NULL) {
        if (line[0] == '\0')
          break;
      }
    }
  }
  if (contig_hook != NULL && reads != NULL && arrayMax (reads) > 0)
    returnContig ();
}
