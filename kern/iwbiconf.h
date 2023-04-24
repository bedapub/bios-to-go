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
/** @file iwbiconf.h
 */
#ifndef IWBICONF_H
#define IWBICONF_H

// some internal functions from identwwwbiutil.c :
extern char *iwbi_encrypt (char *s,int len,int *outlen);
extern char *iwbi_decrypt (char *s,int len,int *outlen);

#define IWBI_ENCRYPTOR "Bioinfoc default - please put your own string here"
#define IWBI_ENCRYPTOR_SEED 'Z'

// where to send cookies to (localhost works)
#define IWBI_DOMAIN "bioinfoc.ch"

// ---- for identwwwbi[util].c ----
#define IWBI_HOST "bioinfoc.ch"
#define IWBI_PORT 80
#define IWBI_CGI "/bicgi/identwwwbicgi"
#define IWBI_SESSIONNAME "bioinfoc.ch"

#define IWBI_USER_IN_FORM "My username is"
#define IWBI_PASSWORD_HELP "The password is case-sensitive!<p><p>"

/* message displayed to users who are not in the group they need to be;
   must feature a first %s for username and a second %s for groupname
   to make sense */
#define IWBI_GROUPFAILMSG "Sorry, %s, to perform this function you need to be member of group %s. According to my database, you are currently not member of this group. Please contact <a mailto=bioinfoc@bioinfoc.ch>bioinfoc.ch</a>, who is managing access rights on this machine.<p>"

#define IWBI_SULOGFILE "/data/log/identwwwbisu.log"

// bioinfo login serice -- application accounts
#define IWBI_APPLOGFILE "/data/log/identwwwbiapp.log"
#define IWBI_APP_ACCOUNT_FILE "/apps/conf/identwwwbi_appaccounts"

#endif
