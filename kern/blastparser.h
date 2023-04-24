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
/** @file blastparser.h
    @brief Purpose: dissect the standard output of the BLAST program.
    Module prefix bp_
*/

#ifndef BLASTPARSER_H
#define BLASTPARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "linestream.h"

/* details about function calls see file blastparser.c / blastparser2.c
basic usage:
1. call bp_init();
2. register callback functions (which should return 1 if
   parsing should continue, 0 to quit parsing).
   you need only to register functions needed.
3. call bp_run()

link either with blastparser.o for processing BLAST output
or with blastparser2.o for processing BLAST2 output
*/

void bp_init (void);
void bp_register_begin (int (*f)(void));
void bp_register_end (int (*f)(void));
void bp_register_progName (int (*f)(char *programName));
void bp_register_query (int (*f)(char *query,int querylen));
void bp_register_database (int (*f)(char *db,int numSeq,int numLetters));
void bp_register_summary (int (*f)(char *subject,char *desc,float score,
                                   double expect));
void bp_register_subjectStart (int (*f)(char *subject,int subjectlen,
                                        char *desc));
void bp_register_subjectEnd (int (*f)(void));
void bp_register_HSPStart (int (*f)(float score,double expect,int n));
void bp_register_idFrame (int (*f)(int hspLen,int identities,int positives,
                                   int gaps,int queryFrame,int sbjctFrame));
void bp_register_HSPEnd (int (*f)(int sLeftEnd,int sRightEnd,
                                  int qLeftEnd,int qRightEnd));
void bp_register_HSPSeq (int (*f)(char *querySeq,char *sbjctSeq,char *match));
void bp_run (LineStream ls);

#ifdef __cplusplus
}
#endif

#endif
