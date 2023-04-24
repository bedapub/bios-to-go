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
/** @brief Relational database utilites, handling database login info.
    Module prefix rdbu_
*/
#ifndef RDBU_H
#define RDBU_H

#ifdef __cplusplus
extern "C" {
#endif

extern void rdbu_loginFunc (char* (*rdbr_login)(char *server,char *dbname,
                                                char *user,char *passwd));
extern void rdbu_initLoginInfo (char *filename);
extern char *rdbu_user (void);
extern char *rdbu_password (void);
extern char *rdbu_database (void);

/// if you use rdbr_login() then you need to also include "rdbr.h"
#define rdbu_login() rdbu_loginFunc(&rdbr_login)

#ifdef __cplusplus
}
#endif

#endif
