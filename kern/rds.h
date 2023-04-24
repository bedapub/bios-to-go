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
/** @file rds.h
    @brief Provides access to user information from usrman.
    Module prefix rds_
*/
#ifndef RDS_H
#define RDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "format.h"

/// field
#define RDS_FIRSTNAME 0
/// field
#define RDS_LASTNAME 1
/// field
#define RDS_COMPANY 2
/// field
#define RDS_EMAIL 3
/// field
#define RDS_USERID 4
/// field
#define RDS_CREATED 5
/// field
#define RDS_LASTUPDATED 6
/// last field
#define RDS_LAST 6

extern char *rds_get (char *userid,int column);
/// convenience function to get first name
#define rds_getFirstName(userid) rds_get((userid),RDS_FIRSTNAME)
/// convenience function to get last name
#define rds_getLastName(userid) rds_get((userid),RDS_LASTNAME)
/// convenience function to get email
#define rds_getEmail(userid) rds_get((userid),RDS_EMAIL)
/// convenience function to get company name
#define rds_getCompany(userid) rds_get((userid),RDS_COMPANY)
extern char *rds_getName (char *userid);

#ifdef __cplusplus
}
#endif

#endif
