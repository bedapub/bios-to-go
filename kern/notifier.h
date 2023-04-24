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
/** @file notifier.h
    @brief Client for the event notification service:
    "Automatic Event Notifier".
    Module prefix notifier_
*/
#ifndef NOTIFIER_H
#define NOTIFIER_H

#ifdef __cplusplus
extern "C" {
#endif

extern void notifier_useDevelopment (void);
extern char *notifier_addEvent (char *application,Texta domains,char *message);
extern char *notifier_deleteEvent (char *application,Texta domains,
                                   char *message);

#ifdef __cplusplus
}
#endif

#endif
