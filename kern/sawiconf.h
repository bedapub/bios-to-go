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
/** @file sawiconf.h
    @brief Site-specific definitions for SAWI Sequence Analysis Web Interface
*/
#ifndef SAWICONF_H
#define SAWICONF_H

#include "biosdefs.h"

#define URL_SAWI_PRD_CGI "http://" BISERVER "/sawicgi"
#define FASTA_PROG_DIR "/apps/fasta/c"
#define PEARSONFASTA_VERSION "36"
#define MUSCLE_PROG_DIR "/apps/muscle/c/src"
#define VIENNARNA_PROG_DIR "/apps/vienna/c/Progs"
#define SIM4_PROG_DIR "/apps/sim4/c"
#define SAWI_TEMP_DIR "/data/httpd/htdoc/sawitmp"

#endif
