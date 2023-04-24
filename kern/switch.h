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
/* BIOS config
   which services are to be compiled
   in which modes of operation
*/
#define kern 1
#define rdb_oracle 0
#define rdb_postgres 0
#define usrman 0
#define grpmanc 0
#define grpman 0
#define iwbi 0
#define iwbic 0
#define appserv 0
#define rannoc 0
#define seqobj 1
#define seqobjemboss 0
#define chem 1
#define sawic 0
#define sawi 0
#define ontoc 0
#define onto 0
#define alcoc 0
#define alco 0
#define excel2tab 0
#define java 0

/* usage notes:
- kern always needs to be 1, it is essential
- if you turn on a server (grpman, prefman, embossrocs)
  also turn on the corresponding client
  (grpmanc, prefmanc, embossroc)
*/
