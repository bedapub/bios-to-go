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
/** @file biurl.h
    @brief Knows how to map bioinformatics object identifiers to URLs.
    Object identifiers are e.g. sequence identifiers like "SW:DYR_HUMAN"
    or user account names like "USER:wolfd". The URLs constructed should
    lead to a page giving details about the object in a user-readable form.
    This module runs parallel to bios/src/bios/kern/BiURL.java.
    Module prefix biurl_
*/
#ifndef BIURL_H
#define BIURL_H

#ifdef __cplusplus
extern "C" {
#endif

extern void biurl_setTypeCheckMode (int onOff);

extern char *biurl_typedBuild (char *classobjname,char *reportType);
extern char *biurl_build (char *classobjname);

extern char *biurl_typedBuild2 (char *classname,char *objname,char *reportType);
extern void biurl_typedBuildRegister (char *(*f)(char *classname,char *objname,
                                                 char *reportType));
extern char *biurl_build2 (char *classname,char *objname);
extern char *biurl_typedBuild2Html (char *classname,char *objname,
                                    char *urlDescription,char *reportType);
extern char *biurl_typedBuildHtml (char *classobjname,char *urlDescription,
                                   char *reportType);
extern char *biurl_buildHtml (char *classobjname,char* urlDescription);
extern char *biurl_build2Html (char *classname,char *objname,
                               char* urlDescription);
extern char *biurl_typedBuild2Popup (char *classname,char *objname,
                                     char *urlDescription,char *reportType);
extern char *biurl_typedBuildPopup (char *classobjname,char *urlDescription,
                                    char *reportType);
extern char *biurl_build2Popup (char *classname,char *objname,
                                char* urlDescription);
extern char *biurl_buildPopup (char *classobjname,char* urlDescription);

#ifdef __cplusplus
}
#endif

#endif
