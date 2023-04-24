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
/** @file seqautil.h
    @brief Sequence analysis utilities, promoter utilities.
    Module prefixes: seqa_, prom_
*/
#ifndef SEQAUTIL_H
#define SEQAUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

float seqa_hsspThresh(int alilen, float percentid, int nSigma);

/// structure to store one or more sequences as plain strings
typedef struct PromSequenceStruct {
  Stringa sequence; //!< the sequence itself
  Stringa name;  //!< sequence name
  int length; //!< sequence length
}PromSequence;

/// structure to store oligomer frequency information
typedef struct PromFreqStatStruct {
  int wordsize; //!< length of oligomers (6=hexamers...)
  int total; //!< total counts; sum over all oligos
  int *c; //!< absolute count number for oligomer; use prom_nmer2index to calculate array index
  double *p; //!< probability of oligomer (normalized to 1); use prom_nmer2index to calculate array index
}*PromFreqStat;

extern PromFreqStat prom_createFreqStat (int wordsize);
extern void prom_destroyFreqStatFunc (PromFreqStat this1);
/// use this macro, not prom_destroyFreqStatFunc()
#define prom_destroyFreqStat(x) ((x) ? prom_destroyFreqStatFunc(x),x=NULL,1:0)
extern PromFreqStat prom_freqStatFromFile (char *filename);
extern int prom_nmer2index (char *oligo,int wordsize);
extern char *prom_index2nmer (int index,int wordsize);
extern char *prom_revComp (char *oligo,int wordsize);
extern void prom_oligoCount (Array seqs,int *wincounts,int *wintotal,
                             int *backcounts,int *backtotal,int from,int to,
                             int offset,int wordsize);
extern void prom_CpGCount (char *start,int winsize,int *c_count,int *g_count,
                           int *cpg_count);
extern void prom_updateCpGCount (char *start,int winsize,int *c_count,
                                 int *g_count,int *cpg_count);
extern double prom_CpGScore (PromSequence sequence,int offset);
extern int prom_CpGIslands (char *sequence,int length,Texta islands);
extern int prom_CpGIslands2 (char *sequence,int length,Texta islands);
extern void prom_rateCpGIslands (char *sequence,Texta islands,int isRodent);
extern double prom_indexScore (char *start,int winsize,PromFreqStat stat);
extern double prom_updateIndexScore (char *start,int winsize,PromFreqStat stat);
extern void prom_skewness (char *start,int winsize,
                           double *at_skew,double *cg_skew);
extern void prom_updateSkewness (char *start,int winsize,
                                 double *at_skew,double *cg_skew);
extern int prom_fastaFromFile (char *filename,int maxseqs,Array seqs);

extern double sp_calcMW (char *seq);
extern double sp_calcAbsorbance (char *seq);
extern double sp_calcPartSpecVol (char *seq);
extern double sp_calcIEP (char *seq);

#ifdef __cplusplus
}
#endif

#endif
