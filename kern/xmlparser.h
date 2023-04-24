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
/** @file xmlparser.h
    @brief Quick and Dirty xml parser. This parser is, like the SAX parser,
    an event based parser, but with much less functionality.
    Module prefix xmlp_
*/
#ifndef XMLPARSER_H
#define XMLPARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "linestream.h"
#include "stringlist.h"

extern void xmlp_init (void);
extern void xmlp_register_startDocument (void (*f)(void));
extern void xmlp_register_endDocument (void (*f)(void));
extern void xmlp_register_startElement (void (*f)(char *tag, StringList attributes));
extern void xmlp_register_endElement (void (*f)(char *tag));
extern void xmlp_register_text (void (*f)(char *str));
extern void xmlp_run (LineStream ls);

#ifdef __cplusplus
}
#endif

#endif
