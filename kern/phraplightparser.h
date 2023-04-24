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
/** @file phraplightparser.h
   @brief Purpose: dissect the output of the PHRAP program in .ace files.
   Module prefix phrlp_
*/
#ifndef PHRAPLIGHTPARSER_H
#define PHRAPLIGHTPARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "linestream.h"

extern void phrlp_register_contig (int (*f)(int nReads,char *contigName,
                                            char *consensus,
                                            int *offsets,char **readNames,
                                            char **segments,int *revs));
extern void phrlp_register_contigWithQual (int (*f)(int nReads,char *contigName,
                                                    char *consensus,
                                                    char *consensusQual,
                                                    int *offsets,
                                                    char **readNames,
                                                    char **segments,int *revs));
extern void phrlp_init (void);
extern void phrlp_run (LineStream ls);

#ifdef __cplusplus
}
#endif

#endif
