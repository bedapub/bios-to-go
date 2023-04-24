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
/** @file sequtil.h
    @brief Module containing sequence utilities.
*/
#ifndef SEQUTIL_H
#define SEQUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "format.h"

/*
  In the following three macros, be sure not to use an expression
  with side effect for 'c'!
  (e.g. *cp++ would be disastrous)
*/
/**
   Determine whether input character is a valid gap character of a sequence
*/
#define isGapChar(c)  (c == '.' || c == '~' || c == '-')

/**
   Determine whether input character is a valid character of an EMBL sequence
*/
#define isSeqCharEmbl(c) ((c>='A' && c<='Z') || (c>='a' && c<='z') || c == '*')

/**
   Determine whether input character is a valid gap character or a valid
   character of an EMBL sequence
*/
#define isSeqOrGapChar(c) (isSeqCharEmbl(c) || isGapChar(c))

extern char complement (char b);
extern void revcomp (char *s);
extern char *iupac2alleles (char c);
extern void printSeq (FILE *strm,char *s);
extern char *seqBufferFasta (char *desc,char *s);
extern void printSeqFasta (FILE *strm,char *desc,char *s);
extern void printSeqGCG (FILE *stream,char *name,char *seq,int isNuc);

extern double calcMW (char *seq);

extern double getGEShydrophobicity (char aa);
extern double getKDhydrophobicity (char aa);
extern double calcAvgKDhydrophobicity (char *seq);

extern char translateCodon (char *cod);
extern char *translateCodonAmbig (char *cod);
extern char *translateSeq (char *nuc);

extern char *threeLetterAA (char oneLetterAA);

extern char *GCGtimeStamp (void);
extern int GCGcheckSum (char *seq);

extern char getSeqType (char *seq);

extern char *fileName2userseqName (char *nonDBname);

extern Texta ecExtract (char *de_line);

extern int blosum62 (char aa1,char aa2);

extern int isSwissprotID (char *n);

extern int rangeAc_set (char *rangeAc);
extern char *rangeAc_next (void);

#ifdef __cplusplus
}
#endif

#endif
