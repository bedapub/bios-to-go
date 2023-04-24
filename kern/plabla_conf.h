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
/**
   @file plabla_conf.h
   select platform to compile for
   (PLatform ABstraction LAyer)
*/

#ifndef PLABLACONF_H
#define PLABLACONF_H

#define BIOS_PLATFORM_IRIX      1
#define BIOS_PLATFORM_SOLARIS   2
#define BIOS_PLATFORM_WINNT     3
#define BIOS_PLATFORM_LINUX     4
#define BIOS_PLATFORM_APPLE     5

#ifdef __linux__
  #define BIOS_PLATFORM BIOS_PLATFORM_LINUX
#elif _WIN32
  #define BIOS_PLATFORM BIOS_PLATFORM_WINNT
#elif __sun
  #define BIOS_PLATFORM BIOS_PLATFORM_SOLARIS
#elif __APPLE__
  #define BIOS_PLATFORM BIOS_PLATFORM_APPLE
#endif

// number of bits in a long integer variable; currently 32 and 64 are supported
#define BIOS_BITS_PER_LONG 64

#endif
