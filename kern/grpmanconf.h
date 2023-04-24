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
/** @file grpmanconf.h
    @brief Site-specific definitions for grpman
    ! --- this file contains the grpman database account password
    ! --- be careful with giving read access
*/
#ifndef GRPMANCONF_H
#define GRPMANCONF_H

#include "biosdefs.h"

#define GROUPMEMBERCHECK "/apps/bin/grpman_ok"
#define GPM_GROUPS_OF_USER_URL "http://bioinfoc.ch/bicgi/grpmancgi?command+groupsOfUser+userid+"

// define GPM_DB_USE if the grpman internal DB should be used

#define GPM_DB_USE

#define GPM_DB "fill with database"
#define GPM_DB_USERNAME "grpman"
#define GPM_DB_PASSWORD "fill with password"
#define GPM_MAXSESSIONAGE 3600
#define GPM_REFRESHCMD BIAPPSERVBIN "/grpmanclient -refresh"

#define GPM_DOWNMSG "\nSorry, some internal problem occurred.\nPerhaps the grpman server is down.\nPlease contact <a mailto:bioinfoc@bioinfoc.ch>the administrator</a> if you need this fixed.\n"

#define GPM_FILES BIOSURL "/specific/grpman"
#define GPM_UP BIHTAPPSURL "/"
#define GPM_SUPERUSERGRP "su"
#define GPM_USER_LOOKUP "http://bioinfoc.ch/bicgi/usrmancgi"

#define GPM_LEN_USERID 50
#define GPM_LEN_GROUPNAME 50
#define GPM_LEN_DESCR 250

#endif
