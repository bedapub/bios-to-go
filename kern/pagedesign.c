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
/** @file pagedesign.c
    @brief Look and feel of bioinfoc.ch website.
    Module prefix pd_
*/
#include "format.h"
#include "biosdefs.h"
#include "pagedesign.h"

void pd_html (void) {
  /**
     Writes the document type
  */
  puts ("<!DOCTYPE html>\n<html>");
}

static Stringa gHeadAdd = NULL;

void pd_headAdd (char *line) {
  /**
     Allows to add a line to be printed in pd_head(), such as to include
     stylesheets
     @param[in] line - the line to be added
  */
  stringCreateOnce (gHeadAdd,50);
  stringCat (gHeadAdd,line);
}

void pd_head (char *title) {
  /**
     Prints the header of the web page
     @param[in] title - the title to be shown on to of the browser window
  */
  puts ("<head>");
  printf ("<title>%s</title>\n",title);
  puts ("<link rel='shortcut icon' href='http://bioinfoc.ch/favicon.ico'>");
  puts ("<link rel='icon' href='http://bioinfoc.ch/favicon_ani.gif' type='image/gif'>");
  printf ("<link rel=stylesheet type='text/css' href='%s'>\n",
          BICSSURL_DEFAULT);
  if (gHeadAdd != NULL)
    printf ("%s",string (gHeadAdd));
  puts ("</head>");
}

static char *gBodyTitle = NULL;

void pd_bodySetTitle (char *title) {
  /**
     Allows to set the title to be displayed in the body of the web page.
     To be called before pd_body()
     @param[in] title - the title
  */
  strReplace (&gBodyTitle,title);
}

static Stringa gBodyLinks = NULL;

void pd_bodySetLink (char *url,char *disp) {
  /**
     Allows to add a link to be shown in the menu bar, To be called before
     pd_body()
     @param[in] url - the URL
     @param[in] disp - wht to display for the link
  */
  stringCreateOnce (gBodyLinks,100);
  stringCat (gBodyLinks," | ");
  stringAppendf (gBodyLinks,"<a href=%s class=bic_header>%s</a>",url,disp);
}

void pd_body (char *attr,char *user) {
  /**
     Prints the body of the page.
     @param[in] attr - the attributes to be added to the body tag
     @param[in] user - the user id to be shown in the menu bar
  */
  printf ("<body");
  if (attr != NULL && attr[0] != '\0')
    printf (" %s>\n",attr);
  else
    puts (">");
  puts ("<div class=bic_header></div>");
  puts ("<div class=bic_menubar></div>");
  printf ("<div class=bic_menubarmiddle>User: %s</div>\n",
          user != NULL ? user : "[unknown]");
  printf ("<div class=bic_menubarright>%s %s &nbsp;&nbsp;&nbsp;&nbsp;</div>\n",
          user == NULL ?
          "<a href='http://bioinfoc.ch/bicgi/identwwwbicgi?login' class=bic_header>Sign in</a>" :
          strEqual (user,USER_IN_TRANSITION) ? "" :
          "<a href='http://bioinfoc.ch/bicgi/identwwwbicgi?logout' class=bic_header>Sign out</a>",
          gBodyLinks != NULL ? string (gBodyLinks) : "");
  puts ("<div class=bic_menubarsub></div>");
  puts ("<div class=bic_headerleft_logo><a href='http://bioinfoc.ch'><img src='http://bioinfoc.ch/biccss/bioinfoc_logo_small.png' width=127 height=110 border=0 alt=bioinfoc_logo></a></div>");
  puts ("<div class=bic_headerleft_text><a href='http://bioinfoc.ch' class=bic_header>bioinfoc.ch</a></div>");
  printf ("<div class=bic_headercenter>%s</div>\n",
          gBodyTitle);
  puts ("<div class=bic_main>");
}

void pd_bodyClose (int withTechs) {
  /**
     Close the div bic_main, body and html
     @param[in] withTechs - whether technologies should be shown
  */
  puts ("</div>"); // bic_main
  if (withTechs) {
    puts ("<div class=bic_footer></div>");
    puts ("<div class=bic_technologies><table cellpadding=4><tr><td>\n"
          "&nbsp;"
          "  <a href='http://www.ubuntu.com' target=techs><img src='http://bioinfoc.ch/biccss/poweredByUbuntu.jpeg' width=65 height=65 alt=poweredByUbuntu></a>\n"
          "&nbsp;"
          "  <a href='http://www.postgresql.org' target=techs><img src='http://bioinfoc.ch/biccss/poweredByPostgres.gif' width=65 height=65 alt=poweredByPostgres></a>\n"
          "&nbsp;"
          "  <a href='http://www.rdkit.org' target=techs><img src='http://bioinfoc.ch/biccss/poweredByRdkit.png' width=65 height=65 alt=poweredByRdkit></a>\n"
          "&nbsp;"
          "  <a href='http://emboss.sourceforge.net/' target=techs><img src='http://bioinfoc.ch/biccss/poweredByEmboss.jpeg' width=75 height=50 alt=poweredByEmboss></a>\n"
          "&nbsp;"
          "  <a href='http://lucene.apache.org' target=techs><img src='http://bioinfoc.ch/biccss/poweredByLucene.gif' width=85 height=30 alt=poweredByLucene></a>\n"
          //"  <a href='http://www.stack.nl/~dimitri/doxygen' target=techs><img src='http://bioinfoc.ch/biccss/poweredByDoxygen.png' width=100 height=20 alt=poweredByDoxygen></a>\n"
          "</td></tr></table></div>");
  }
  puts ("</body>\n</html>");
}
