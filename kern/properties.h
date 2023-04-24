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
/** @file properties.h
    @brief Property files are lists of name value pairs separated by newline
    characters. The name is separated from the value by an arbitrary number of
    space, :, or = characters, the value is everything between the last
    consecutive separator and the newline character.
    For non-pathological cases, the property file format understood by this C
    module is identical to the one used by the standard Java class
    java.util.Properties.
    Module prefix pty_
*/
#ifndef PROPERTIES_H
#define PROPERTIES_H

#ifdef __cplusplus
extern "C" {
#endif

/// structure to hold properties
typedef struct _PropertiesStruct_ {
  Array pairs; //!< of type NVpair
}*Properties;

/// structure to hold one property
typedef struct {
  char *name; //!< name of property
  char *value; //!< value of property
}NVpair;

Properties pty_create (void);
void pty_destroyFunc (Properties this1);
/// use this macro, not pty_destroyFunc()
#define pty_destroy(x) ((x) ? pty_destroyFunc(x),x=NULL,1 : 0)
void pty_load (Properties this1,char* filename);
char* pty_getProperty (Properties this1,char *name);
char* pty_getPropertyChecked (Properties this1,char *name);

#ifdef __cplusplus
}
#endif

#endif
