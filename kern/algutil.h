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
/** @file algutil.h
    @brief Makes local or global sequence alignments.
    Uses code from EMBOSS.
    Module prefix algutil_
*/
#ifndef ALGUTIL_H
#define ALGUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

/// indicates the a global alignment should be produced
#define ALGUTIL_GLOBAL 1
/// indicates the a local alignment should be produced
#define ALGUTIL_LOCAL 2

extern void algutil_setSeqs (char *name1,char *seq1,
                             char *name2,char *seq2,int isNuc);
extern void algutil_register_weight_symb (float (*f)(char c1,char c2));
extern void algutil_register_weight_pos (float (*f)(int p1,int p2));
extern float algutil_run (int mode,float go,float ge,
                          int doEndWeight,float ego,float ege);
extern void algutil_getAlg (char **alg1,char **alg2);

#ifdef __cplusplus
}
#endif

#endif
