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
/** @file hmmparser.c
    @brief Purpose: dissect the output of the HMMSCAN or HMMSEARCH programs.
    Module prefix hmmp_
*/
#include "log.h"
#include "format.h"
#include "linestream.h"
#include "hlrmisc.h"

/* --------------- begin module hmmparser --------------------
secret: knows how to dissect the output of the HMMSCAN
        or HMMSEARCH programs

implementation idea:
  first the user of this module registers functions that should
  be called when interesting points in the program output are found.
  Then the user starts the parser, which in turn drives the functions
  the user registered before, feeding these functions with
  the current data (e.g. query name, HMM name, ...).

public functions:
   void hmmp_init()
   void hmmp_run(filename)
   int  hmmp_register_<name>(function); (returns 1 parsing should continue,
                                       else 0 to stop parser)
     where <name> is:
------------------------------------------------------------
name          when called  /
              what delivered
------------------------------------------------------------
progName      at beginning/
              HMMSCAN or HMMSEARCH
database      after HMM or sequence database has been read /
              database name
query         after sequence or HMM file name has been read /
              query name
domainAli     after a domain summary line has been read /
              HMM or sequence name, current domain number,
              match structure, score, prob
summary       after summary of a hit has been read /
              HMM or sequence name, desc, score, prob, #domains
domain        when a domain summary has been read /
              name, score, E-value, current domain number,
              maximum domain number, pos. of match in query,
              match topologies (query & HMM)
domainSeq     always after domainAli/
              pointer to query and HMM sequences and match line
------------------------------------------------------------
rough idea of the structure of HMMSCAN output:
- title (program name and short description)
- header  (tells HMM database name and query sequence name)
- summary
  -- best matching HMMs for query sequence
     (name, description, score, E-value, #domains)
  -- best matching domains
     (name, numbering, pos. in sequence, pos. in HMM, score, E-value)
- (possibly) domain alignment part
  -- HMM name, numbering, pos. in sequence, score, E-value
  -- a set of alignment blocks, foreach block:
     --- 3 lines:
     ---   HMM: consensus_seq
     ---        alignment markers
     --- Query: name leftend seq rightend
- (possibly) some trailing information on used parameters etc. (not used)

rough idea of the structure of HMMSEARCH output:
- maybe a leading section produced by HMMBUILD, separated by
  a line with "= = = = = = = = = " (if derived from web interface)
- title (program name and short description)
- header  (tells sequence database name and query HMM name)
- summary
  -- best matching database sequences for query HMM
     (name, description, score, E-value, #domains)
  -- best matching domains
     (name, numbering, pos. in sequence, pos. in HMM, score, E-value)
- (possibly) domain alignment part
  -- sequence name, numbering, pos. in sequence, score, E-value
  -- a set of alignment blocks, foreach block:
     --- 3 lines:
     ---   HMM: consensus_seq
     ---        alignment markers
     --- Query: name leftend seq rightend
- (possibly) some trailing information: histogram, used parameters etc. (not used)
*/

/// possible state of the parser
#define waitForAlignments      0x1
/// possible state of the parser
#define waitForDomainSummary   0x2
/// possible state of the parser
#define waitForEndOfInput      0x4
/// possible state of the parser
#define waitForAlignmentHeader 0x8
/// possible state of the parser
#define waitForHMMConsensus    0x10
/// possible state of the parser
#define waitForHMM             0x20
/// possible state of the parser
#define waitForName            0x40
/// possible state of the parser
#define waitForSeq             0x80
/// possible state of the parser
#define waitForQueryAlignment  0x100
/// possible state of the parser
#define waitForSummary         0x200
/// possible state of the parser
#define waitForMatchLine       0x400

static int nextState;
static char *line;
static Stringa querySeq = NULL;
static Stringa hmmSeq = NULL;
static Stringa matchSeq = NULL;

static int (*begin_hook) (void);
static int (*end_hook) (void);
static int (*progName_hook) (char *name);
static int (*query_hook) (char *query);
static int (*db_hook) (char *db);
static int (*summary_hook) (char *name,char *desc,float score,double prob,
                            float bias,int domainNr);
static int (*domain_hook) (char *name,float score,double prob,int currDomainNr,
                           int seqLeftEnd,int seqRightEnd,
                           int hmmLeftEnd,int hmmRightEnd,
                           char seqTopo[2],char hmmTopo[2]);
static int (*domainAli_hook) (char *name,float score,double prob,
                              int currDomainNr,
                              int seqLeftEnd,int seqRightEnd);
static int (*domainSeq_hook) (char *querySeq,char *hmmSeq,char *matchSeq);

void hmmp_register_begin (int (*f)(void)) {
  /**
     Register a function to be called when the HMM output starts
     @param[in] f - the function
  */
  begin_hook = f;
}

void hmmp_register_end (int (*f)(void)) {
  /**
     Register a function to be called when the HMM output ends
     @param[in] f - the function
  */
  end_hook = f;
}

void hmmp_register_progName (int (*f)(char *progName)) {
  /**
     Register a function to be called when the program name is found
     @param[in] f - the function
  */
  progName_hook = f;
}

void hmmp_register_query (int (*f)(char *query)) {
  /**
     Register a function to be called when the query is known
     @param[in] f - the function
  */
  query_hook = f;
}

void hmmp_register_db (int (*f)(char *db)) {
  /**
     Register a function to be called when the database to be searched is known
     @param[in] f - the function
  */
  db_hook = f;
}

void hmmp_register_summary (int (*f)(char *name,char *descr,
                                     float score,double prob,
                                     float bias,int domainNr)) {
  /**
     Register a function to be called when a summary line is found
     @param[in] f - the function
  */
  summary_hook = f;
}

void hmmp_register_domain (int (*f)(char *name,float score,double prob,
                                    int currDomainNr,
                                    int seqLeftEnd,int seqRightEnd,
                                    int hmmLeftEnd,int hmmRightEnd,
                                    char seqTopo[2],char hmmTopo[2])) {
  /**
     Register a function to be called when a domain is found
     @param[in] f - the function
  */
  domain_hook = f;
}

void hmmp_register_domainAli (int (*f)(char *name,float score,double prob,
                                       int currDomainNr,
                                       int seqLeftEnd,int seqRightEnd)) {
  /**
     Register a function to be called when an alignment is found
     @param[in] f - the function
  */
  domainAli_hook = f;
}

void hmmp_register_domainSeq (int (*f)(char *querySeq,char *hmmSeq,
                                       char *matchSeq)) {
  /**
     Register a function to be called when the sequence of a domain is found
     @param[in] f - the function
  */
  domainSeq_hook = f;
}

void hmmp_init (void) {
  /**
     Initializes the HMM parser
  */
  begin_hook = NULL;
  progName_hook = NULL;
  query_hook = NULL;
  summary_hook = NULL;
  db_hook = NULL;
  domain_hook = NULL;
  domainAli_hook = NULL;
  domainSeq_hook = NULL;
  end_hook = NULL;
}

static int handleSummaryLine (char *line,int *goOn) {
  /**
     Check if line is a summary line and if yes call the user hook;
     @param[out] goOn - flag set by user hook, if present,
                        else unchanged; also set if 'line'
                        indicates end of usable output
     @return 1 - if line is a summary line, else 0
  */
  float score;
  double prob;
  int domainNr;
  float bias;
  char *name;
  char *descr;
  char *scanline;

  if (strstr (line,"[No hits detected that satisfy reporting thresholds]")) {
    *goOn = 0; // unsuccessful hmmscan or hmmsearch --> terminate run
    nextState = 0;
    return 0;
  }
  /* other possible lines:
     [no more scores below E threshold]
     or
     [no more scores below T threshold]
     will be captured by the strlen check below.
  */

  /*
    hmmscan:
    0         1         2         3         4         5         6         7
    0123456789012345678901234567890123456789012345678901234567890123456789012345
    PH       PF00169 [Fragment-mode] PH (pleckstrin  homo   110.0    1.4e-31   1
    SH3      PF00018 [Fragment-mode] Src homology domain     43.6    2.3e-10   1
    C2       PF00168 [Fragment-mode] C2 domain               19.7    0.00071   1

    hmmsearch:
    old format:
    0         1         2         3         4         5         6         7
    0123456789012345678901234567890123456789012345678901234567890123456789012345
    SPRGP:AG0316 ABC TRANSPORTER, ATP-BINDING PROTEIN. [A  -236.5          8   1
    SPRGP:AG0328 FUNCTION UNKNOWN. . [BACILLUS SUBTILIS].  -281.3          8   1
    SPRGP:AG0223 YTJA. [BACILLUS SUBTILIS]. 0/0. 0/0       -314.5          8   1

    new format:
    0         1         2         3         4         5         6         7
    0123456789012345678901234567890123456789012345678901234567890123456789012345
        E-value  score  bias    E-value  score  bias    exp  N
          8e-56  172.7   0.6      3e-16   45.9   0.5    9.5  9  SEV      7less
        4.2e-34  109.0   0.6    5.5e-33  105.4   0.0    2.0  2  HBB_RABIT
    NOTE: first line entry (name) may be longer than 8 characters;
      - description will then be shortened to stop at pos. 52.
*/
  if (strlen (line) > 60 &&
      sscanf (line+4,"%lf %f %f",&prob,&score,&bias) == 3 &&
      sscanf (line+57,"%d",&domainNr) == 1) {
    scanline = hlr_strdup (line+60);
    name = strtok (scanline," ");
    descr = name + strlen (name)+1;
    /*scanline[53] = '\0';*/ // separate descr from scores
    if (summary_hook != NULL)
      *goOn = (*summary_hook) (name,descr,score,prob,bias,domainNr);
    hlr_free (scanline);
    return 1;
  }
  return 0;
}

static int handleDomainSummaryLine (char *line,char *name,int *goOn) {
  /**
     Check if line is a domain summary line and if yes call the user hook;
     @param[out] goOn - flag set by user hook, if present,
                        else unchanged; also set if 'line'
                        indicates end of usable output
     @return 1 if line is a domain summary line, else 0
  */
  float score;
  float bias;
  double cprob;
  double prob;
  int currDomainNr;
  char passed[2];
  int seqLeftEnd = 0;
  int seqRightEnd = 0;
  int hmmLeftEnd = 0;
  int hmmRightEnd = 0;
  int envLeftEnd = 0;
  int envRightEnd = 0;
  char S_topology [3];
  char H_topology [3];
  char E_topology [3];
  float accuracy;
  char *scanline;

  if (strstr (line,"[No targets detected that satisfy reporting thresholds]")) {
    *goOn = 0; /* unsuccessful hmmscan --> terminate run; should never happen
                  here because this situation is caught by handleSummaryLine */
    nextState = 0;
    return 0;
  }
  /* other possible lines:
     [no more scores below domE threshold]
     or
     [no more scores below domT threshold]
     will be captured by the strlen check below.
  */

  /*
    hmmscan:
    0         1         2         3         4         5         6
    012345678901234567890123456789012345678901234567890123456789012345
    SH2        1/2     181   256 ..     1    79 []   112.1  1.2e-37
    SH3        1/1     282   339 ..     1    57 []    43.6  2.3e-10
    SH2        2/2     351   426 ..     1    79 []   116.8  2.8e-39
    PH         1/1     474   577 ..     1   112 []   110.0  1.4e-31
    C2         1/1     596   650 ..     1    68 [.    19.7  0.00071

    new:
    same format as hmmsearch

    hmmsearch:
    old:
    0         1         2         3         4         5         6
    01234567890123456789012345678901234567890123456789012345678901234567
    SPRGP:AG0316   1/1      41   257 ..     1   296 []  -236.5        8
    SPRGP:AG0328   1/1       1   110 [.     1   296 []  -281.3        8
    SPRGP:AG0223   1/1       1    78 [.     1   296 []  -314.5        8

    new:
    0         1         2         3         4         5         6
    01234567890123456789012345678901234567890123456789012345678901234567
       #    score  bias  c-Evalue  i-Evalue hmmfrom  hmm to    alifrom  ali to    envfrom  env to     acc
     ---   ------ ----- --------- --------- ------- -------    ------- -------    ------- -------    ----
       1 ?   -1.0   0.0      0.13      0.13      60      73 ..     396     409 ..     395     411 .. 0.85
       2 !   40.8   0.0   1.2e-14   1.2e-14       1      83 [.     439     520 ..     439     521 .. 0.95
       3 !   14.3   0.0   2.1e-06   2.1e-06      14      83 ..     838     912 ..     827     914 .. 0.72

       NOTE: first line entry (name) may be longer than 8 characters;
       - rest of line will be shifted to the right accordingly.
  */
  if (strlen (line) > 60 && (strchr (line,'?') || strchr (line,'!'))) {
    scanline = hlr_strdup (line);
    // extract the remaining information
    if (sscanf (scanline,"%d %s %f %f %lf %lf %d %d %s %d %d %s %d %d %s %f",
                &currDomainNr,passed,&score,&bias,&cprob,&prob,
                &hmmLeftEnd,&hmmRightEnd,H_topology,
                &seqLeftEnd,&seqRightEnd,S_topology,
                &envLeftEnd,&envRightEnd,E_topology,&accuracy) != 16) {
      return 0;
    }
    if (domain_hook != NULL)
      *goOn = (*domain_hook) (name,score,prob,currDomainNr,
                              seqLeftEnd,seqRightEnd,hmmLeftEnd,hmmRightEnd,
                              S_topology,H_topology);
    hlr_free (scanline);
    return 1;
  }
  return 0;
}

static int handleDomainStart (char *line,char *name,int *goOn) {
  /**
     Check if line is an alignment summary line and if yes call the user hook;
     @param[out] goOn - flag set by user hook, if present,
                        else unchanged; also set if 'line'
                        indicates end of usable output
     @return 1 if line is an alignment summary line, else 0
  */
  float score;
  double prob;
  int currDomainNr;
  int seqLeftEnd = 0;
  int seqRightEnd = 0;
  char *scanline;

  if (strstr (line,"[no hits above thresholds]") ||
      strstr (line,"[output cut off") ||
      strstr (line,"[no more alignments")) {
    *goOn = 0; // no more alignment output to read
    nextState = 0;
    return 0;
  }
  /*
    hmmscan:
    0         1         2         3         4         5         6
    012345678901234567890123456789012345678901234567890123456789012345
    SH2: domain 1 of 2, from 181 to 256: score 112.1, E = 1.2e-37

    hmmsearch:
    old:
    0         1         2         3         4         5         6
    0123456789012345678901234567890123456789012345678901234567890123456789
    SPRGP:SP0932: domain 1 of 1, from 1 to 247: score 532.0, E = 1.5e-157

    new:
    0         1         2         3         4         5         6
    0123456789012345678901234567890123456789012345678901234567890123456789
      == domain 1    score: -1.0 bits;  conditional E-value: 0.13

      NOTE: first line entry (name) may be longer than 8 characters
      - rest of line will be shifted to the right accordingly.
  */
  if (strNEqual (line,"  == domain",11)) {
    scanline = hlr_strdup (line);
    sscanf (scanline,"   == domain %d    score: %f bits;  conditional E-value: %lf",
            &currDomainNr,&score,&prob);
    /* before we call domainAli for the next alignment block, we take care
       of the sequences from the last one - in case domainSeq is required */
    if (domainSeq_hook != NULL && stringLen (querySeq) > 0) { // for the first domain, there will not yet be a sequence
      if (!(*goOn = (*domainSeq_hook) (string (querySeq),string (hmmSeq),
                                       string (matchSeq))))
        printf ("Problem with domainSeq!\n");
    }
    if (domainAli_hook != NULL)
      *goOn = (*domainAli_hook) (name,score,prob,currDomainNr,
                                 seqLeftEnd,seqRightEnd);
    hlr_free (scanline);
    return 1;
  }
  return 0;
}

static void expect (int state,LineStream ls) {
  /**
     Checks whether the parser is in the expected state.<br>
     Precondition: the parse is initialized (see queryMatchExtract())
     nextState can be an OR combination, so it can specify a set of
     allowed states
  */
  if (! (state & nextState)) {
    die ("Parsing problem in line no %d,\naccording to the current line, the parser should be in state 0x%x,\nbut actually it is in state(s) 0x%x, current line:\n%s",
         ls_lineCountGet (ls),state,nextState,line);
  }
}

void hmmp_run (LineStream ls) {
  /**
     Parses the output file and calls user defined functions.<br>
     Postcondition: the functions registered have been called
     @param[in] ls - input line stream with HMMER output
  */
  char progName[20]; // program name
  char query[256]; // query name
  char db[256]; // database name
  char seq[101];
  int goOn = 1; // 1 while parsing should go on, 0 to stop
  int matchFound = 0; // 1 if at least one matching HMM has been found
  char *cp;
  char name[100]; // sequence or domain name
  char *tmpline;
  int pos=0;
  int len=0;

  /* the idea of this parsing schema is to 'work or fail', i.e.
     if the hmmscan output changes, then this parser should
     break, instead of picking up wrong pieces of the output
  */
  nextState = waitForName;
  if (domainSeq_hook != NULL) {
    stringCreateClear (querySeq,100);
    stringCreateClear (hmmSeq,100);
    stringCreateClear (matchSeq,100);
  }
  while (goOn && (line = ls_nextLine (ls))) {
    if (strStartsWithC (line,"//") ||
        strStartsWithC (line,"Internal pipeline "))
      break;
    if (nextState == waitForName) {
      if (strStartsWithC (line,"# hmm")) {
        sscanf (line+2,"%19s ",progName);
        if (strEqual (progName, "hmmbuild")) { // WWW output file
          while ((line = ls_nextLine (ls)) && !strStartsWithC (line,"# hmm"));
          sscanf (line+2,"%19s ",progName);
        }
        if (progName_hook != NULL)
          goOn = (*progName_hook) (progName);
      }
      nextState = waitForHMM;
      continue;
    }
    if (strstr (line,"HMM file:")) {
      expect (waitForHMM,ls);
      // hmmsearch
      sscanf (line,"# query HMM file:  %255s",query);
      if (query_hook != NULL)
        goOn = (*query_hook) (query);
      nextState = waitForSeq;
      continue;
    }
    if (strstr (line,"# target HMM database")) {
      expect (waitForHMM,ls);
      // for hmmscan
      sscanf (line,"# target HMM database:  %255s",db);
      if (db_hook != NULL)
        goOn = (*db_hook) (db);
      nextState = waitForSeq;
      continue;
    }
    if (sscanf (line,"# query sequence file:  %255s",query) == 1) {
      // for hmmscan
      expect (waitForSeq,ls);
      if (query_hook != NULL)
        goOn = (*query_hook) (query);
      nextState = waitForSummary;
      continue;
    }
    if (sscanf (line,"# target sequence database:  %255s",db) == 1) {
      // for hmmsearch
      expect (waitForSeq,ls);
      if (db_hook != NULL)
        goOn = (*db_hook) (db);
      nextState = waitForSummary;
      continue;
    }
    if ((nextState & waitForSummary) && handleSummaryLine (line,&goOn)) {
      if (!goOn)
        continue;
      matchFound = 1;
      nextState = waitForDomainSummary | waitForSummary;
      continue;
    }
    if (matchFound && (strstr (line,">>"))) {
      if (domainSeq_hook != NULL && stringLen (querySeq)) { // print leftover domain seq if necessary
        if (!(goOn = (*domainSeq_hook) (string (querySeq),string (hmmSeq),
                                        string (matchSeq))))
          printf ("Problem with domainSeq!\n");
        stringClear (querySeq);
      }
      if (sscanf (line+2,"%99s ",name) == 1) {
        nextState = waitForDomainSummary;
        continue;
      }
    }
    if ((nextState & waitForDomainSummary) &&
        handleDomainSummaryLine (line,name,&goOn)) {
      if (!goOn)
        continue;
      nextState = waitForDomainSummary | waitForAlignments;
      continue;
    }
    if (matchFound && strstr (line,"Alignments for each domain")) {
      expect (waitForAlignments,ls);
      nextState = waitForAlignmentHeader | waitForEndOfInput;
      continue;
    }
    if ((nextState & waitForAlignmentHeader) &&
        handleDomainStart (line,name,&goOn)) {
      nextState = waitForHMMConsensus;
      // new match starts
      if (domainSeq_hook != NULL) {
        stringClear (querySeq);
        stringClear (hmmSeq);
        stringClear (matchSeq);
      }
      continue;
    }
    if (strlen (line) > 10 && (nextState & waitForHMMConsensus)) {
      // skip lines ending with RF, CS, or PP
      if (strEndsWith (line," RF") || strEndsWith (line," CS") ||
          strEndsWith (line," PP"))
        continue;
      tmpline = hlr_strdup (line);
      cp = strtok (line, " "); // sequence name
      cp = strtok (NULL, " "); // sequence position
      cp = strtok (NULL, " "); // sequence
      pos = strstr(tmpline,cp)-tmpline; // store position where sequence actually starts
      len = strlen (cp); // store length of sequence
      hlr_free (tmpline);
      nextState = waitForMatchLine;
      if (domainSeq_hook != NULL)
        stringCat (hmmSeq,cp);
      continue;
    }
    if (nextState & waitForMatchLine) {
      nextState = waitForQueryAlignment;
      if (domainSeq_hook != NULL) {
        if (pos && len > 0)
          stringNCat (matchSeq,line+pos,len);
      }
      continue;
    }
    if (strlen (line) > 10 && (nextState & waitForQueryAlignment)) {
      nextState = waitForHMMConsensus | waitForAlignmentHeader |
        waitForEndOfInput;
      cp = strtok (line, " "); // sequence name
      cp = strtok (NULL, " "); // sequence position
      cp = strtok (NULL, " "); // sequence
      strcpy (seq,cp);
      if (domainSeq_hook != NULL)
        stringCat (querySeq,seq);
      continue;
    }
  } // loop over input lines
  // treat the last alignment if necessary
  if (domainSeq_hook != NULL && stringLen (querySeq)) {
    goOn = (*domainSeq_hook) (string (querySeq),string (hmmSeq),
                              string (matchSeq));
  }
}
