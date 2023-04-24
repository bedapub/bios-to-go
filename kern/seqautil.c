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
/** @file seqautil.c
    @brief Sequence analysis utilities, promoter utilities.
    Module prefixes: seqa_, prom_
*/
#include <ctype.h>
#include <math.h>
#include "log.h"
#include "format.h"
#include "linestream.h"
#include "seqautil.h"

float seqa_hsspThresh (int alilen,float percentid,int nSigma)  {
  /**
     Function from Abagyan and Batalov, JMB 273(1997)355.
     Determine distance from threshold for structure identity with given
     alignment length and percent identity.
     @param[in] alilen - length of alignment
     @param[in] percentid - percent identity of alignment
     @param[in] nSigma - standard deviations: 2 or 3 is fine, 4 is safe
     @return above zero: sequences are related
  */
  float thresh;
  thresh = pow (alilen,-0.124)*31 + nSigma * 18.2 * pow (alilen,-0.305);
  return percentid-thresh;
}

/* --------------------- promotor related stuff ----------------
   This module section contains routines and definitions used in Roche
   developed  promoter prediction tools and CpG island related stuff
   Module prefix: prom_
*/

/// maximum oligomer length supported
#define PROM_MAXWORDSIZE 8

static int gSkew_a; // storing nucleotid counts for skew value calculation
static int gSkew_c;
static int gSkew_t;
static int gSkew_g;

PromFreqStat prom_createFreqStat (int wordsize) {
  /**
     Allocates memory for a PromFreqStat (oligomer frequency statistics)
     data structure
     @param[in] wordsize - oligomer length
     @return PromFreqStat object (free with prom_destroyFreqStat)
  */
  PromFreqStat this1;

  if (wordsize>PROM_MAXWORDSIZE)
    die ("prom_createFreqStat: oligomer exceeds maximum word size");
  this1 = (PromFreqStat)hlr_calloc (1,sizeof (struct PromFreqStatStruct));
  this1->c = (int *)hlr_calloc (1<<(2*wordsize),sizeof (int));
  this1->p = (double *)hlr_calloc (1<<(2*wordsize),sizeof (double));
  this1->wordsize=wordsize;
  return this1;
}

void prom_destroyFreqStatFunc (PromFreqStat this1) {
  /**
     Destroys PromFreqStat data structure by freeing the memory it occupies
     @param[in] this1 - PromFreqStat object
  */
  hlr_free (this1->c);
  hlr_free (this1->p);
  hlr_free (this1);
}

PromFreqStat prom_freqStatFromFile (char *filename) {
  /**
     Reads oligomer frequency statistics data from a file and stores it in a
     PromFreqStat object
     @param[in] filename - name of the input file
     @return pointer to data; memory is allocated by this function
  */
  /*
    Input file example (use StatSeqBE application to create these statistics):
    total: 12343655     <-total counts
    wordsize: 5         <-oligomer length (here: pentamers)
    0 TTTTT 3324 1.043e-5   <- index, oligomer, counts, probability (normalized to 1)
    1 TTTTC 6543 2.563e-5
    .
    .
    .
    1023 AAAAA 4543 1.832e-5
  */
  int i;
  char *line;
  LineStream ls;
  char text[16];
  int totalnum;
  int wordsize;
  int pos;
  int c;
  double p;
  PromFreqStat this1;

  ls = ls_createFromFile (filename);
  sscanf (ls_nextLine (ls),"%15s %d",text,&totalnum);
  if (!strEqual (text,"total:"))
    die ("prom_freqStatFromFile: \'total\' line missing in  file %s",filename);
  sscanf (ls_nextLine (ls),"%15s %d",text,&wordsize);
  if (!strEqual (text,"wordsize:"))
    die ("prom_freqStatFromFile: \'wordsize\' line missing in  file %s",
         filename);
  if (wordsize > PROM_MAXWORDSIZE)
    die ("prom_freqStatFromFile: oligomer exceeds maximum word size");
  this1 = prom_createFreqStat (wordsize);
  this1->total=totalnum;
  i = 0;
  while ((line = ls_nextLine (ls)) != NULL) {
    sscanf (line, "%d %*s %d %lf",&pos,&c,&p);
    if (pos != i)
      die ("prom_freqStatFromFile: line for oligoindex %d missing in file %s",i,
           filename);
    this1->c[i] = c;
    this1->p[i] = p;
    i++;
  }
  ls_destroy (ls);
  if (i != 1<<(2*wordsize))
    die ("prom_freqStatFromfile: wrong number of oligos in file %s",filename);
  return this1;
}

int prom_nmer2index (char *oligo,int wordsize) {
  /**
     Calculates a number code for a specific oligomer which should e.g.
     be used as an array index
     @param[in] oligo - first nucleotide of oligomer (could be within a large
                        sequence)
     @param[in] wordsize - length of oligomer
     @return oligomer number code or -1 if characters other than A,C,G,T are
             found in oligomer string
           or oligo is shorter than wordsize characters
    this is the reverse function to prom_index2nmer
 */
  int i;
  int res = 0;
  int factor = 1;

  if (wordsize > PROM_MAXWORDSIZE)
    die ("prom_nmer2index: oligomer exceeds maximum word size");
  for (i=wordsize-1;i>=0;i--) {
    switch (oligo [i]) {
    case 'A':
      res += 3 * factor;
      break;
    case 'C':
      res += 2 * factor;
      break;
    case 'G':
      res += 1 * factor;
      break;
    case 'T':
      break;
    default:
      return -1;
    }
    factor *= 4;
  }
  return res;
}

char *prom_index2nmer (int index,int wordsize) {
  /**
     Returns the oligomer string which is coded by the given integer number.
     This is the reverse function to prom_nmer2index
     @param[in] index - number code (e.g. for pentamers, must lie in interval
                        [0..1023] )
     @param[in] wordsize - length of oligomer
     @return pointer to oligomer string; memory is managed by this function
 */
  static char wordbuffer[PROM_MAXWORDSIZE+1];
  int i,k;
  int factor;

  if (wordsize > PROM_MAXWORDSIZE)
    die ("prom_index2nmer: oligomer exceeds maximum word size");
  *(wordbuffer+wordsize) = '\0';
  factor = 1 << (2*wordsize-2);
  for (k=0;k<wordsize;k++) {
    i = index/factor;
    if (i == 0)
      wordbuffer[k] = 'T';
    else if (i == 1)
      wordbuffer[k] = 'G';
    else if (i == 2)
      wordbuffer[k] = 'C';
    else
      wordbuffer[k] = 'A';
    index -= factor*i;
    factor/=4;
  }
  return wordbuffer;
}

char *prom_revComp (char *oligo,int wordsize) {
  /**
     Returns the reverse complement for a given oligomer (e.g. ATCTT --> AAGAT)
     characters other than A,C,G,T are mapped to N
     @param[in] oligo - first nucleotide of oligomer (could be within a large
                        sequence)
     @param[in] wordsize - length of oligomer
     @return reverse complement; memory is managed by this function
  */
  int i;
  int j=0;
  static char wordbuffer[PROM_MAXWORDSIZE+1];

  if (wordsize > PROM_MAXWORDSIZE)
    die ("prom_revComp: oligomer exceeds maximum word size");
  *(wordbuffer+wordsize) = '\0';
  for (i=wordsize-1;i>=0;i--) {
    switch (oligo[i]) {
    case 'A':
      wordbuffer[j]='T';
      break;
    case 'C':
      wordbuffer[j]='G';
      break;
    case 'G':
      wordbuffer[j]='C';
      break;
    case 'T':
      wordbuffer[j]='A';
      break;
    default:
      wordbuffer[j]='N';
    }
    j++;
  }
  return wordbuffer;
}

void prom_oligoCount (Array seqs,int *wincounts,int *wintotal,int *backcounts,
                      int *backtotal,int from,int to,int offset,int wordsize) {
  /**
     Counts oligomer occurrences in a given sequence or a collection of
     sequences; the frequencies in a 'window of interest' (WOI) are separated
     from the 'background' counts; the window of interest can span the entire
     sequence, if no differentiation is needded; if several sequences are
     passed, the counts in all are summed up.<br>
     Precondition: memory for 'wincounts', 'wintotal', 'backcounts' and
                   'backtotal' must thave been allocated;
                   'wincounts' and 'backcounts' are integer arrays (int xxx[size]),
                   'size' should equal 1<<(2*n) to store all possible n-mers
     @param[in] seqs - array of sequences (type PromSequence)
     @param[out] wincounts - integer array: contains the count numbers of all
                             oligos INSIDE the WOI (use prom_nmer2index to get
                             array indices)
     @param[out] wintotal - integer value: total number of oligos INSIDE the WOI
     @param[out] backcounts - integer array: contains the count numbers of all
                              oligos OUTSIDE the WOI
     @param[out] backtotal - integer value: total number of oligos OUTSIDE
                             the WOI
     @param[in] from - start position of WOI
     @param[in] to - end position of WOI
     @param[in] offset - an offset which is added to the coordinates of the WOI
                         (e.g. offset could mark the TSS in a sequence, and
                         then the window coordinates would be measured relative
                         to this value; otherwise 0 is a sensible value :-)
     @param[in] wordsize - length of oligomers to be counted (6=hexamers...)
 */
  int i;
  int s;
  int index;
  int limit;
  PromSequence *current;

  if (wordsize > PROM_MAXWORDSIZE)
    die ("prom_oligoCount: oligomer exceeds maximum word size");
  limit = 1<<(2*wordsize);
  for (i=0;i<limit;i++) {
    wincounts[i] = 0;
    backcounts[i] = 0;
  }
  *wintotal = 0;
  *backtotal = 0;
  for (s=0;s<arrayMax (seqs);s++) {
    current=arrp (seqs,s,PromSequence);
    for (i=0;i<(current->length-wordsize);i++) {
      index = prom_nmer2index (string (current->sequence)+i,wordsize);
      if (index == -1)
        continue;
      if (i>from+offset-2 && i<to+offset-wordsize+2) {
        (*wintotal)++;
        wincounts[index]++;
      }
      else {
        backcounts[index]++;
        (*backtotal)++;
      }
    }
  }
}

void prom_CpGCount (char *start,int winsize,int *c_count,int *g_count,
                    int *cpg_count) {
  /**
     Determines content of 'C', 'G' and 'CpG dinucleotides' in a sequence.
     See also: prom_updateCpGCount
     @param[in] start - pointer to a starting point in a sequence
     @param[in] winsize - size of the sequence window to be examined
     @param[in] c_count - valid location
     @param[in] g_count - valid location
     @param[in] cpg_count - valid location
     @param[out] c_count - number of C nucleotides in sequence window
     @param[out] g_count - number of G nucleotides in sequence window
     @param[out] cpg_count - number of CpG dinucleotides in sequence window
  */
  int i;
  *cpg_count = *c_count = *g_count = 0;
  for (i=0;i<winsize-1;i++) {
    if (*(start+i) == 'C')
      (*c_count)++;
    if (*(start+i) == 'G')
      (*g_count)++;
    if (*(start+i) == 'C' && *(start+i+1) == 'G')
      (*cpg_count)++;
  }
  if (*(start+winsize-1) == 'C')
    (*c_count)++;
  if (*(start+winsize-1) == 'G')
    (*g_count)++;
}

void prom_updateCpGCount (char *start,int winsize,int *c_count,int *g_count,
                          int *cpg_count) {
  /**
     Updates the count numbers returned by prom_CpGCount if the sequence window
     has been shifted downstream by one position;
     this makes it much more effective to score very long sequences.<br>
     See also: prom_CpGCount.<br>
     Precondition: prom_CpGCount should have been called to obtain valid start
                   values for c_count, g_count, cpg_count;
                   it is assumed that the 'start' value has already been
                   increased by 1, so it now points to the next nucleotide;
                   this must be done each time before the routine is called
                   again;
     @param[in] start - pointer to a starting point in a sequence
     @param[in] winsize - size of the sequence window to be examined
     @param[in] c_count - valid location
     @param[in] g_count - valid location
     @param[in] cpg_count - valid location
     @param[out] c_count - number of C nucleotides in sequence window
     @param[out] g_count - number of G nucleotides in sequence window
     @param[out] cpg_count - number of CpG dinucleotides in sequence window
  */
  if (*(start-1) == 'C')
    (*c_count)--;
  if (*(start-1) == 'G')
    (*g_count)--;
  if (*(start-1) == 'C' && *start == 'G')
    (*cpg_count)--;
  if (*(start+winsize-1) == 'C')
    (*c_count)++;
  if (*(start+winsize-1) == 'G')
    (*g_count)++;
  if (*(start+winsize-2) == 'C' && *(start+winsize-1) == 'G')
    (*cpg_count)++;
}

double prom_CpGScore (PromSequence sequence,int offset) {
  /**
     Calculates CpG-Score as defined by Davuluri, Grosse & Zhang,
     Nature Genet. 29 (2001), 412-417;
     a score value of >7.5 can be regarded as a good threshold for the
     definition of CpG related promoters;
     @param[in] sequence - the input nucleotide sequence
     @param[in] offset - the position at which the CpG-Score should be
                         calculated; this is essentially the central position
                         of a 1000 bp window which is examined
     @return the CpG score or -1.0 if the 1000 bp window doesn't fit at that
             point of the sequence
  */
  int i;
  int c;
  int g;
  int cpg;
  int max;

  if (offset < 500 || offset > sequence.length-500)
    return -1.0;
  prom_CpGCount (string (sequence.sequence)+offset-500,201,&c,&g,&cpg);
  max=cpg;
  for (i=-499;i<=499;i++) {
    prom_updateCpGCount (string (sequence.sequence)+offset+i,201,&c,&g,&cpg);
    if (cpg > max)
      max=cpg;
  }
  return (double)max/2;
}

int prom_CpGIslands (char *sequence,int length,Texta islands) {
  /**
     Scans a sequence for CpG islands (definition by Takai & Jones: g+c>=0.55,
     cpg(obs)/cpg(exp)>=0.65, length >500 bp);<br>
     Precondition: textarray 'islands' must have been created (it is not
                   cleared by this function, which makes it easy to collect the
                   results from several sequence scans)
     @param[in] sequence - input nucleotide sequence
     @param[in] length - length of sequence
     @param[out] islands - contains information about the found CpG islands;
                           line format is as follows:
                           start_position:center_position:end_position:length.
                           A NULL pointer may be given as 'islands' parameter
                           if only the number of islands is needed;
     @return the number of found islands
  */
  int i;
  int c_count;
  int g_count;
  int cpg_count;
  double cplusg_content;
  double cpg_content;
  int counter;
  int win_size;
  Stringa temp = stringCreate (64);

  if (length < 500) // return directly if sequence is not long enough
    return 0;
  // initialize
  win_size = 500;
  prom_CpGCount (sequence,500,&c_count,&g_count,&cpg_count);
  counter = 0;
  i=0;
  // iterate over the sequence
  while (i <= length-500) {
    cpg_content = (double)(cpg_count)/(c_count*g_count/win_size);
    cplusg_content = (double)(c_count+g_count)/win_size;
    if (cpg_content >= 0.65 && cplusg_content >= 0.55) { // CpG Window found
      while (cpg_content >= 0.65 && cplusg_content >= 0.55 &&
             i+win_size < length) { // then maximise CpG window
        if (*(sequence+i+win_size) == 'C')
          c_count++;
        if (*(sequence+i+win_size) == 'G')
          g_count++;
        if (*(sequence+i+win_size-1) == 'C' && *(sequence+i+win_size) == 'G')
          cpg_count++;
        win_size++;
        cpg_content = (double)(cpg_count)/(c_count*g_count/win_size);
        cplusg_content = (double)(c_count+g_count)/win_size;
      }
      counter++;
      if (islands) {
        stringPrintf (temp,"%d:%d:%d:%d",
                      i,i+(win_size/2),i+win_size-1,win_size);
        textAdd (islands,string (temp));
      }
      i += win_size;
      if (i < length-500)
        prom_CpGCount (sequence+i,500,&c_count,&g_count,&cpg_count);
      else
        i=length;
      win_size=500;
    }
    else { // else move window
      i++;
      prom_updateCpGCount (sequence+i,500,&c_count,&g_count,&cpg_count);
    }
  }
  stringDestroy (temp);
  return counter;
}

int prom_CpGIslands2 (char *sequence,int length,Texta islands) {
  /**
     Scans a sequence for CpG islands that are likely to be associated with
     5' ends of genes; this is a reimplementation of the algorithm published by
     Ponger & Mouchiroud, Bioinformatics 18 (2002);<br>
     Precondition: textarray 'islands' must have been created (it is not
                   cleared by this function, which makes it easy to collect the
                   results from several sequence scans)
     @param[in] sequence - input nucleotide sequence
     @param[in] length - length of sequence
     @param[out] islands - contains information about the found CpG islands;
                           line format is as follows:
                           start_position:center_position:end_position:length
                           A NULL pointer may be given as 'islands' parameter
                           if only the number of islands is needed;
     @return the number of found islands
  */
  int i;
  int c_count;
  int g_count;
  int cpg_count;
  int counter;
  int last;
  Array start;
  Array end;
  Stringa temp;

  if (length < 500) // return directly if sequence is not long enough
    return 0;
  start = arrayCreate (256,int);
  end = arrayCreate (256,int);
  // initialize
  prom_CpGCount (sequence,500,&c_count,&g_count,&cpg_count);
  counter = 0;
  i=0;
  // iterate over the sequence
  last = length-500;
  while (i<last) {
    if ((c_count+g_count >= 250) &&
        ((double)cpg_count/(c_count*g_count) >= 0.0012)) { // CpG rich region found
      if (counter > 0 && i-arru (end,counter-1,int) < 200)
        arru (end,counter-1,int) = i+499; // extend previous island
      else {
        array (start,counter,int) = i; // create new island
        array (end,counter,int) = i+499;
        counter++;
      }
    }
    i++;
    prom_updateCpGCount (sequence+i,500,&c_count,&g_count,&cpg_count);
  }
  if (islands) {
    temp=stringCreate (64);
    for (i=0;i<counter;i++) {
      stringPrintf (temp,"%d:%d:%d:%d",
                    arru (start,i,int)+1,
                    (arru (end,i,int) + arru (start,i,int))/2+1,
                    arru (end,i,int)+1,arru (end,i,int) - arru (start,i,int)+1);
      textAdd (islands,string (temp));
    }
    stringDestroy (temp);
  }
  arrayDestroy (start);
  arrayDestroy (end);
  return counter;
}

void prom_rateCpGIslands (char *sequence,Texta islands,int isRodent) {
  /**
     Append additional information to CpG islands found on a sequence;
     uses same formulae as program 'CpgProD' (Ponger & Mouchiroud,
     Bioinformatics 18 (2002), 631-633)
     @param[in] sequence - the sequence on which the islands are located
     @param[in] islands - CpG island coordinates as returned by
                          prom_CpGIslands or prom_CpGIslands2
     @param[out] islands - the following parameters are appended to the lines
                           in array 'islands':
                           ats - AT-skew value;
                           cgs - CG-skew value;
                           startp - probability that CpG island is a 5' end of
                                    a gene;
                           strand - prediction of the strand the gene is
                                    located on (can be '-' or '+');
                           strandp - reliability of strand prediction
     @param[in] isRodent - uses rodent (mouse, rat) data if set to 1,
                           data for other vertebrates (e.g. humans) otherwise
  */
  int i;
  int start;
  int size;
  double ats;
  double cgs;
  int c;
  int g;
  int cpg;
  double z;
  double startp;
  double strandp;
  char strand;
  Stringa temp;

  temp = stringCreate (64);
  for (i=0;i<arrayMax (islands);i++) {
    if (sscanf (textItem (islands,i),"%d:%*d:%*d:%d",&start,&size) != 2)
      die ("prom_rateCpGislands: bad format of input line: %s",
           textItem (islands,i));
    prom_skewness (sequence+start-1,size,&ats,&cgs);
    prom_CpGCount (sequence+start-1,size,&c,&g,&cpg);
    if (isRodent==1) {
      z = -19.4423 - 0.00008749142 * (double)size+15.27366 *
        ((double)(c+g)/size) + 16.38997 * ((double)cpg/((double)c*g/size));
      startp = exp (z)/(1+exp (z));
      z = 0.2161-12.3270*ats-8.8730*cgs;
      strandp = exp (z)/(1+exp (z));
    }
    else {
      z = -7.271471 + 0.0005927055 * (double)size + 4.043293 *
        ((double)(c+g)/size)+4.83027 * ((double)cpg/((double)c*g/size));
      startp = exp (z)/(1+exp (z));
      z = 0.02853-11.01590*ats-13.44387*cgs;
      strandp = exp (z)/(1+exp (z));
    }
    if (z < 0) {
      strandp = 1-strandp;
      strand = '-';
    }
    else
      strand = '+';
    stringPrintf (temp,"%s:%lf:%lf:%lf:%c:%lf",
                  textItem (islands,i),ats,cgs,startp,strand,strandp);
    strReplace (&textItem (islands,i),string (temp));
  }
  stringDestroy (temp);
}

double prom_indexScore (char *start,int winsize,PromFreqStat stat) {
  /**
     Takes an oligomer frequency data structure and scores a sequence chunk
     with it.<br>
     See also: prom_updateIndexScore
     @param[in] start - pointer to a starting point in a sequence
     @param[in] winsize - size of the sequence window to be examined
     @param[in] stat - PromFreqStat object
     @return score value
  */
  int i;
  int index;
  double score = 0.0;
  for (i=0;i<=winsize-stat->wordsize;i++) {
    index = prom_nmer2index (start+i,stat->wordsize);
    if (index!=-1)
      score+=stat->p[index];
  }
  return score;
}

double prom_updateIndexScore (char *start,int winsize,PromFreqStat stat) {
  /**
     Updates the score returned by prom_CpGCount if the sequence window has
     been shifted downstream by one position;
     This makes it much more effective to score very long sequences.<br>
     See also: prom_indexScore.<br>
     Precondition: prom_indexScore should have been called to obtain a valid
                   start value; it is assumed that the 'start' value has
                   already been increased by 1, so it now points to the next
                   nucleotide; this must be done each time before the routine
                   is called again;
     @param[in] start - pointer to a starting point in a sequence
     @param[in] winsize - size of the sequence window to be examined
     @param[in] stat - PromFreqStat object
     @return score value
  */
  int index;
  double delta = 0.0;

  index = prom_nmer2index (start-1,stat->wordsize);
  if (index != -1)
    delta -= stat->p[index];
  index = prom_nmer2index (start+winsize-stat->wordsize,stat->wordsize);
  if (index != -1)
    delta += stat->p[index];
  return delta;
}

void prom_skewness (char *start,int winsize,double *at_skew,double *cg_skew) {
  /**
     Determines the AT- and CG-Skew value in a sequence
     (see Ponger & Mouchiroud, Bioinformatics 18 (2002);
     This is useful for predicting the orientation of a CpG island.<br>
     Postcondition: prom_updateSkewness can be called with parameter (start+1)
     @param[in] start - pointer to a starting point in a sequence
     @param[in] winsize - size of the sequence window to be examined
     @param[in] at_skew - valid location
     @param[in] cg_skew - valid location
     @param[out] at_skew - AT skew value
     @param[out] cg_skew - CG skew value
  */
  int i;
  gSkew_a=gSkew_c=gSkew_t=gSkew_g=0;
  for (i=0;i<winsize;i++)
    switch (*(start+i)) {
    case 'A':
      gSkew_a++;
      break;
    case 'G':
      gSkew_g++;
      break;
    case 'T':
      gSkew_t++;
      break;
    case 'C':
      gSkew_c++;
      break;
    }
  *at_skew = (double)(gSkew_a-gSkew_t)/(gSkew_a+gSkew_t);
  *cg_skew = (double)(gSkew_c-gSkew_g)/(gSkew_c+gSkew_g);
}

void prom_updateSkewness (char *start,int winsize,double *at_skew,
                          double *cg_skew) {
  /**
     Updates the score returned by prom_skewness if the sequence window has
     been shifted downstream by one position; this makes it much more effective
     to score very long sequences.<br>
     Precondition: prom_skewness should have been called to obtain a valid
                   start value; it is assumed that the 'start' value has
                   already been increased by 1, so it now points to the next
                   nucleotide; this must be done each time before the routine
                   is called again;<br>
     Postcondition: can be called with parameter (start+1).<br>
     See also: prom_skewness
     @param[in] start - pointer to a starting point in a sequence
     @param[in] winsize - size of the sequence window to be examined
     @param[in] at_skew - valid location
     @param[in] cg_skew - valid location
     @param[out] at_skew - AT skew value
     @param[out] cg_skew - CG skew value
  */
  switch (*(start-1)) {
  case 'A':
    gSkew_a--;
    break;
  case 'G':
    gSkew_g--;
    break;
  case 'T':
    gSkew_t--;
    break;
  case 'C':
    gSkew_c--;
    break;
  }
  switch (*(start+winsize-1)) {
  case 'A':
    gSkew_a++;
    break;
  case 'G':
    gSkew_g++;
    break;
  case 'T':
    gSkew_t++;
    break;
  case 'C':
    gSkew_c++;
    break;
  }
  *at_skew = (double)(gSkew_a-gSkew_t)/(gSkew_a+gSkew_t);
  *cg_skew = (double)(gSkew_c-gSkew_g)/(gSkew_c+gSkew_g);
}

int prom_fastaFromFile (char *filename,int maxseqs,Array seqs) {
  /**
     Reads a Fasta DB file and appends the sequences to one array.<br>
     Precondition: array 'seqs' must have been created
     @param[in] filename - name of input file
     @param[in] maxseqs - maximum number of sequences to load from file
                          (unlimited if maxseqs<1)
     @param[out] seqs - all sequences (or first 'maxseqs' sequences) read from
                        the file are appended; type PromSequence
     @return number of sequences appended
  */
  LineStream ls;
  char *line;
  int i;
  int count=0;
  int alreadythere;
  PromSequence *current = malloc(sizeof(PromSequence));

  alreadythere = arrayMax (seqs);
  ls = ls_createFromFile (filename);
  while ((line = ls_nextLine (ls)) != NULL) {
    toupperStr (line);
    if (*line == ' ')
      line += 1;
    if (*line == '>') {
      count++;
      if (maxseqs>0 && count>maxseqs) {
        count--;
        break;
      }
      current = arrayp (seqs,arrayMax (seqs),PromSequence);
      current->sequence = stringCreate (10000);
      current->name = stringCreate (64);
      stringCpy (current->name,line+1);
    }
    else
      stringCat (current->sequence,line);
  }
  ls_destroy (ls);
  for (i=alreadythere;i<arrayMax(seqs);i++)
    arrp (seqs,i,PromSequence)->length =
      stringLen (arrp (seqs,i,PromSequence)->sequence);
  return count;
}

/* -------------------- sequence properties ---------------------- */

static char *currSeq = NULL;

static int aaCounts[26];
static double aaWeights[26] =
  { 71.07940,114.59699,103.13940,115.08935,129.11644,
   147.17818, 57.05231,137.14219,113.16067,  0.00000,
   128.17534,113.16067,131.19358,114.10462,  0.00000,
    97.11764,128.13171,156.18874, 87.07880,101.10589,
     0.00000, 99.13358,186.21515,  0.00000,163.17758,
   128.62408};

static double aaVols[26] = {0.74,0.61,0.63,0.60,0.66,
                            0.77,0.64,0.67,0.90,0.72,
                            0.82,0.90,0.75,0.62,0.72,
                            0.76,0.67,0.70,0.63,0.70,
                            0.72,0.86,0.74,0.72,0.71,
                            0.665};

static double aapKs[28] = { 0.0, 0.0, 8.5, 3.9, 4.1,
                            0.0, 0.0, 6.5, 0.0, 0.0,
                           10.8, 0.0, 0.0, 0.0, 0.0,
                            0.0, 0.0,12.5, 0.0, 0.0,
                            0.0, 0.0, 0.0, 0.0,10.1,
                            0.0, 8.6, 3.6};
// last 2 are amino / carboxy terminal

static void updateCounts (char *seq) {
  int i,len;
  char c;
  static Stringa msg = NULL;

  stringCreateOnce (msg,50);
  if (currSeq != NULL && strEqual (currSeq,seq))
    return;
  for (i=0;i<26;i++)
    aaCounts[i] = 0;
  len = strlen (seq);
  for (i=0;i<len;i++) {
    c = toupper (seq[i]);
    if (c < 'A' || c > 'Z') {
      stringPrintf (msg,"Invalid amino acid encountered: %c, skipped",c);
      warnAdd ("calcMW",string (msg));
    }
    else
      (aaCounts[c-'A'])++;
  }
  strReplace (&currSeq,seq);
}

double sp_calcMW (char *seq) {
  /**
     Calculate molecular weihgt of a protein
     @param[in] seq - the protein sequence
     @return the molecular weight
  */
  double weight;
  int i;

  updateCounts (seq);
  weight = 18.01534;
  for (i=0;i<26;i++)
    weight += (aaCounts[i] * aaWeights[i]);
  return weight;
}

double sp_calcAbsorbance (char *seq) {
  /**
     Calculate absorbance at 280 nm, no disulfides
     @param[in] seq - the protein sequence
     @return the absorbance
  */
  double abs = 0.0;

  updateCounts (seq);
  if (aaCounts[22] + aaCounts[24] > 0)
    abs = (5500.0 * aaCounts[22] + 1490.0 * aaCounts[24]) / sp_calcMW (seq);
  return abs;
}

double sp_calcPartSpecVol (char *seq) {
  /**
     Calculate the partial specific valume at 25degrees
     @param[in] seq - the protein sequence
     @return the partial specific volume
  */
  double a = 0.0,b = 0.0;
  int i;
  double v;
  float temp = 20.0;

  updateCounts (seq);
  for (i=0;i<26;i++) {
    if (aaCounts[i] > 0) {
      a += aaCounts[i] * aaWeights[i] * aaVols[i];
      b += aaCounts[i] * aaWeights[i];
    }
  }
  v = a/b;
  v += 4.25e-4 * (temp - 25.0);
  return v;
}

static double getProtons (int i,double H,double K[]) {
  /**
     Needed for calculation of isoelectrip point
     i=26 : Nterm, i=27 : C-term
  */
  if (K[i] == 0.0)
    return 0.0;
  if (i == 26 || i == 27)
    return H/(H+K[i]); // one amino and one carboxy terminal
  return aaCounts[i] * (H/(H+K[i]));
}

static double getCharge (double pH,double K[]) {
  /**
     Needed for calculation of isoelectrip point
  */
  double H = pow (10.0,-pH);
  return (getProtons (10,H,K) + getProtons (17,H,K) + getProtons (7,H,K) +
          getProtons (26,H,K)) -
    (aaCounts[24]-getProtons (24,H,K) + aaCounts[2]-getProtons (2,H,K) +
     aaCounts[3]-getProtons (3,H,K) + aaCounts[4]-getProtons (4,H,K) +
     1-getProtons (27,H,K));
}

double sp_calcIEP (char *seq) {
  /**
     Calculate the isoelectrip point of a protein
     @param[in] seq - the protein sequence
     @return the isoelectric point
  */
  double top_pH = 1.0;
  double bot_pH = 14.0;
  double mid_pH;
  double chg;
  double K[28];
  int i;

  updateCounts (seq);
  for (i=0;i<28;i++)
    K[i] = pow (10.0,-aapKs[i]);
  double top_chg = getCharge (top_pH,K);
  double bot_chg = getCharge (bot_pH,K);
  if ((top_chg>0.0 && bot_chg>0.0) ||
      (top_chg<0.0 && bot_chg<0.0))
    return 0.0;
  while (bot_pH - top_pH > 0.0001) {
    mid_pH = top_pH + (bot_pH - top_pH) / 2;
    chg = getCharge (mid_pH,K);
    if (chg > 0.0)
      top_pH = mid_pH;
    else if (chg < 0.0)
      bot_pH = mid_pH;
    else
      return mid_pH;
  }
  return top_pH;
}
