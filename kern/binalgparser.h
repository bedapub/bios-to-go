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
/** @file binalgparser.h
    @brief Parser for binary alignments (e.g. water, needle, prophet).
    Module prefix bap_
*/
#ifndef BINALGPARSER_H
#define BINALGPARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "linestream.h"

void bap_init (void);
void bap_register_seqnames (int (*f)(char *seq1Name,char *seq2Name));
void bap_register_idsimi (int (*f)(int isNuc,float score,
                                   float id,float simi,int gaps));
void bap_register_seq (int (*f)(char *s1,char *s2,char *match,
                                int beg1,int end1,int beg2,int end2));
void bap_run (LineStream ls);

#ifdef __cplusplus
}
#endif

#endif
