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
/** @file primer3parser.h
    @brief Parses output of the primer3 program.
    Module prefix pr3p_
*/
#ifndef PRIMER3PARSER_H
#define PRIMER3PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "linestream.h"

extern void pr3p_register_sequence (int (*f) (char *seqname));
extern void pr3p_register_product (int (*f) (int num,int length));
extern void pr3p_register_forward (int (*f) (int beg,int end,char *seq,
                                             float gc,float tm));
extern void pr3p_register_reverse (int (*f) (int beg,int end,char *seq,
                                             float gc,float tm));
extern void pr3p_register_internal (int (*f) (int beg,int end,char *seq,
                                              float gc,float tm));
extern void pr3p_init (void);
extern void pr3p_run (LineStream ls);

#ifdef __cplusplus
}
#endif

#endif
