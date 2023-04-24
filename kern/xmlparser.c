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
/** @file xmlparser.c
    @brief Quick and Dirty xml parser. This parser is, like the SAX parser,
    an event based parser, but with much less functionality.
    Module prefix xmlp_
*/
#include <ctype.h>
#include "log.h"
#include "format.h"
#include "xmlparser.h"

/// current mode of the parser: looking for text
#define TEXT 1
/// current mode of the parser: looking for entity
#define ENTITY 2
/// current mode of the parser: looking for open tag
#define OPEN_TAG 3
/// current mode of the parser: looking for closed tag
#define CLOSE_TAG 4
/// current mode of the parser: looking for start of tag
#define START_TAG 5
/// current mode of the parser: looking for attribute lvalue
#define ATTRIBUTE_LVALUE 6
/// current mode of the parser: looking for equal sign between lval and rval
#define ATTRIBUTE_EQUAL 9
/// current mode of the parser: looking for attribute rvalue
#define ATTRIBUTE_RVALUE 10
/// current mode of the parser: looking for quote
#define QUOTE 7
/// current mode of the parser: while in a tag
#define IN_TAG 8
/// current mode of the parser: while in tag that is opening and closing
#define SINGLE_TAG 12
/// current mode of the parser: while in comment
#define COMMENT 13
/// current mode of the parser: when done
#define DONE 11
/// current mode of the parser: in doc type
#define DOCTYPE 14
/// current mode of the parser: at the beginning
#define PRE 15
/// current mode of the parser: in data
#define CDATA 16

// define once to save string space
static char *MODNAME = "xmlparser";

static void (*startElement_hook)(char *tag, StringList attributes);
static void (*endElement_hook)(char *tag);
static void (*startDocument_hook)(void);
static void (*endDocument_hook)(void);
static void (*text_hook)(char *str);

void xmlp_init (void) {
  /**
     Initializes the parser
  */
  startElement_hook = NULL;
  endElement_hook = NULL;
  startDocument_hook = NULL;
  endDocument_hook = NULL;
  text_hook = NULL;
}

void xmlp_register_startDocument (void (*f)(void)) {
  /**
     Register a function to be called when the document starts.
     @param[in] f - the function
  */
  startDocument_hook = f;
}

void xmlp_register_endDocument (void (*f)(void))
{
  /**
     Register a function to be called when the document ends.
     @param[in] f - the function
  */
  endDocument_hook = f;
}

void xmlp_register_startElement (void (*f)(char *tag,StringList attributes)) {
  /**
     Register a function to be called when an element starts.<br>
     When f() is called, the 'attributes' are read-only to f().<br>
     The memory for 'attributes' is managed by this parser.
     @param[in] f - the function
  */
  /*
    How to use the attributes stringlist:
    Example 1:
    startElement (char *tag,StringList attrs) {
      if (strEqual (tag,"SEQUENCE")) {
        char *id = sl_getFirstP (attrs,"ID");
        if (id != NULL)
          printf ("id=%s\n",id);
        else
          puts ("id not defined");
      }
    }

    Example 2:
    startElement (char *tag,StringList attrs) {
      int j;
      char *name;
      char *value;
      if (sl_count (attrs))
        printf ("Element %s has attribute(s):\n",tag)
      for (j=0;j<sl_count (attrs);j++) {
        name = sl_getStrI (attrs,j);
        value = sl_getFirstP (attrs,name);
        printf ("  %s=%s\n",name,value);
      }
    }
  */
  startElement_hook = f;
}

void xmlp_register_endElement (void (*f)(char *tag)) {
  /**
     Register a function to be called when an element ends.
     @param[in] f - the function
  */
  endElement_hook = f;
}

void xmlp_register_text (void (*f)(char *str)) {
  /**
     Register a function to be called when text is found.
     @param[in] f - the function
  */
  text_hook = f;
}

static StringList gAttributes = NULL;
static Texta rvalues = NULL;

static void attributesInit (void) {
  if (gAttributes) {
    textClear(rvalues);
    sl_clear (gAttributes);
  }
  else {
    rvalues = textCreate (5);
    gAttributes = sl_create (5);
  }
}

static char nextChar (LineStream ls) {
  static char *line = NULL;
  static int lp = 0;
  static int returnNewline=0;
  char c;

  if (returnNewline) {
    returnNewline=0;
    return '\n';
  }
  if (line == NULL) {
    while ((line = ls_nextLine (ls)) != NULL)
      if (line[0]!='\0')
        break;
    lp = 0;
  }
  if (line == NULL)
    return  '\0';
  c = line[lp];
  lp++;
  if (line[lp] == '\0') {
    returnNewline=1;
    line = NULL;
  }
  return c;
}

void xmlp_run (LineStream ls) {
  /**
     Runs the parser and calls the registered functions if corresponding
     features found.
     @param[in] ls - a line stream to read from
  */
  int depth = 0;
  int mode = PRE;
  int c = 0;
  int quotec = '"';
  static Stringa sb = NULL;
  static Stringa etag = NULL;
  char *tagName = NULL;
  char *lvalue = NULL;
  int lineNum=1,colNum=0;
  int eol = 0;
  Stacka stack = NULL;

  stack = stackCreate (10,int);
  stringCreateClear (sb,100);
  stringCreateClear (etag,100);
  if (startDocument_hook)
    (*startDocument_hook) ();
  while ((c = nextChar (ls)) != '\0') {
    // We need to map \r, \r\n, and \n to \n: see XML spec section 2.11
    if (c == '\n' && eol) {
      eol = 0;
      continue;
    }
    else if (eol)
      eol = 0;
    else if (c == '\n') {
      lineNum++;
      colNum=0;
    }
    else if (c == '\r') {
      eol = 1;
      c = '\n';
      lineNum++;
      colNum=0;
    }
    else
      colNum++;

    if (mode == DONE) {
      if (endDocument_hook)
        (*endDocument_hook) ();
      stackDestroy (stack);
      return;
    }
    else if (mode == TEXT) { // collect text between tags
      if (c == '<') {
        stackPush (stack,mode,int);
        mode = START_TAG;
        if (stringLen (sb) > 0) {
          if (text_hook && !isBlankStr (string (sb)))
            (*text_hook)(string (sb));
          stringClear (sb);
        }
      }
      else if (c == '&') {
        stackPush (stack,mode,int);
        mode = ENTITY;
        stringClear (etag);
      }
      else
        stringCatChar (sb,(char)c);
    }
    else if (mode == CLOSE_TAG) { // e.g. </foo>
      if (c == '>') {
        mode = stackPop (stack,int);
        strReplace (&tagName,string (sb));
        stringClear (sb);
        depth--;
        if (depth==0)
          mode = DONE;
        if (endElement_hook)
          (*endElement_hook) (tagName);
      }
      else
        stringCatChar (sb,(char)c);
    }
    else if (mode == CDATA) {
      if (c == '>' && strEndsWith (string (sb),"]]")) {
        stringChop (sb, 2);
        if (text_hook && !isBlankStr (string (sb)))
          (*text_hook) (string (sb));
        stringClear (sb);
        mode = stackPop (stack,int);
      }
      else
        stringCatChar (sb,(char)c);
    }
    else if (mode == COMMENT) { // inside the <!-- .... --> looking for the -->
      if (c == '>' && strEndsWith (string (sb),"--")) {
        stringClear (sb);
        mode = stackPop (stack,int);
      }
      else
        stringCatChar (sb,(char)c);
    }
    else if (mode == PRE) { // outside the root tag element
      if (c == '<') {
        mode = TEXT;
        stackPush (stack,mode,int);
        mode = START_TAG;
      }
    }
    else if (mode == DOCTYPE) { // inside <? ... ?> or <!DOCTYPE ... >
      if (c == '>') {
        if (stackDepth (stack) > 0)
          mode = stackPop (stack,int);
        else
          mode = PRE;
        if (mode == TEXT)
          mode = PRE;
      }
    }
    else if (mode == START_TAG) { // after < : <foo> or </foo> or  <!-- ... --->, etc.
      mode = stackPop (stack,int);
      if (c == '/') {
        stackPush (stack,mode,int);
        mode = CLOSE_TAG;
      }
      else if (c == '?')
        mode = DOCTYPE;
      else {
        stackPush (stack,mode,int);
        mode = OPEN_TAG;
        attributesInit ();
        stringCatChar (sb,(char)c);
      }
    }
    else if (mode == ENTITY) { // e.g. &lt;, &#187;, etc.
      if (c == ';') {
        static char *cent = NULL;
        mode = stackPop (stack,int);
        strReplace (&cent,string (etag));
        stringClear (etag);
        if (strEqual (cent,"lt"))
          stringCatChar (sb,'<');
        else if (strEqual (cent,"gt"))
          stringCatChar (sb,'>');
        else if (strEqual (cent,"amp"))
          stringCatChar (sb,'&');
        else if (strEqual (cent,"quot"))
          stringCatChar (sb,'"');
        else if (strEqual (cent,"apos"))
          stringCatChar (sb,'\'');
        else if (strStartsWith (cent,"#x")) {
          unsigned int x;

          sscanf (cent+2,"%x",&x);
          stringCatInt (sb,x);
        }
        else if (strStartsWithC (cent,"#")) {
          int a;

          a = atoi (cent+1);
          stringCatChar (sb,a);
        }
        // Insert custom entity definitions here
        else
          warnAdd (MODNAME,
                   stringPrintBuf ("Unknown entity: &%s; near line %d col %d",
                                   cent,lineNum,colNum));
      }
      else
        stringCatChar (etag,(char)c);
    }
    else if (mode == SINGLE_TAG) { // after <foo a="b"/ looking for >
      //strReplace (&tagName,string (sb));
      if (c != '>')
        warnAdd (MODNAME,
                 stringPrintBuf("Expected > for tag: <%s/> near line %d col %d",
                                tagName,lineNum,colNum));
      if (startElement_hook)
        (*startElement_hook) (tagName,gAttributes);
      if (endElement_hook)
        (*endElement_hook) (tagName);
      if (depth == 0) {
        if (endDocument_hook)
          (*endDocument_hook) ();
        stackDestroy (stack);
        return;
      }
      stringClear (sb);
      attributesInit ();
      mode = stackPop (stack,int);
    }
    else if (mode == OPEN_TAG) { // something like <foo ... >, could still be <!-- ... -->
      if (c == '>') {
        strReplace (&tagName,string (sb));
        stringClear (sb);
        depth++;
        if (startElement_hook)
          (*startElement_hook)(tagName,gAttributes);
        attributesInit ();
        mode = stackPop (stack,int);
      }
      else if (c == '/')
        mode = SINGLE_TAG;
      else if (c == '-' && strEqual (string (sb),"!-"))
        mode = COMMENT;
      else if (c == '[' && strEqual (string (sb),"![CDATA")) {
        mode = CDATA;
        stringClear (sb);
      }
      else if (c == 'E' && strEqual (string (sb),"!DOCTYP")) {
        stringClear (sb);
        mode = DOCTYPE;
      }
      else if (isspace ((char)c)) {
        strReplace (&tagName,string (sb));
        stringClear (sb);
        mode = IN_TAG;
      }
      else
        stringCatChar (sb,(char)c);
    }
    else if (mode == QUOTE) { // quoted right-hand side of an element's attribute
      if (c == quotec) {
        textAdd (rvalues,string (sb));
        sl_addP (gAttributes,lvalue,textItem (rvalues,arrayMax (rvalues)-1));
        stringClear (sb);
        mode = IN_TAG;
      }
      else if (strchr (" \r\n\011",c)) // XML spec, section 3.3.3 on normalization processing
        stringCatChar (sb,' ');
      else if (c == '&') {
        stackPush (stack,mode,int);
        mode = ENTITY;
        stringClear (etag);
      }
      else
        stringCatChar (sb,(char)c);
    }
    else if (mode == ATTRIBUTE_RVALUE) {
      if (c == '"' || c == '\'') {
        quotec = c;
        mode = QUOTE;
      }
      else if (isspace ((char)c)) {
        ;
      }
      else
        warnAdd (MODNAME,
                 stringPrintBuf ("Error in attribute processing near line %d col %d",
                                lineNum,colNum));
    }
    else if (mode == ATTRIBUTE_LVALUE) {
      if (isspace ((char)c)) {
        strReplace (&lvalue,string (sb));
        stringClear (sb);
        mode = ATTRIBUTE_EQUAL;
      }
      else if (c == '=') {
        strReplace (&lvalue,string (sb));
        stringClear (sb);
        mode = ATTRIBUTE_RVALUE;
      }
      else
        stringCatChar (sb,(char)c);
    }
    else if (mode == ATTRIBUTE_EQUAL) {
      if (c == '=')
        mode = ATTRIBUTE_RVALUE;
      else if (isspace ((char)c)) {
        ;
      }
      else
        warnAdd (MODNAME,
                 stringPrintBuf ("Error in attribute processing near line %d col %d",
                                 lineNum,colNum));
    }
    else if (mode == IN_TAG) {
      if (c == '>') {
        mode = stackPop (stack,int);
        if (startElement_hook)
          (*startElement_hook) (tagName,gAttributes);
        depth++;
        attributesInit ();
      }
      else if (c == '/')
        mode = SINGLE_TAG;
      else if (isspace ((char)c)) {
        ;
      }
      else {
        mode = ATTRIBUTE_LVALUE;
        stringCatChar (sb,(char)c);
      }
    }
  }
  if (mode == DONE) {
    if (endDocument_hook)
      (*endDocument_hook) ();
  }
  else
    warnAdd (MODNAME,
             stringPrintBuf ("missing end tag near line %d col %d",
                             lineNum,colNum));
  stackDestroy (stack);
}
