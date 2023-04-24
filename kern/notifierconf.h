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
/** @file notifierconf.h
 */
#ifndef NOTIFIERCONF_H
#define NOTIFIERCONF_H

#define NOTIFIER_DB "bic1" 
#define NOTIFIER_USER "notifier"
#define NOTIFIER_PW "xxx"

#define NOTIFIER_DB_LOGFILE "xxx"

#define NOTIFIER_EVENT_ADD_URL "http://bioinfoc.ch/bicgi/notifiercgi?event+add"
#define NOTIFIER_EVENT_ADD_URL_DEV "http://bioinfoc.ch/bicgidev/notifiercgi?event+add"
#define NOTIFIER_EVENT_DELETE_URL "http://bioinfoc.ch/bicgi/notifiercgi?event+delete"
#define NOTIFIER_EVENT_DELETE_URL_DEV "http://bioinfoc.ch/bicgidev/notifiercgi?event+delete"

#endif
