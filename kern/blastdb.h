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
/** @file blastdb.h
    @brief Reading sequences from blast databases.
    Module prefix bdb_
*/
#ifndef BLASTDB_H
#define BLASTDB_H

#ifdef __cplusplus
extern "C" {
#endif

/// database is nucleotide
#define DB_TYPE_NUC 0
/// database is protein
#define DB_TYPE_PRO 1

extern int bdb_read_next (char **name,char **seq);
extern void bdb_open (char *dbname,int dbtype);

#ifdef __cplusplus
}
#endif

#endif
