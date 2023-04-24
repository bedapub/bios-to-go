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
/** @file plabla.h
    @brief PLatform ABstraction LAyer
    for Bioinformatics Objects and Services.
    Module prefix plabla_
*/
/*
  inputs: (via plabla_conf.h)
          BIOS_PLATFORM
          BIOS_BITS_PER_INT
*/
#ifndef PLABLA_H
#define PLABLA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "plabla_conf.h"

#ifndef BIOS_PLATFORM
/// undefined, because #include plabla_conf.h failed
#define BIOS_PLATFORM -1
#endif

#if (BIOS_PLATFORM == BIOS_PLATFORM_IRIX) || (BIOS_PLATFORM == BIOS_PLATFORM_SOLARIS) || (BIOS_PLATFORM == BIOS_PLATFORM_LINUX) || (BIOS_PLATFORM == BIOS_PLATFORM_APPLE)
#define PLABLA_INCLUDE_IO_UNISTD <unistd.h>
#define PLABLA_POPEN popen
#define PLABLA_PCLOSE pclose
#define PLABLA_OPEN open
#define PLABLA_CLOSE close
#define PLABLA_ISATTY isatty
#endif

#if BIOS_PLATFORM == BIOS_PLATFORM_IRIX
#define PLABLA_FLOCK_OPENFFLAG O_RDONLY
#define PLABLA_FLOCK(fildes) flock(fildes,LOCK_EX)
#define PLABLA_FLOCKNB(fildes) flock(fildes,LOCK_EX|LOCK_NB)
#define PLABLA_FUNLOCK(fildes) flock(fildes,LOCK_UN)
#define PLABLA_ISLOCKED EWOULDBLOCK
#endif

#if (BIOS_PLATFORM == BIOS_PLATFORM_LINUX) || (BIOS_PLATFORM == BIOS_PLATFORM_APPLE)
#define PLABLA_FLOCK_OPENFFLAG O_RDONLY
#define PLABLA_FLOCK(fildes) flock(fildes,LOCK_EX)
#define PLABLA_FLOCKNB(fildes) flock(fildes,LOCK_EX|LOCK_NB)
#define PLABLA_FUNLOCK(fildes) flock(fildes,LOCK_UN)
#define PLABLA_ISLOCKED EWOULDBLOCK
#endif

#if BIOS_PLATFORM == BIOS_PLATFORM_SOLARIS
#define PLABLA_FLOCK_OPENFFLAG O_RDWR
#define PLABLA_FLOCK(fildes) lockf(fildes,F_LOCK,0)
#define PLABLA_FLOCKNB(fildes) lockf(fildes,F_TLOCK,0)
#define PLABLA_FUNLOCK(fildes) lockf(fildes,F_ULOCK,0)
#define PLABLA_ISLOCKED EAGAIN
#endif

#if BIOS_PLATFORM == BIOS_PLATFORM_WINNT
#define PLABLA_INCLUDE_IO_UNISTD <io.h>
#define PLABLA_OPEN _open
#define PLABLA_CLOSE _close
#define PLABLA_ISATTY _isatty
#define PLABLA_POPEN _popen
#define PLABLA_PCLOSE _pclose
#endif

/// minimum value of 2 byte integer
#define INTGR2_MIN (-32767-1)
/// maximum value of 2 byte integer
#define INTGR2_MAX (32767)
/// maximum value of 2 byte unsigned integer
#define UINTGR2_MAX (65535u)
/// minimum value of 4 byte integer
#define INTGR4_MIN (-2147483647-1)
/// maximum value of 4 byte integer
#define INTGR4_MAX (2147483647)
/// maximum value of 4 byte unsigned integer
#define UINTGR4_MAX (4294967295u)
/// minimum value of 8 byte integer
#define INTGR8_MIN (-9223372036854775807LL-1)
/// maximum value of 8 byte integer
#define INTGR8_MAX (9223372036854775807LL)
/// maximum value of 8 byte unsigned integer
#define UINTGR8_MAX (18446744073709551615uLL)

/// intgr is synonym of int
#define intgr int
/// intgr2 is synonym of short: short seems always to be 2 bytes
#define intgr2 short
/// intgr4 is synonym of int: int seems always to be 4 bytes
#define intgr4 int

/// intgr8 is an 8 byte integer but it's name in C depends on the processor architecure
#if BIOS_BITS_PER_LONG == 32
#define intgr8 long long
#endif

#if BIOS_BITS_PER_LONG == 64
#define intgr8 long
#endif

#if BIOS_BITS_PER_INT == 32 && BIOS_PLATFORM == BIOS_PLATFORM_SOLARIS
/// platform dependent stat function
#define PLABLA_STAT stat64
/// platform dependent lstat function
#define PLABLA_LSTAT lstat64
#else
/// platform dependent stat function
#define PLABLA_STAT stat
/// platform dependent lstat function
#define PLABLA_LSTAT lstat
#endif

#ifdef __cplusplus
}
#endif

#endif
