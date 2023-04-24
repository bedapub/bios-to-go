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
/** @file html.h
    @brief Parsing HTML CGI POST data, various other CGI routines
    and generating HTML pages.
    Module prefices cgi_, html_
*/
#ifndef HTML_H
#define HTML_H

#ifdef __cplusplus
extern "C" {
#endif

#include "format.h"

extern void cgiInit (void);
extern int cgiIsCGI (void);
extern void cgiContentDispositionSet (char *filename);
extern void cgiExpiresSet (int seconds_valid);
extern void cgiRedirSet (char *url);
extern void cgiEncodingSet (char *charset);
extern void cgiHeader (char *mimeType);
extern int cgiHeaderIsPrinted (void);
extern void cgiHeaderCookie (char *mimeType,char *cookieSpec);
extern void cgiDomainSet (char *domain);
extern char *cgiConstructCookie (char *name,char *value,int lifelength);
extern char *cgiReadCookie (char *name);

extern char *cgiGet2Post (void);
extern void cgiGet2PostReset (void);
extern void cgiGetInit (void);
extern char *cgiGetNext (Stringa value);
extern void cgiGetNextNamed (char *name, Stringa value);
extern char *cgiGetByName (char *name);
extern char *cgiGetByNameM (char *name);

extern void cgiEncodeWord (char *s,Stringa a);
extern char *cgiEncodeW (char *s);
extern void cgiDecodeWord (Stringa a);
extern void html_encode (char *inText,Stringa outText,int withExceptions);
extern char *html_encodeS (char *s);

extern void cgiURLCreate (char *host,int port,char *program);
extern void cgiURLCreate2 (char *cgiServerUrl,char *program);
extern void cgiURLAdd (char *param);
extern void cgiURLAddNV (char *name,char *value);
extern void cgiURLAddNVInt (char *name,int value);
extern void cgiURLAddInt (int param);
extern char *cgiURLGet (void);

extern void html_URLSet (char *host,int port,char *program);
extern void html_URLSet2 (char *cgiServerUrl,char *program);
extern void html_URLOptSet (char *option);
extern char *html_clink4 (char *class1,char *method,
                          char *p1,char *p2,char *p3,char *p4);

/// simplyfied version of html_clink4
#define html_clink(c,m) html_clink4(c,m,0,0,0,0)

/// simplyfied version of html_clink4
#define html_clink1(c,m,p1) html_clink4(c,m,p1,0,0,0)

/// simplyfied version of html_clink4
#define html_clink2(c,m,p1,p2) html_clink4(c,m,p1,p2,0,0)

/// simplyfied version of html_clink4
#define html_clink3(c,m,p1,p2,p3) html_clink4(c,m,p1,p2,p3,0)

extern void html_hlink4 (char *class1,char *method,char *label,
                         char *p1,char *p2,char *p3,char *p4);

/// simplyfied version of html_hlink4
#define html_hlink(c,m,l) html_hlink4(c,m,l,0,0,0,0)

/// simplyfied version of html_hlink4
#define html_hlink1(c,m,l,p1) html_hlink4(c,m,l,p1,0,0,0)

/// simplyfied version of html_hlink4
#define html_hlink2(c,m,l,p1,p2) html_hlink4(c,m,l,p1,p2,0,0)

/// simplyfied version of html_hlink4
#define html_hlink3(c,m,l,p1,p2,p3) html_hlink4(c,m,l,p1,p2,p3,0)

extern void html_setCSSclass (char *css_class);
extern char *html_getPopupLinkAdv (char *url,char *urlLabel,char *windowName,
                                   int isButton, int width,int height,
                                   int showScrollbars,int isResizeable,
                                   int showBars);
extern char *html_getPopupLink (char *url,char *urlLabel,char *windowName,
                                int isButton);
extern void html_printPopupLinkAdv (char *url,char *urlLabel,char *windowName,
                                    int isButton,int width,int height,
                                    int showScrollbars,int isResizeable,
                                    int showBars);
extern void html_printPopupLink (char *url,char *urlLabel,char *windowName,
                                 int isButton);
extern int html_uniqueIntGet (void);

extern char *html_tab2table (char *tab,int firstLineIsHeader,int borderWidth,
                             int withMarkup);
extern char *html_text2tables (char *tab,int firstLineIsHeader,int borderWidth,
                               int withMarkup);

extern void html_appletTagOpen (FILE *fp,char *jarFileUrls,char *appletClass,
                                int width,int height);
extern void html_appletParam (char *name,char *value);
extern void html_appletTagClose (void);

extern void html_webstartOpen (FILE *fp,char *codebase,char *title,
                               char *homepage,char *description,char *icon,
                               int allPermissions,char *heap,char *mainClass);
extern void html_webstartAddJar (char *jar);
extern void html_webstartAddArg (char *arg);
extern void html_webstartClose (void);

extern int cgiMpInit (void);
extern int cgiMpNext (Stringa item,Array value,Stringa filename,
                      Stringa contentType);
extern void cgiMpReset (void);
extern void cgiMpDeinit (void);

#ifdef __cplusplus
}
#endif

#endif
