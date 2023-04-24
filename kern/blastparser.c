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
/** @file blastparser.c
    @brief Purpose: dissect the standard output of the BLAST program.
    Module prefix bp_
*/
#include <ctype.h>
#include "log.h"
#include "format.h"
#include "hlrmisc.h"
#include "linestream.h"
#include "blastparser.h"
#include "biosdefs.h"

/// a very large eValue to indicate no hit
#define VERY_LARGE 1.0e6
/// maximum length of certain variables
#define STRING_SIZE 256
/// default length of the expect string
#define EXPECT_STRING_SIZE 20
/// maximum length of the expect string
#define EXPECT_STRING_MAX_SIZE 30

/* --------------- begin module blastparser --------------------
   Secret: knows how to dissect the output of the BLAST program

   Implementation idea:
   first the user of this module registers functions that should
   be called when interesting points in the blast output are found.
   Then the user starts the parser, which in turn drives the functions
   the user registered before, feeding these functions with
   the current data (e.g. query name, subject name, ...).

   Public functions:
   void bp_init()
   void bp_run(filename)
   int  bp_register_<name>(function); (returns 1 parsing should continue,
                                       else 0 to stop parser)
     where <name> is:
------------------------------------------------------------
name          when called /
              what delivered
------------------------------------------------------------
begin         at very beginning, if not called only the first /
              blast output in a multiquery blast will be parsed
progName      at beginning/
              BLASTP, BLASTN or similar
query         after header has been read /
              query name and length
database      after query has been read /
              database name, #sequences, #symbols
summary       after a summary line has been read /
              subject name, desc, score, expect
subjectStart  after header of a subject has been read /
              subject name, length and description
subjectEnd    when subject ends
HSPStart      when a HSP for a subject starts /
              score, P-value and N
idFrame       after HSPStart() /
              %id, %positives, queryFrame, sbjctFrame (0:frame does not apply)
HSPEnd        when a HSP ends /
              left and right ends of query and subject
HSPSeq        always after HSPEnd /
              pointer to query and sbjct sequences, match line
end           at the end of a blast output
------------------------------------------------------------

Private functions: see below, declared 'static'

rough idea of the structure of BLAST output:
- output of query1
  - title (program name)
  - header  (tells query name and length
                   database name, num seq and num letters
  - summary (best match for each subject)
  - a set of matching subjects, foreach subject:
    -- its name and length
    -- a set of matching regions (called HSPs), foreach: HSP:
       --- its P-value
       --- a set of alignment blocks, foreach block:
           ---- 3 lines:
           ---- Query: leftend seq rightend
           ----        alignment markers
           ---- Sbjct: leftend seq rightend
  - trailer (some statistics, ignored here)
[- output of query2
...]
*/

static int (*begin_hook) (void);
static int (*progName_hook) (char *name);
static int (*query_hook) (char *query,int querylen);
static int (*database_hook) (char *db,int numSeq,int numLetters);
static int (*summary_hook) (char *name,char *desc,float score,double expect);
static int (*subjectStart_hook) (char *subject,int subjectlen,char *desc);
static int (*subjectEnd_hook) (void);
static int (*HSPStart_hook) (float score,double expect,int n);
static int (*idFrame_hook) (int hspLen,int identities,int positives,int gaps,
                            int frameQ,int frameS);
static int (*HSPEnd_hook) (int sLeftEnd,int sRightEnd,
                           int qLeftEnd,int qRightEnd);
static int (*HSPSeq_hook) (char *querySeq,char *sbjctSeq,char *match);
static int (*end_hook) (void);

static int needsSeq = 0;
static int stillParsingQueryName = 0;

void bp_init (void) {
  /**
     Initilize the blast parser
  */
  begin_hook = NULL;
  end_hook = NULL;
  progName_hook = NULL;
  query_hook = NULL;
  database_hook = NULL;
  subjectStart_hook = NULL;
  subjectEnd_hook = NULL;
  HSPStart_hook = NULL;
  idFrame_hook = NULL;
  HSPEnd_hook = NULL;
  HSPSeq_hook = NULL;
  needsSeq = 0;
}

void bp_register_begin (int (*f)(void)) {
  /**
     Register a function to be called when a blast output starts
     @param[in] f - the function
  */
  begin_hook = f;
}

void bp_register_end (int (*f)(void)) {
  /**
     Register a function to be called when a blast output ends
     @param[in] f - the function
  */
  end_hook = f;
}

void bp_register_progName (int (*f)(char *programName)) {
  /**
     Register a function to be called when the blast program is known
     @param[in] f - the function
  */
  progName_hook = f;
}

void bp_register_query (int (*f)(char *query,int querylen)) {
  /**
     Register a function to be called when the query was found
     @param[in] f - the function
  */
  query_hook = f;
}

void bp_register_database (int (*f)(char *db,int numSeq,int numLetters)) {
  /**
     Register a function to be called when the database to be searched was found
     @param[in] f - the function
  */
  database_hook = f;
}

void bp_register_summary (int (*f)(char *subject,char *desc,float score,
                                   double expect)) {
  /**
     Register a function to be called when a summary line was found
     @param[in] f - the function
  */
  summary_hook = f;
}

void bp_register_subjectStart (int (*f)(char *subject,int subjectlen,
                                        char *desc)) {
  /**
     Register a function to be called when a subject sequence starts
     @param[in] f - the function
  */
  subjectStart_hook = f;
}

void bp_register_subjectEnd (int (*f)(void)) {
  /**
     Register a function to be called when a subject sequence ends
     @param[in] f - the function
  */
  subjectEnd_hook = f;
}

void bp_register_HSPStart (int (*f)(float score,double expect,int n)) {
  /**
     Register a function to be called when an HSP of a subject starts
     @param[in] f - the function
  */
  HSPStart_hook = f;
}

void bp_register_idFrame (int (*f)(int hspLen,int identities,int positives,
                                   int gaps,int queryFrame,int sbjctFrame)) {
  /**
     Register a function to be called when identity and frame information
     has been determined
     @param[in] f - the function
  */
  idFrame_hook = f;
  needsSeq = 1;
}

void bp_register_HSPEnd (int (*f)(int sLeftEnd,int sRightEnd,
                                  int qLeftEnd,int qRightEnd)) {
  /**
     Register a function to be called when an HSP of a subject ends
     @param[in] f - the function
  */
  HSPEnd_hook = f;
}

void bp_register_HSPSeq (int (*f)(char *querySeq,char *sbjctSeq,char *match)) {
  /**
     Register a function to be called when the query sequence, subject sequence
     and match line of an HSP is ready
     @param[in] f - the function
  */
  HSPSeq_hook = f;
  needsSeq = 1;
}

static char *line;
static char progName[11]; // program name
static int hspLen,identities,positives,gaps,frameQ,frameS;
static Stringa querySeq = NULL;
static Stringa sbjctSeq = NULL;
static Stringa match = NULL;
static int sLeftEnd = 0; // begin of subject sequence
static int sRightEnd = 0; // end of subject sequence
static int qLeftEnd = 0; // begin of query sequence
static int qRightEnd = 0; // end of query sequence
static int querylen = 0; // length of query sequence in bases
static int subjectlen =0; // length of subject sequence in bases

static int stringToI (char *s) {
  /**
     Same as atoi(1), except that
     string s may contain commas which are ignored,
     and the string s is destroyed
  */
  int c;
  char *from = s - 1;
  char *to = s;
  while ((c = *(++from)) != '\0' ) // remove ',' from string
    if (c != ',')
      *(to++) = c;
  *to = '\0';
  return atoi (s);
}

static void calcIDframe (int needsSeq) {
  int i;
  int qGaps,sGaps;

  if (!needsSeq)
    die ("Cannot calculate ID, positives, gap and frame information");
  hspLen = arrayMax (querySeq) - 1;
  identities = positives = gaps = qGaps = sGaps = 0;
  for (i=0;i<hspLen;i++) {
    if (isalpha (arru (match,i,char)) || arru (match,i,char) == '|')
      identities++;
    else if (arru (match,i,char) == '+')
      positives++;
  }
  positives = (int)(100.0 * (positives + identities) / hspLen + 0.5);
  identities = (int)(100.0 * identities / hspLen + 0.5);
  for (i=0;i<hspLen;i++) {
    if (arru (querySeq,i,char) == '-' || arru (sbjctSeq,i,char) == '-')
      gaps++;
    if (arru (querySeq,i,char) == '-')
      qGaps++;
    if (arru (sbjctSeq,i,char) == '-')
      sGaps++;
  }
  gaps = (int)(100.0 * gaps / hspLen + 0.5);
  if (strEqual (progName,"BLASTN")) {
    if (qRightEnd > qLeftEnd)
      frameQ = 1;
    else
      frameQ = -1;
    if (sRightEnd > sLeftEnd)
      frameS = 1;
    else
      frameS = -1;
    return;
  }
  if (qLeftEnd < qRightEnd) {
    if (arrayMax (querySeq) - 1 - qGaps == qRightEnd - qLeftEnd + 1) // protein
      frameQ = 0;
    else {
      frameQ = qLeftEnd % 3;
      if (frameQ == 0)
        frameQ = 3;
    }
  }
  else {
    frameQ = (querylen-qLeftEnd+1) % 3;
    if (frameQ == 0)
      frameQ = 3;
    frameQ = -frameQ;
  }
  if (sLeftEnd < sRightEnd) {
    if (arrayMax (sbjctSeq) - 1 - sGaps == sRightEnd - sLeftEnd + 1) // protein
      frameS = 0;
    else {
      frameS = sLeftEnd % 3;
      if (frameS == 0)
        frameS = 3;
    }
  }
  else {
    frameS = (subjectlen-sLeftEnd+1) % 3;
    if (frameS == 0)
      frameS = 3;
    frameS = -frameS;
  }
}

void bp_run (LineStream ls) {
  /**
     Parses the output of the blast program and calls user defined functions<br>
     Postcondition: the functions registered have been called
     @param[in] ls - input line stream with blast output
  */
  char query[STRING_SIZE]; // query sequence name
  char database[STRING_SIZE]; // database name
  char subject[STRING_SIZE]; // subject sequence name
  double expect; // Expect Value of current HSP
  char expectStr[EXPECT_STRING_SIZE]; // Expect value as a string in case of e.g. e-123
  char expectStr1[EXPECT_STRING_MAX_SIZE];
  float score; // blast score for a HSP
  int left,right; // temporary
  char seq[101];
  int i;
  int hspCnt; // number of HSP for current subject
  int goOn = 1; // 1 while parsing should go on, 0 to stop
  int subjectOpen = 0;
  int found;
  char *name,*desc;
  char *pos;
  char s[100];
  int numSeq,numLett;
  Stringa longDesc = NULL;
  int expectN = 0;
  int offs;

  /* the idea of this parsing schema is to 'work or fail', i.e.
     if the blast output changes, then this parser should
     break, instead of picking up wrong pieces of the output
  */
  if (needsSeq) {
    stringCreateClear (querySeq,100);
    stringCreateClear (sbjctSeq,100);
    stringCreateClear (match,100);
  }
  ls_bufferSet (ls,1);
again:
  // look for program name
  while ((line = ls_nextLine (ls)) != NULL) {
    if (strNEqual (line,"BLAST",5) || strNEqual (line+1,"BLAST",5)) {
      sscanf (line,"%10s ",progName);
      if (begin_hook != NULL)
        goOn = (*begin_hook) ();
      if (!goOn)
        return;
      if (progName_hook != NULL)
        goOn = (*progName_hook) (progName);
      if (goOn)
        break;
      else
        return;
    }
  }
  if (line == NULL)
    return;

  // look for database and size
  while ((line = ls_nextLine (ls)) != NULL) {
    if (sscanf (line,"Database: %255s",database) == 1)
      continue;
    if (sscanf (line,"%50s%n",s,&i) == 1 &&
        strNEqual (line+i," sequences;",11)) {
      numSeq = stringToI (s);
      if (sscanf (line+i+11,"%20s",s) != 1)
        die ("Parsing error in line %d",ls_lineCountGet (ls));
      numLett = stringToI (s);
      if (database_hook != NULL)
        goOn = (*database_hook) (database,numSeq,numLett);
      if (goOn)
        break;
      else
        return;
    }
  }

  if (line == NULL) { // database not found
    if (goOn && end_hook != NULL)
      goOn = (*end_hook) ();
    return;
  }

  // look for query and query length
  while ((line = ls_nextLine (ls)) != NULL) {
again1:
    if (sscanf (line,"Query=  %255s",query) == 1) {
      if (query[strlen (query)-1] == '/') // the query name is not yet complete - continued in next line
        stillParsingQueryName = 1;
      continue;
    }
    if (sscanf (line,"Length=%14s",s) == 1) {
      stillParsingQueryName = 0;
      querylen = stringToI (s);
      if (query_hook != NULL)
        goOn = (*query_hook) (query,querylen);
      if (goOn)
        break;
      else
        return;
    }
    if (stillParsingQueryName) {
      strTrim (line, " ", "");
      pos = strchr (line, ' ');
      *pos = '\0';
      if (strlen (line) + strlen (query) > STRING_SIZE)
        die ("Parsing error in line %d: query name too long",
             ls_lineCountGet (ls));
      strcat (query,line);
    }
  }

  if (line == NULL) { // query not found
    if (goOn && end_hook != NULL)
      goOn = (*end_hook) ();
    return;
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
      offs = 67;
      if (line[offs] != ' ')
        offs++;
      if (sscanf (line+offs,"%f %s",&score,expectStr) != 2)
        die ("Parsing error in line %d",ls_lineCountGet (ls));
      if (expectStr[0] == 'e')
        snprintf (expectStr1,EXPECT_STRING_MAX_SIZE, "1.0%s",expectStr);
      else
        strcpy (expectStr1,expectStr);
      if (sscanf (expectStr1,"%lf",&expect) != 1)
        die ("Parsing error in line %d",ls_lineCountGet (ls));
      line[offs] = '\0';
      name = strtok (line+2," ");
      desc = strtok (NULL,"\n");
      if (desc != NULL) {
        i = strlen (desc);
        while (i > 0 && (desc[i-1] == ' ' || desc[i-1] == '.'))
          i--;
        desc[i] = '\0';
      }
      else
        desc = hlr_strdup ("");
      if (summary_hook != NULL)
        goOn = (*summary_hook) (name,desc,score,expect);
      if (!goOn)
        return;
    }
    else if (strStartsWithC (line,"Sequences producing significant")) {
      line = ls_nextLine (ls); // skip empty line
      found = 1;
    }
    else if (strStartsWithC (line,"***** No hits found *****"))
      break;
  }
  if (!found) {
    if (summary_hook != NULL)
      goOn = (*summary_hook) ("-","no hit",0.0,VERY_LARGE);
    if (goOn && end_hook != NULL)
      goOn = (*end_hook) ();
    if (goOn) {
      if (line != NULL)
        goto again1;
      else
        die ("It seems that the Blast program crashed, please tell %s\nwhat query you used, what database(s) you searched and what parameters you changed",
             BIOS_ADMIN);
    }
    else
      return;
  }

  // look for alignments
  hspCnt = 0;
  subjectOpen = 0;
  while ((line = ls_nextLine (ls)) != NULL) {
    if (sscanf (line,">%255s%n",subject,&i) == 1) {
      if (hspCnt > 0) {
        if (idFrame_hook != NULL) {
          calcIDframe (needsSeq);
          if (! (goOn = (*idFrame_hook) (hspLen,identities,positives,gaps,
                                         frameQ,frameS)))
            return;
        }
        if (HSPEnd_hook != NULL)
          if (! (goOn = (*HSPEnd_hook) (sLeftEnd,sRightEnd,qLeftEnd,qRightEnd)))
            return;
        if (HSPSeq_hook != NULL)
          if (! (goOn = (*HSPSeq_hook) (string (querySeq),string (sbjctSeq),
                                        string (match))))
            return;
      }
      if (subjectOpen)
        if (subjectEnd_hook != NULL)
          if (! (goOn = (*subjectEnd_hook) ()))
            return;
      // new match starts
      subjectOpen = 1;
      hspCnt = 0; // so far no HSPs seen
      stringCreateClear (longDesc,100);
      if (*(line+i) != '\0')
        stringCat (longDesc,line+i+1);
      while ((line = ls_nextLine (ls)) != NULL) {
        if (sscanf (line," Length = %s",s) == 1) {
          subjectlen = stringToI (s);
          if (subjectStart_hook != NULL) {
            if (! (goOn = (*subjectStart_hook) (subject,subjectlen,
                                                string (longDesc))))
              return;
            else
              break;
          }
          else
            break;
        }
        else {
          while (*line == ' ')
            line++;
          stringCat (longDesc,line);
        }
      }
    }
    else if (sscanf (line,"Score = %f bits (%*d), Expect = %s",
                     &score,expectStr) == 2 ||
             sscanf (line,"Score = %f bits (%*d), Expect(%d) = %s",
                     &score,&expectN,expectStr) == 3 ||
             sscanf (line," Score = %f bits (%*d), Expect = %s",
                     &score,expectStr) == 2 || // newer BLAST version
             sscanf (line," Score = %f bits (%*d), Expect(%d) = %s",
                     &score,&expectN,expectStr) == 3) {
      if (expectStr[0] == 'e')
        snprintf (expectStr1, EXPECT_STRING_MAX_SIZE, "1.0%s",expectStr);
      else
        strcpy (expectStr1,expectStr);
      if (sscanf (expectStr1,"%lf",&expect) != 1)
        die ("Parsing error in line %d",ls_lineCountGet (ls));
      if (hspCnt > 0) {
        if (idFrame_hook != NULL) {
          calcIDframe (needsSeq);
          if (! (goOn = (*idFrame_hook) (hspLen,identities,positives,gaps,
                                         frameQ,frameS)))
            return;
        }
        if (HSPEnd_hook != NULL)
          if (! (goOn = (*HSPEnd_hook) (sLeftEnd,sRightEnd,qLeftEnd,qRightEnd)))
            return;
        if (HSPSeq_hook != NULL)
          if (! (goOn = (*HSPSeq_hook) (string (querySeq),string (sbjctSeq),
                                        string (match))))
            return;
      }
      hspCnt++;
      if (HSPStart_hook != NULL)
        if (! (goOn = (*HSPStart_hook) (score,expect,expectN)))
          return;
      expectN = 0;
      sLeftEnd = 0; // don't know the limits yet
      qLeftEnd = 0;
      if (needsSeq) {
        stringClear (querySeq);
        stringClear (sbjctSeq);
        stringClear (match);
      }
    }
    else if (sscanf (line,"Query %d %100s %d",&left,seq,&right) == 3 ||
             sscanf (line,"Query: %d %100s %d",&left,seq,&right) == 3) {
      if (qLeftEnd == 0)
        qLeftEnd = left;
      qRightEnd = right; // take the last one seen
      if (needsSeq)
        stringCat (querySeq,seq);
      pos = strstr (line,seq);
      line = ls_nextLine (ls); // match line
      if (needsSeq)
        stringNCat (match,pos,strlen (seq));
      line = ls_nextLine (ls); // subject line
      if (sscanf (line,"Sbjct %d %100s %d",&left,seq,&right) != 3)
        if (sscanf (line,"Sbjct: %d %100s %d",&left,seq,&right) != 3)
          die ("Parsing error in line %d: Subject expected",
               ls_lineCountGet (ls));
      if (sLeftEnd == 0)
        sLeftEnd = left;
      sRightEnd = right; // take the last one seen
      if (needsSeq)
        stringCat (sbjctSeq,seq);
    }
    else if (strStartsWithC (line,"Query="))
      break;
    else if (strStartsWithC (line,"BLAST") || strStartsWithC (line+1,"BLAST")) {
      ls_back (ls,1);
      break;
    }
  }
  if (goOn) {
    if (hspCnt > 0) { // previous match ends
      if (idFrame_hook != NULL) {
        calcIDframe (needsSeq);
        goOn = (*idFrame_hook) (hspLen,identities,positives,gaps,frameQ,frameS);
      }
      if (HSPEnd_hook != NULL)
        goOn = (*HSPEnd_hook) (sLeftEnd,sRightEnd,qLeftEnd,qRightEnd);
      if (HSPSeq_hook != NULL)
        goOn = (*HSPSeq_hook) (string (querySeq),string (sbjctSeq),
                               string (match));
    }
    if (subjectOpen)
      if (subjectEnd_hook != NULL)
        (*subjectEnd_hook) ();
  }
  if (line != NULL &&
      strStartsWithC (line,"Query=")) // new query in a multi-query Blast search
    goto again1;
  if (end_hook != NULL)
    goOn = (*end_hook) ();
  if (begin_hook != NULL)
    goto again;
}
