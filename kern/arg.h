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
/** @file arg.h
    @brief Module to parse command line arguments.
    Module prefix arg_
*/
#ifndef ARG_H
#define ARG_H

#ifdef __cplusplus
extern "C" {
#endif

extern int arg_isInit (void);
extern int arg_init (int argc,char *argv[],
                     char *optargs,char *reqargs,
                     void (*usagef)(int level));
extern int arg_initTry (int argc, char *argv[],
                        char *optargs, char *reqargs,
                        void (*usagef)(int level));
extern int arg_present (char *name);
extern char *arg_getPos (char *name,int pos);
/// for convenience because most arguments have only one value
#define arg_get(name) arg_getPos(name,1)
extern char *arg_getProgName (void);

#ifdef __cplusplus
}
#endif

#endif
