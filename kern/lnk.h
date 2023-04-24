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
/** @file lnk.h
    @brief Knows how to find bioinformatics object identifiers in
    natural language text and hyperlink them using module biurl.
    Module prefix lnk_
*/
#ifndef LNK_H
#define LNK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "format.h"

/// e.g. sw:Q9Y5X9
#define TL_SEQ_ID  1
/// e.g. sw:lipe_human
#define TL_SEQ_NAME 2
/// e.g. http://www.roche.com/abc.html
#define TL_URL 3
/// e.g. <<http://www.roche.com/x y z/xyz.html>>
#define TL_MARKED_URL 4
/// e.g. ranno:801
#define TL_RANNO_ID 5
/// e.g. link="http://www.google.com?q=KINASE;Google Kinase"
#define TL_PDT_URL 6

/// structure to hold one link
typedef struct textLinkStruct {
  char *url; //!< the url
  int position; //!< the position
  int type; //!< use types: TL_*
}*TextLink;

/// synonym for Array
typedef Array TextLinks;

extern TextLink lnk_textLinkCreate (char *url,int position,int type);
extern void lnk_textLinkDestroy (TextLink textLink);
extern void lnk_textLinksDestroy (Array textLinks);
extern TextLinks lnk_textParse (char *text,int includeHTTP);
/// signature of function
typedef char *(*SWName2SWAccFunc) (char *dbseqname);
extern void lnk_registerSWName2SWAccFunc (SWName2SWAccFunc f);
extern Stringa lnk_addURLsOrSW2text (Array textLinks,char* text,
                                     int addOnlySWname);
extern Stringa lnk_addURLs2text (Array textLinks,char* text);
extern void lnk_text2html (char *text,Stringa outText);

/// hyperlink to a WWW interface to the Roche user directory; usage: printf("<a href=\"" LNK_F_ACCOUNTINFO "\">info</a>", username)
#define LNK_F_ACCOUNTINFO "http://bioinfoc.ch/bicgi/userinfocgi?%s"

#ifdef __cplusplus
}
#endif

#endif
