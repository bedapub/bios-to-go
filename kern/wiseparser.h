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
/** @file wiseparser.h
    @brief Knows how to parse genewise output.
    Module prefix wp_
*/
#ifndef WISEPARSER_H
#define WISEPARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "linestream.h"

extern void wp_init (int offset);
extern void wp_register_exon (int (*f)(char *proQ,int pBeg,int pEnd,
                                       char *proS,int numID,
                                       char *nuc,int frame,int nBeg,int nEnd,
                                       char *edit));
extern void wp_register_intron (int (*f)(int iBeg,int iEnd,
                                         char *spliceAAQ,char *spliceAAS));
extern void wp_run (LineStream ls);

#ifdef __cplusplus
}
#endif

#endif
