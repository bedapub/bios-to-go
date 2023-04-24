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
/** @file hmmparser.h
    @brief Purpose: dissect the output of the HMMSCAN or HMMSEARCH programs.
    Module prefix hmmp_
*/
#ifndef HMMPARSER_H
#define HMMPARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "linestream.h"

void hmmp_init (void);
void hmmp_register_begin (int (*f)(void));
void hmmp_register_end (int (*f)(void));
void hmmp_register_progName (int (*f)(char *progName));
void hmmp_register_query (int (*f)(char *query));
void hmmp_register_db (int (*f)(char *db));
void hmmp_register_summary (int (*f)(char *name,char *descr,
                                     float score,double prob,
                                     float bias,int domainNr));
void hmmp_register_domain (int (*f)(char *name,float score,double prob,
                                    int currDomainNr,
                                    int seqLeftEnd,int seqRightEnd,
                                    int hmmLeftEnd,int hmmRightEnd,
                                    char seqTopo[2],char hmmTopo[2]));
void hmmp_register_domainAli (int (*f)(char *name,float score,double prob,
                                       int currDomainNr,
                                       int seqLeftEnd,int seqRightEnd));
void hmmp_register_domainSeq (int (*f)(char *querySeq,char *hmmSeq,
                                       char *matchSeq));
void hmmp_run (LineStream ls);

#ifdef __cplusplus
}
#endif

#endif
