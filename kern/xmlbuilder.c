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
/** @file xmlbuilder.c
    @brief Build a string in XML format.
    Module prefix xmlb_
*/

// #define ARRAY_CHECK 1

#include "log.h"
#include "format.h"
#include "xmlbuilder.h"

static void appendNBlanks (Stringa s,int n) {
  static Stringa blanks = NULL;
  int i;

  stringCreateOnce (blanks,10);
  if (n > stringLen (blanks))
    for (i=stringLen (blanks);i<n;i++)
      stringCat (blanks, " ");
  stringNCat (s,string (blanks),n);
}

/* ------------ begin local module stack of char* ------- */

static Texta gStck = NULL;
static int gLastOpWasPush;

static void stckInit (void) {
  textCreateClear (gStck,8);
  gLastOpWasPush = 0;
}

static void stckPush (char *s) {
  textAdd (gStck,s);
  gLastOpWasPush = 1;
}

static char *stckPop (int *lastOpWasPush) {
  static char *s = NULL;

  if (arrayMax (gStck) == 0)
    die ("stck underflow");
  hlr_free (s);
  s = textItem (gStck,arrayMax (gStck)-1);
  arraySetMax (gStck,arrayMax (gStck)-1);
  *lastOpWasPush = gLastOpWasPush;
  gLastOpWasPush = 0;
  return s;
}

static char *stckPeek (void) {
  if (arrayMax (gStck) == 0)
    die ("stck empty");
  return textItem (gStck,arrayMax (gStck)-1);
}

/// get the depth of the stack
#define stckDepth() (arrayMax (gStck))

/* ---------------- end submodule stck */

static Stringa gXml = NULL;
static int gAttributesOk = 0;
static int gDoIndent = 0;
static int gBaseIndentDepth = 0;

void xmlb_init (void) {
  /**
     Initializes the xml builder.<br>
     Precondition: none<br>
     Postcondition: the other xmlb_ functions can be called
  */
  stckInit ();
  stringCreateOnce (gXml,10);
  stringCpy (gXml,"<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>");
}

void xmlb_doIndentSet (int doIndent) {
  /**
     Whether to indent the xml text or not
     @param[in] doIndent - whether or not
  */
  gDoIndent = doIndent;
}

void xmlb_doctypeSystem (char *rootElement,char *dtdURI) {
  /**
     Add DOCTYPE SYSTEM.<br>
     Precondition: xmlb_init()
     @param[in] rootElement
     @param[in] dtdURI
  */
  stringCpy (gXml,"<?xml version=\"1.0\" encoding=\"iso-8859-1\" standalone=\"no\" ?>");
  stringAppendf (gXml,"%s<!DOCTYPE %s SYSTEM \"%s\">",
                 gDoIndent ? "\n" : "",rootElement,dtdURI);
}

void xmlb_embedded (int baseIndentDepth) {
  /**
     Sets the depath of the indent.<br>
     Precondition: last call was xmlb_init()<br>
     Postcondition: xmlb_get() will not add an XML header
                    and result will be indented by 'baseIndentDepth' spaces
     @param[in] baseIndentDepth
  */
  gBaseIndentDepth = baseIndentDepth;
  stringClear (gXml);
}

static void endAttribsIfNecessary (void) {
  if (gAttributesOk) {
    stringCat (gXml,">");
    gAttributesOk = 0;
  }
}

void xmlb_es (char *tag) {
  /**
     ElementStart<br>
     Postcondition: ee() must be called before get() will work again.
  */
  endAttribsIfNecessary ();
  if (gDoIndent) {
    stringCat (gXml,"\n");
    appendNBlanks (gXml,gBaseIndentDepth + stckDepth ());
  }
  stringCat (gXml,"<");
  stringCat (gXml,tag);
  stckPush (tag);
  gAttributesOk = 1;
}

/// append the chars from a string constant to an Array of char
#define scatC(s,a) scatn(s,a,sizeof(a)-1)

static void scatn (Array s,char *a,int n) {
  /**
     Append the first n chars from 'a' to 's'
     where 's' is an Array of char, not null-terminated,
     i.e. not a Stringa
     @param[in] s - Array of char, not Stringa
     @param[in] a - a string
     @param[in] n - where 0 < n <= strlen(a) must hold
     @param[out] s - char(s) appended
  */
  int l = arrayMax (s);
  array (s,l+n-1,char) = '\0'; // allocate
  /* copy; since it is so cheap to do, check that 'a' does
     not contain any '\0' char in the copied region */
  if (memccpy (arrp (s,l,char),a,'\0',n))
    die ("scat: nul in 'a'");
}


static void appendEncoded (Stringa xml,char *s) {
  /**
     Append the string 's' to xml, with encoding of <, >, &, etc.
  */
  char c;
  char *cp = s - 1;
  arrayMax (xml) = arrayMax (xml) - 1; // Stringa to Array of char
  while ((c = *++cp) != '\0') {
    if (c == '<')
      scatC (xml,"&lt;");
    else if (c == '>')
      scatC (xml,"&gt;");
    else if (c == '&')
      scatC (xml,"&amp;");
    else if (c == '"')
      scatC (xml,"&quot;");
    else if (c == '\'')
      scatC (xml,"&apos;");
    //scatC (xml,"&#39;");
    else
      array (xml,arrayMax (xml),char) = c;
  }
  array (xml,arrayMax (xml),char) = '\0'; // Array of char to Stringa
}

static void xmlb_aFunc (char *name,char *value,void(*catFunc)(Stringa, char*)) {
  /**
     Add String attribute to current element.<br>
     Precondition: ES() or A() was last call
     @param[in] name - name of the attibute
     @param[in] value - value of the attibute
     @param[in] catFunc - the cat function to use
  */
  if (!gAttributesOk)
    die ("trying to add attribute to closed element, tag=%s",stckPeek ());
  stringCat (gXml," ");
  stringCat (gXml,name);
  stringCat (gXml,"=\"");
  catFunc (gXml,value);
  stringCat (gXml,"\"");
}

void xmlb_aRaw (char *name,char *value) {
  /**
     Add String attribute 'name'='value' to current element;
     'value' must not contain any of the characters ", < or > .<br>
     Precondition: ES() or A() was last call.
     @param[in] name - name of the attibute
     @param[in] value - value of the attibute
  */
  xmlb_aFunc (name,value,stringCat);
}

void xmlb_a (char *name,char *value) {
  /**
     Same as xmlb_aRaw(), but the characters &, <, >, " and '
     get automatically HTML-encoded
     @param[in] name - name of the attibute
     @param[in] value - value of the attibute
  */
  xmlb_aFunc (name,value,appendEncoded);
}

void xmlb_aInt (char *name,int value) {
  /**
     Add integer attribute to current element.<br>
     Precondition: ES() or A() was last call
     @param[in] name - name of the attibute
     @param[in] value - value of the attibute
  */
  char str[HLR_ITOA_SIZE];
  hlr_itoa (str,value);
  xmlb_aRaw (name,str);
}

void xmlb_aDouble (char *name,double value,char *format) {
  /**
     Add double precision float valued attribute to current element.<br>
     Precondition: ES() or A() was last call
     @param[in] name - name of attribute
     @param[in] value - value of attribute
     @param[in] format - e.g. "%f" to format like printf ("%f",value)br>
                         "%.1f" to format with one digit after decimal
                         point.<br>
                         More options see manual for printf()
  */
  static Stringa s = NULL;

  stringCreateOnce (s,40);
  stringPrintf (s,format,value);
  xmlb_aRaw (name,string (s));
}

void xmlb_t (char *data) {
  /**
     Add string data to current element as text.<br>
     Precondition: ES() or A() or T()<br>
     Postcondition: A() cannot be called anymore
     @param[in] data - the data
  */
  endAttribsIfNecessary ();
  hlr_assert (data,"xmlb_t: no data");
  appendEncoded (gXml,data);
}

void xmlb_tInt (int data) {
  /**
     Add integer data to current element as text.<br>
     Precondition: ES() or A() or T()<br>
     Postcondition: A() cannot be called anymore
     @param[in] data - the interger to show as data
  */
  char str[HLR_ITOA_SIZE];
  hlr_itoa (str,data);
  xmlb_t (str);
}

void xmlb_ee (void) {
  /**
     Element End;
     for each ES() there must be a matching EE()<br>
     Precondition: ES() or A() or T()
  */
  int lastOpWasPush;
  char *tag;

  endAttribsIfNecessary ();
  tag = stckPop (&lastOpWasPush);
  if (gDoIndent && !lastOpWasPush) {
    stringCat (gXml,"\n");
    appendNBlanks (gXml,gBaseIndentDepth + stckDepth ());
  }
  stringCat (gXml,"</");
  stringCat (gXml,tag);
  stringCat (gXml,">");
}

char *xmlb_get (void) {
  /**
     Retrieve the xml text.<br>
     @return text constructed using es(),a(),t() and ee() calls
             memory returned belongs to this routine; read only to user;
             stable until next call.
  */
  if (stckDepth () != 0)
    die ("Element still open, tag=%s",stckPeek ());
  return string (gXml);
}
