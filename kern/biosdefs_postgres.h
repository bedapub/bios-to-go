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
/** @file biosdefs_postgres.h
    C-counterpart of 'postgresenv' script

    this file is read by the C preprocess and by the 'scriptconf' utility
    (part of bios/base/utils/).
*/
#ifndef BIOSDEFS_POSTGRES_H
#define BIOSDEFS_POSTGRES_H

#define PGHOST   "localhost"
#define PGPORT   "5432"

// needed to compile bios/exa/rdb_postgres/ test programs:
#define BIOSDBMSTEST_DBNAME "bic1"
#define BIOSDBMSTEST_SCHEMA "test"
#define BIOSDBMSTEST_USER "test"
#define BIOSDBMSTEST_PASSWORD "fill with pass"
#define BIOSDBMSTEST_PASSWORD_ENCODED " \001\146\317\073\247\307\076\247\033\203\243\023\164\347\132\150"

#endif
