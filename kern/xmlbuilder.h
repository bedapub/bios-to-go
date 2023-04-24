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
/** @file xmlbuilder.h
    @brief Build a string in XML format.
    Module prefix xmlb_
*/
#ifndef XMLBUILDER_H
#define XMLBUILDER_H

#ifdef __cplusplus
extern "C" {
#endif

extern void xmlb_init (void);
extern void xmlb_doIndentSet (int doIndent);
extern void xmlb_doctypeSystem (char *rootElement,char *dtdURI);
extern void xmlb_embedded (int baseIndentDepth);
extern void xmlb_es (char *tag);
extern void xmlb_aRaw (char *name,char *value);
extern void xmlb_a (char *name,char *value);
extern void xmlb_aInt (char *name,int value);
extern void xmlb_aDouble (char *name,double value,char *format);
extern void xmlb_t (char *data);
extern void xmlb_tInt (int data);
extern void xmlb_ee (void);
extern char *xmlb_get (void);

#ifdef __cplusplus
}
#endif

#endif
