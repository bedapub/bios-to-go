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
/** @file properties.c
    @brief Property files are lists of name value pairs separated by newline
    characters. The name is separated from the value by an arbitrary number of
    space, :, or = characters, the value is everything between the last
    consecutive separator and the newline character.
    For non-pathological cases, the property file format understood by this C
    module is identical to the one used by the standard Java class
    java.util.Properties.
    Module prefix pty_
*/
#undef PTY_DEBUG
/// used for debugging
#define PTY_VERB 9
#ifdef PTY_DEBUG
#define DD(a,b) {if (PTY_VERB>=a) {printf b;fflush(stdout);fflush(stderr);}}
#else
/// used for debugging
#define DD(a,b)
#endif

#include "log.h"
#include "linestream.h"
#include "properties.h"

static char* splitNameValue (char *line) {
  char delim[] = ": =";
  char *pos = strpbrk (line,delim);
  *pos = '\0';
  pos++;
  pos += strspn (pos,delim);
  return pos;
}

static int orderByName (NVpair *p1,NVpair *p2) {
  return strcmp (p1->name,p2->name);
}

Properties pty_create (void) {
  /**
     Create Properties
     @return Properties object
  */
  Properties new1;
  new1 = (Properties)hlr_malloc (sizeof (struct _PropertiesStruct_));
  new1->pairs = arrayCreate (10,NVpair);
  return new1;
}

void pty_destroyFunc (Properties this1) {
  /**
     Destroys Properties
     @param[in] this1 - Properties object
  */
  int i;
  NVpair *currNV;

  for (i=0;i<arrayMax (this1->pairs);i++) {
    currNV = arrp (this1->pairs,i,NVpair);
    hlr_free (currNV->name);
    hlr_free (currNV->value);
  }
  arrayDestroy (this1->pairs);
  hlr_free (this1);
}

void pty_load (Properties this1,char* filename) {
  /**
     Load Properties from file
     @param[in] this1 - Properties object
     @param[in] filename - file to read
  */
  char *line;
  NVpair oneNV;
  int inserted;
  char *msg;

  LineStream ls = ls_createFromFile (filename);
  while ((line = ls_nextLine (ls)) != NULL) {
    char *pos = splitNameValue (line);
    oneNV.name = hlr_strdup (line);
    oneNV.value = hlr_strdup (pos);
    inserted = arrayFindInsert (this1->pairs,&oneNV,NULL,
                                (ARRAYORDERF)orderByName);
    msg = inserted ? "Inserted ": "Not inserted ";
    DD(9,("%s'%s'='%s'\n", msg,
          currNV->name,currNV->value));
  }
  ls_destroy (ls);
}

char* pty_getProperty (Properties this1,char *name) {
  /**
     Get property value
     @param[in] this1 - Properties object
     @param[in] name - name of property
     @return value of property if found, else NULL
  */
  int i;
  NVpair oneNV;

  oneNV.name = name;
  if (arrayFind (this1->pairs,&oneNV,&i,(ARRAYORDERF)orderByName))
    return arrp (this1->pairs,i,NVpair)->value;
  return NULL;
}

char* pty_getPropertyChecked (Properties this1,char *name) {
  /**
     Get property value
     @param[in] this1 - Properties object
     @param[in] name - name of property
     @return value of property if found, else die.
  */
  char *prop = pty_getProperty (this1,name);
  if (prop == NULL)
    die ("Could not read property '%s' from Properties.",name);
  return prop;
}
