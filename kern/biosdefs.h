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
/** @file biosdefs.h
    site/configuration specific definitions for
    Bioinformatics Objects and Services
    and defautls for application programs

    !!! if you change here, please also update biosdefs.make  !!!
    !!! there is no automatism in place to synchronize.       !!!

    This file has two sections:
    I. symbols used within BIOS
       (and perhaps additionally by some application)
    II. symbols not used within BIOS
        (but maybe one or more applications outside of BIOS
        - site-wide defaults)

    This file is read by the C compiler and by 'scriptconf'
    (see bios/base/utils/scriptconf.c) for configuring C-shell scripts.
    This places the following format restrictions on this file:
    - allowed: C-comments and C++-comments
               continuation lines
               #define symbol value
               where value is "string", 'c', NULL or number
    but:
    - no #if/#ifdef/#ifndef (except for first line)
    - no #include
    - no #define a(x) ...

    *** this is the bios source instance biosdefs.make
    *** this is a template which needs to be customized
    *** for a specific bios code instance
*/
#ifndef BIOSDEFS_H
#define BIOSDEFS_H

// --------- section I:  symbols used within BIOS

/* As of today, there might be some symbols not
   used in BIOS in section I. This is a bug needing
   cleanup.
   However there is an automatic system which analyzes the
   BIOS source code an reports the symbols not used within BIOS;
   see $BIOINFOCONFDIR/../roof/confana/README
*/

// ------ general stuff (this web server)
#define BISERVER "bioinfoc.ch"
#define BIHTTPPORT "80"

// ------ general stuff (mostly command line)
#define BIOINFOBIN "/apps/bin"
#define BIAPPSERVBIN "/apps/appservbin"
#define BIOINFOBINJAVA "/apps/binjava"
#define BIOSJAVALIBDIR "/apps/bios/libjava"
#define BIOSJAVABIN "/usr/bin"
#define BIOSPERL "/usr/bin/perl"

// ------ general stuff concerning the bioinfo WWW server
#define BIOSURL "http://" BISERVER ":" BIHTTPPORT "/bios"
#define BICSSURL "http://" BISERVER ":" BIHTTPPORT "/biccss"
#define BICSSURL_DEFAULT BICSSURL "/bioinfoc.css"
#define BIIMAGEURL "http://" BISERVER ":" BIHTTPPORT "/images"

#define BIOINFOCGIURL "http://" BISERVER ":" BIHTTPPORT "/bicgi"
#define BIOINFOCGIDIR "/apps/httpd/bicgi"
#define BIHTAPPSURL "http://" BISERVER ":" BIHTTPPORT "/apps"
//#define BIHTAPPSDIR "/data/httpd/htdoc/apps"
//#define BIHTDOCSDIR "/data/httpd/htdoc"
#define BIOSJAVALIBURL "http://" BISERVER ":" BIHTTPPORT "/libjava"
#define BIOSJSLIBURL "http://" BISERVER ":" BIHTTPPORT "/libjs"
#define BIOS_HTML_DOCTYPE "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">"
#define BIOS_HTML_IEMODE "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=8\">"

#define HTTP_PROXY ""
#define HTTP_PROXY_EXCEPTION ""
/* use "" to disable proxy
#define HTTP_PROXY "http://www-proxy1.company.com:8080/"
  use "" to signal "no exceptions"; must be lower case:
#define HTTP_PROXY_EXCEPTION ".company.com"
*/

// WWW group membership manager
#define GPM_URL "http://bioinfoc.ch/bicgi/grpmancgi"

// ----- misc sequence analysis packages used by BIOS modules
#define EMBOSS_BIN "/apps/emboss/c/bin"
#define BLAST_PROG_DIR "/apps/blast/c/c++/GCC460-Debug64/bin"
#define BLAST_DB_DIR "/data/blast"
#define BLAST_MAX_NUM_THREADS 1
#define GENEWISE_PROG_DIR "/apps/genewise/c/src/bin"

/* --------- section II: ------------------------------------
   symbols not used within BIOS
   -- site defaults --
*/
#define BIDATADIR "/data/bi"
#define BIOINFOTMP "/data/tmp"
#define BIOINFOLOGDIR "/data/log"
#define BIOS_ADMIN "your email address"

#endif
