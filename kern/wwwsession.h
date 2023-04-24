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
/** @file wwwsession.h
    @brief Handle WWW sessions for CGI programs.
    Module prefix wwwsess_
*/
#ifndef WWWSESSION_H
#define WWWSESSION_H

#ifdef __cplusplus
extern "C" {
#endif

extern char *wwwsession_identEncrypt (char *s,int len,int *outlen);
extern char *wwwsession_identDecrypt (char *s,int len,int *outlen);

extern void wwwsess_register_encrypt (char*(*f)(char *s,int len,int *outlen));
extern void wwwsess_register_decrypt (char*(*f)(char *s,int len,int *outlen));
extern void wwwsess_fake (char *sessionname,char *user,char *password);
extern void wwwsess_start (char *sessionname,char *user,char *password,
                           int lifelength);
extern void wwwsess_check (char *sessionname,char **user,char **password,
                           int *age);
extern void wwwsess_read (char *value,char **user,char **password,int *age);
extern void wwwsess_logout (char *sessionname);

#ifdef __cplusplus
}
#endif

#endif
