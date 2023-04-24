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
/** @file pearsonfastaparser.c
    @brief Parser for output of Pearson fasta programs.
    Module prefix pfp_
*/
#include "log.h"
#include "format.h"
#include "hlrmisc.h"
#include "pearsonfastaparser.h"

/// a constant to be used if no hits have been found
#define VERY_LARGE 1.0e6

/* --------------- begin module pearsonfastaparser --------------------
knows how to dissect the output of the Fasta/TFasta programs

implementation idea:
  first the user of this module registers functions that should
  be called when interesting points in the fasta output are found.
  Then the user starts the parser, which in turn drives the functions
  the user registered before, feeding these functions with
  the current data (e.g. query name, subject name, ...).

public functions:
   void pfp_init()
   void pfp_run(linestream)
   int  pfp_register_<name>(function); (returns 1 parsing should continue,
                                        else 0 to stop parser)
     where <name> is:
------------------------------------------------------------
name          when called  /
              what delivered
------------------------------------------------------------
begin         at beginning of output
progName      at beginning/
              FASTA protein, FASTA nucleotide or TFASTA
query         after header has been read /
              query name and length
database      after query has been read /
              query name and numSeq,numLetters
summary       after a summary line has been read /
              subject name, desc, queryframe, subjframe, score, bits, prob
subjectStart  after header of a subject has been read /
              subject name
alignment     when the alignment ends /
              querySegment, subjectSegment, alignment marker,
                id, overlap, queryStart,queryEnd
subjectEnd    after alignment
end           at end of output
------------------------------------------------------------

private functions: see below, declared 'static'

rough idea of the structure of FASTA/TFASTA output:
- title (program name,query name and length)
- summary (subjects with their scores)
- details about subjects, foreach subject:
  -- name
  -- scores
  -- alignments
- trailer (some statistics, ignored here)
*/

static int (*begin_hook) (void);
static int (*progName_hook) (char *progName);
static int (*query_hook) (char *query, int querylen,int queryIsNuc);
static int (*database_hook) (char *name,int numSeq,int numLetters);
static int (*summary_hook) (char *name,char *desc,int qframe,int sframe,
                            int score,float bits,double prob);
static int (*subjectStart_hook) (char *subject,int subjLen,int subjIsNuc);
static int (*subjectEnd_hook) (void);
static int (*alignment_hook) (char *querySeq,char *subjSeq,char *matchStr,
                              float id,int overlap,int queryBeg,int queryEnd,
                              int subjBeg,int subjEnd);
static int (*end_hook) (void);

void pfp_init (void) {
  /**
     Initializes the parser
  */
  begin_hook = NULL;
  progName_hook = NULL;
  query_hook = NULL;
  database_hook = NULL;
  summary_hook = NULL;
  subjectStart_hook = NULL;
  alignment_hook = NULL;
  subjectEnd_hook = NULL;
  end_hook = NULL;
}

void pfp_register_begin (int (*f)(void)) {
  /**
     Registers a function to be called when a new output begins
     @param[in] f - the function
  */
  begin_hook = f;
}

void pfp_register_progName (int (*f)(char *progName)) {
  /**
     Registers a function to be called when the progam name is found
     @param[in] f - the function
  */
  progName_hook = f;
}

void pfp_register_query (int (*f)(char *query,int querylen,int queryIsNuc)) {
  /**
     Registers a function to be called when the query is found
     @param[in] f - the function
  */
  query_hook = f;
}

void pfp_register_database (int (*f)(char *name,int numSeq,int numLetters)) {
  /**
     Registers a function to be called when the database t be seached is found
     @param[in] f - the function
  */
  database_hook = f;
}

void pfp_register_summary (int (*f)(char *subject,char *desc,
                                    int qframe,int sframe,
                                    int score,float bits,double prob)) {
  /**
     Registers a function to be called when a summary line is found
     @param[in] f - the function
  */
  summary_hook = f;
}

void pfp_register_subjectStart (int (*f)(char *subject,int subjLen,
                                         int subjIsNuc)) {
  /**
     Registers a function to be called when a subject sequence is found
     @param[in] f - the function
  */
  subjectStart_hook = f;
}

void pfp_register_alignment (int (*f)(char *querySeq,char *subjSeq,
                                      char *matchStr,
                                      float id,int overlap,
                                      int queryBeg,int queryEnd,
                                      int subjBeg,int subjEnd)) {
  /**
     Registers a function to be called when an alignment is found
     @param[in] f - the function
  */
  alignment_hook = f;
}

void pfp_register_subjectEnd (int (*f)(void)) {
  /**
     Registers a function to be called when a subject is finished
     @param[in] f - the function
  */
  subjectEnd_hook = f;
}

void pfp_register_end (int (*f)(void)) {
  /**
     Registers a function to be called when the output ends
     @param[in] f - the function
  */
  end_hook = f;
}

void pfp_run (LineStream ls) {
  /**
     Parses the output of the fasta program and calls user defined functions.<br>
     Postcondition: the functions registered have been called
     @param[in] ls - input stream with fasta output
  */
  char *pos;
  char *line;
  char progName[100]; // program name
  int queryLen,subjLen;
  char queryType[3];
  char queryName[100]; // query sequence name
  char database[100];
  int numSeq,numLetters;
  int queryBeg,queryEnd;
  char subjName[100]; // subject sequence name
  char subjType[3];
  int subjBeg,subjEnd;
  int frame;
  int queryFrame=0,subjFrame=0;
  int score;
  float bits;
  double prob;
  int overlap;
  float identity;
  static Stringa querySeq = NULL;
  static Stringa subjSeq = NULL;
  static Stringa matchStr = NULL;
  int goOn = 1; // 1 while parsing should go on, 0 to stop
  int found;
  int offset;
  char *descr;

  ls_bufferSet (ls,1);
  if (alignment_hook != NULL) {
    stringCreateClear (querySeq,100);
    stringCreateClear (subjSeq,100);
    stringCreateClear (matchStr,100);
  }
  /* Look for program name, but do not call callback function because we do
     not know whether it is nuc or pro fasta.
     The program header appears only once in the output if query is with
     multiple sequences.
     Parse it only the first time but call callback for begin and progname for
     every query as in blastparser
  */
  while ((line = ls_nextLine (ls)) != NULL) {
    if (strStartsWithC (line,"FAST") || strStartsWithC (line,"TFAST") ||
        strStartsWithC (line,"SSEARCH")) {
      sscanf (line,"%10s",progName);
      break;
    }
  }
again:
  if (line == NULL)
    return;
  stringClear (querySeq);
  stringClear (subjSeq);
  stringClear (matchStr);
  // look for query
  while ((line = ls_nextLine (ls)) != NULL) {
    if ((pos = strstr (line,">>>")) != NULL) {
      sscanf (pos+3,"%99s",queryName);
      pos = strrchr (line,'-');
      if (!pos)
        die ("pearsonfastaparser: - not found in line %d",
             ls_lineCountGet (ls));
      sscanf (pos+1,"%d %2s",&queryLen,queryType);
      break;
    }
  }
  if (line == NULL)
    return;
  if (begin_hook != NULL)
    goOn = (*begin_hook)();
  if (!goOn)
    return;
  if (progName_hook != NULL) {
    if (strEqual (progName,"FASTA")) {
      if (strStartsWithC (queryType,"nt"))
        strcat (progName," Nucleotide");
      else
        strcat (progName," Protein");
    }
    goOn = (*progName_hook)(progName);
  }
  if (!goOn)
    return;
  if (query_hook != NULL)
    goOn = (*query_hook)(queryName,queryLen,
                         strStartsWithC (queryType,"nt") ? 1 : 0);
  if (!goOn)
    return;
  if (line == NULL)
    return;
  // look for database and size
  line = ls_nextLine (ls);
  if (!strStartsWithC (line,"Library: "))
    die ("pearsonfastaparser: database not found in line %d",
         ls_lineCountGet (ls));
  sscanf (line+9,"%99s",database);
  while ((line = ls_nextLine (ls)) != NULL) {
    if (strstr (line,"residues in")) {
      if (2 != sscanf (line,"%d residues in %d sequences",&numLetters,&numSeq))
        die ("pearsonfastaparser: numSeq and numLetters not found line %d",
             ls_lineCountGet (ls));
      if (database_hook != NULL)
        goOn = (*database_hook)(database,numSeq,numLetters);
      if (goOn)
        break;
      else
        return;
    }
  }
  // look for summary lines
  found = 0;
  while ((line = ls_nextLine (ls)) != NULL) {
    if (found) {
      if (line[0] == '\0') {
        if (goOn)
          break;
        else
          return;
      }
      pos = strrchr (line,'(');
      if (!pos)
        die ("pearsonfastaparser: end of description not found in line %d",
             ls_lineCountGet (ls));
      while (*(pos-1) == ' ')
        pos--;
      *pos = '\0';
      pos = strrchr (pos+1,')');
      if (!pos)
        die ("pearsonfastaparser: last parenthesis not found in line %d",
             ls_lineCountGet (ls));
      pos++;
      while (*pos == ' ')
        pos++;
      if (sscanf (pos,"[%d] %d %f %lf",&frame,&score,&bits,&prob) != 4) {
        char fr;

        frame = 0;
        if (sscanf (pos,"[%c] %d %f %lf",&fr,&score,&bits,&prob) != 4) {
          frame = 0;
          if (sscanf (pos,"%d %f %lf",&score,&bits,&prob) != 3)
            die ("pearsonfastaparser: could not parse scores in line %d",
                 ls_lineCountGet (ls));
        }
        else {
          if (fr == 'f')
            frame = 1;
          else if (fr == 'r')
            frame = -1;
          else
            die ("pearsonfastaparser: unexpected [%c]",fr);
        }
      }
      pos = strchr (line,' ');
      if (!pos) // means no description
        descr = "";
      else {
        *pos = '\0';
        descr = pos+1;
      }
      if (strEqual (progName,"FASTA Nucleotide")) {
        queryFrame = frame;
        subjFrame = 1;
      }
      else if (strEqual (progName,"FASTA Protein")) {
        queryFrame = 0;
        subjFrame = 0;
      }
      else if (strEqual (progName,"FASTX") || strEqual (progName,"FASTY")) {
        queryFrame = frame;
        subjFrame = 0;
      }
      else if (strEqual (progName,"TFASTX") || strEqual (progName,"TFASTY")) {
        queryFrame = 0;
        subjFrame = frame;
      }
      if (summary_hook != NULL)
        goOn = (*summary_hook)(line,descr,queryFrame,subjFrame,score,bits,prob);
      if (!goOn)
        return;
    }
    else if (strStartsWithC (line,"The best scores are:")) {
      found = 1;
    }
    else if (strStartsWithC (line,"!! No sequences"))
      break;
  }
  if (!found) {
    if (summary_hook != NULL)
      goOn = (*summary_hook)("-","no hit",0,0,0,0.0,VERY_LARGE);
   if (goOn && end_hook != NULL)
      goOn = (*end_hook)();
    if (goOn && begin_hook != NULL)
      goto again;
    else
      return;
  }
  // look for alignments
  while ((line = ls_nextLine (ls)) != NULL) {
    if (strStartsWithC (line,"Function used was") ||
        strStartsWithC (line,">>") ||
        strstr (line,"residues in") || strstr (line,">>>")) {
      if (stringLen (querySeq) > 0 && alignment_hook != NULL) {
        pos = string (matchStr) + stringLen (matchStr);
        while (*(pos-1) == ' ')
          pos--;
        offset = pos - string (matchStr);
        *(string (matchStr) + offset) = '\0';
        *(string (querySeq) + offset) = '\0';
        *(string (subjSeq) + offset) = '\0';
        pos = string (matchStr);
        while (*pos == ' ')
          pos++;
        offset = pos - string (matchStr);
        goOn = (*alignment_hook) (string (querySeq)+offset,string (subjSeq)+offset,
                                  string (matchStr)+offset,
                                  identity,overlap,queryBeg,queryEnd,subjBeg,subjEnd);
        if (!goOn)
          break;
        if (subjectEnd_hook != NULL) {
          goOn = (*subjectEnd_hook) ();
          if (!goOn)
            break;
        }
      }
      if (strstr (line,"residues in") || strstr (line,">>>")) {
        if (strstr (line,">>>"))
          ls_back (ls,1);
        break;
      }
    }
    if (strStartsWithC (line,"Function used was"))
      break;
    else if (strStartsWithC (line,">>")) {
      sscanf (line+2,"%99s",subjName);
      pos = strrchr (line,'(');
      if (!pos)
        die ("pearsonfastaparser: subject length not found in line %d",
             ls_lineCountGet (ls));
      sscanf (pos+1,"%d %2s",&subjLen,subjType);
      /* type is aa in case of translated nucleotide sequence! */
      if (strStartsWithC (progName,"TFAST") && strStartsWithC (subjType,"aa"))
        strcpy (subjType,"nt");
      else if ((strStartsWithC (progName,"FASTX") ||
                strStartsWithC (progName,"FASTY")) &&
               strStartsWithC (subjType,"nt"))
        strcpy (subjType,"aa");
      if (subjectStart_hook != NULL)
        goOn = (*subjectStart_hook)(subjName,subjLen,
                                    strStartsWithC (subjType,"nt") ? 1 : 0);
      if (!goOn)
        return;
      line = ls_nextLine (ls);
      line = ls_nextLine (ls);
      pos = strstr (line," identity");
      if (!pos)
        die ("pearsonfastaparser: identity not found in line %d",
             ls_lineCountGet (ls));
      while (*(pos-1) != ' ')
        pos--;
      sscanf (pos,"%f",&identity);
      pos = strstr (line," overlap");
      if (!pos)
        die ("pearsonfastaparser: overlap not found in line %d",
             ls_lineCountGet (ls));
      while (*(pos-1) != ' ')
        pos--;
      pos--; /* blank */
      while (*(pos-1) != ' ')
        pos--;
      sscanf (pos,"%d",&overlap);
      pos = strrchr (line,'(');
      if (!pos)
        die ("pearsonfastaparser: begins and ends not found in line %d",
             ls_lineCountGet (ls));
      sscanf (pos,"(%d-%d:%d-%d)",&queryBeg,&queryEnd,&subjBeg,&subjEnd);
      stringClear (querySeq);
      stringClear (subjSeq);
      stringClear (matchStr);
    }
    else if (line[0] == '\0')
      ;
    else {
      if (line[0] != ' ') {
        while ((line = ls_nextLine (ls)) != NULL) {
          if (line[0] == '\0')
            break;
        }
        continue;
      }
      line = ls_nextLine (ls); // query sequence
      if (line[0] == ' ' || line[0] == '\0') {
        while ((line = ls_nextLine (ls)) != NULL) {
          if (line[0] == '\0')
            break;
        }
        continue;
      }
      pos = strchr (line,' ');
      offset = pos-line+1;
      stringCat (querySeq,line+offset);
      line = ls_nextLine (ls); // match line
      if (line[0] != ' ')
        continue;
      stringCat (matchStr,line+offset);
      line = ls_nextLine (ls); // subject sequence
      if (line[0] == ' ')
        continue;
      stringCat (subjSeq,line+offset);
      line = ls_nextLine (ls); // number line
    }
  }
  if (end_hook != NULL)
    goOn = (*end_hook)();
  if (begin_hook != NULL)
    goto again;
}
