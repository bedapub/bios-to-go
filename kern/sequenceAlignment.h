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
/** @file sequenceAlignment.h
    @brief Allows to print an alignment between 2 sequences in a
    formatted fashion.
    Module prefix sa_
*/
#ifndef SEQUENCEALIGNMENT_H
#define SEQUENCEALIGNMENT_H

#ifdef __cplusplus
extern "C" {
#endif

/// length of the output line
#define LINE_LENGTH 60
/// maximum length of the sequence names
#define NAME_LENGTH 16

extern char *sa_getPositionLine (char *seq,int frame,int begin,int end,
                                 int isNuc,int orgIsNuc);
extern void sa_printAlignment (char *seq1,char *seq2,char *sName1,char *sName2,
                               int frame1,int frame2,int begin1,int begin2,
                               int end1,int end2,int isNuc,
                               int orgIsNuc1,int orgIsNuc2);

#ifdef __cplusplus
}
#endif

#endif
