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
/** @file usrmanconf.h
   Site-specific definitions for usrman
   ! --- this file contains the usrman rdbms account password
   ! --- be careful with giving out read access
*/
#ifndef USRMANCONF_H
#define USRMANCONF_H

#include "biosdefs.h"

#define USRMAN_HOSTNAME "bioinfoc.ch"
#define USRMAN_PORTNUMBER 80
#define USRMAN_PORTNUMBERSTR "80"
#define USRMAN_CGINAME "/bicgi/usrmancgi"

#define USRMAN_LOGFILE "/data/log/usrman.log"

#define USRMAN_DB "bic1"
#define USRMAN_DB_USERNAME "usrman"
#define USRMAN_DB_PASSWORD "fill with password"

#define USRMAN_ADMIN_GROUP "youradmingroup"

#endif
