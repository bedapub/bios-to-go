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
/** @file htmlform.h
    @brief Modification of HTML forms.
    Module prefix htmlform_
*/
#ifndef HTMLFORM_H
#define HTMLFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "format.h"

/// return status of functions
#define HTMLFORM_OK 0
/// return status of functions
#define HTMLFORM_INVALID_FORM 1
/// return status of functions
#define HTMLFORM_INVALID_FIELD 2

extern int htmlform_fieldFind (char *html,char *form,char *name,
                               char **begin,char ** end,Stringa tagType,
                               Stringa inputType);
extern int htmlform_valueSet (Stringa html,char *form,char *name,char *value);
extern int htmlform_valueAdd (Stringa html,char *form,char *name,char *value);
extern int htmlform_valueClear (Stringa html,char *form,char *name);

#ifdef __cplusplus
}
#endif

#endif
