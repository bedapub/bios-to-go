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
/** @file linestream.h
    @brief Module for reading arbitrarily long lines from files,
    pipes or buffers.
    Module prefix ls_
*/
#ifndef LINESTREAM_H
#define LINESTREAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "format.h"

/**
   The LineStream object. The members of this struct are PRIVATE for the
   LineStream module - DO NOT access from outside the LineStream module
*/
typedef struct _lineStreamStruct_ {
  FILE *fp; //!< the file pointer
  char *line; //!< the current line
  int lineLen; //!< length of the current line
  WordIter wi; //!< the WordIter used if buffer is being read
  int count; //!< number of the current line
  int status; //!< exit status of popen()
  char *(*nextLine_hook)(struct _lineStreamStruct_ *); //!< pointer to appropriate nextLine function
  Stringa buffer; //!< NULL if not in buffered mode, else used used for remembering last line seen
  char *bufferLine; //!< pointer to 'buffer' or NULL if EOF
  int bufferBack; //!< 0=normal, 1=take next line from buffer
}*LineStream;

extern LineStream ls_createFromFile (char *fn);
extern LineStream ls_createFromPipe (char *command);
extern LineStream ls_createFromBuffer (char *buffer);
extern char *ls_nextLine (LineStream this1);
extern void ls_destroy_func (LineStream this1); /* do not use this function */

/**
   Destroy the line stream, do not call ls_destroy_func but only this macro
*/
#define ls_destroy(this1) (ls_destroy_func(this1),this1=NULL) /* use this one */

extern void ls_bufferSet (LineStream this1,int lineCnt);
extern void ls_back (LineStream this1,int lineCnt);
extern int ls_lineCountGet (LineStream this1);
extern int ls_skipStatusGet (LineStream this1);
extern void ls_cat (LineStream this1,char *filename);
extern int ls_isEof (LineStream this1);

#ifdef __cplusplus
}
#endif

#endif
