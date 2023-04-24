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
/** @file pearsonfastaparser.h
    @brief Parser for output of Pearson fasta programs.
    Module prefix pfp_
*/
#ifndef PEARSONFASTAPARSER_H
#define PEARSONFASTAPARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "linestream.h"

void pfp_init (void);
void pfp_register_begin (int (*f)(void));
void pfp_register_progName (int (*f)(char *progName));
void pfp_register_query (int (*f)(char *query,int querylen,int queryIsNuc));
void pfp_register_database (int (*f)(char *name,int numSeq,int numLetters));
void pfp_register_summary (int (*f)(char *subject,char *desc,
                                    int qframe,int sframe,
                                    int score,float bits,double prob));
void pfp_register_subjectStart (int (*f)(char *subject,int subjLen,
                                         int subjIsNuc));
void pfp_register_alignment (int (*f)(char *querySeq,char *subjSeq,
                                      char *matchStr,float id,int overlap,
                                      int queryBeg,int queryEnd,
                                      int subjBeg,int subjEnd));
void pfp_register_subjectEnd (int (*f)(void));
void pfp_register_end (int (*f)(void));
void pfp_run (LineStream ls);

#ifdef __cplusplus
}
#endif

#endif
