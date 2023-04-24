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
/** @file lnk.c
    @brief Knows how to find bioinformatics object identifiers in
    natural language text and hyperlink them using module biurl.
    Module prefix lnk_
*/
#include <ctype.h>
#include "log.h"
#include "biurl.h"
#include "lnk.h"

/// length of class name
#define LEN_class 40
/// length of object name
#define LEN_obj 60

/* List of identifiers that should not be hyperlinked.
   Add others if necessary */
static char *dbnamesIgnored[] = {"doi"};

static int lnk_isNumber (char *nr) {
  /**
     Checks if 'nr' is a realnumber.
     @param[in] nr - supposed to contain a realnumber
     @return 1 if number, else 0.
  */
  char *endptr;
  strtod (nr,&endptr);
  return *endptr == '\0';
}

TextLink lnk_textLinkCreate (char *url,int position,int type) {
  /**
     Creates a TextLink object
     @param[in] url
     @param[in] position - position of the url in the according string
     @param[in] type - type of the link (TL_SEQ_ID, TL_URL, TL_MARKED_URL)
  */
  TextLink this1 = (TextLink)hlr_malloc (sizeof (struct textLinkStruct));
  this1->url = hlr_strdup (url);
  this1->position = position;
  this1->type = type;
  return this1;
}

void lnk_textLinkDestroy (TextLink textLink) {
  /**
     Destroys a TextLink object
     @param[in] textLink - TextLink object
  */
  hlr_free (textLink->url);
  hlr_free (textLink);
}

void lnk_textLinksDestroy (Array textLinks) {
  /**
     Destroys an Array of TextLink objects
     @param[in] textLinks = Aray of type TextLink
  */
  int i;
  for (i=0;i<arrayMax (textLinks);i++)
    lnk_textLinkDestroy (arru (textLinks,i,TextLink));
  arrayDestroy (textLinks);
}

static int lnk_returnMarkedLink (char *text,int  position) {
  /**
     @param[in] text -
     @param[in] position - position of first char '<'
     @return length of the link if exists, else 0
  */
  // max length of the link. if it is longer it will not be linked
  int maxSeqLen = 255;
  int seqLen = 0;

  seqLen=1;
  while (text[position+seqLen] != '\0' &&
         !((text[position+seqLen] == '>') &&
           (text[position+seqLen+1] == '>'))) {
    seqLen++;
    if (seqLen > maxSeqLen ||
        ((text[position+seqLen] == '<') && (text[position+seqLen+1] == '<')))
      return 0;
  }
  return seqLen > 0 ? seqLen+2 : 0;
}

static int returnPdtLink (char *text,int textLen,int  position) {
  /**
     @param[in] text -
     @param[in] textLen - ==strlen(text), to avoid this to be calculated more
                          often then necessary.
     @param[in] position - position of "link="
     returns length of the link if exists, else 0
  */
  // max length of the link. if it is longer it will not be linked
  int maxSeqLen = 255;
  int seqLen = 1;
  int quotationMarkCount = 0;
  int colonFound=0;

  // warn("found PDT link at position: %d !<br>", position);
  while (position+seqLen < textLen && quotationMarkCount < 2) {
    if (text[position+seqLen] == '"')
      quotationMarkCount++;
    else if (!colonFound && text[position+seqLen] == ':')
      colonFound = 1;
    if (quotationMarkCount == 0 && seqLen > 6)
      return 0;
    if (seqLen > maxSeqLen)
      return 0;
    seqLen++;
  }
  return colonFound ? seqLen : 0;
}

static int lnk_ignoreHtmlHref (char *text,int position) {
  /**
     @param[in] input: text -
     @param[in] position - position of char ' ' of "<a "
     @return length of the link if exists, else 0
  */
  // max length of the link. if it is longer it will not be linked
  int maxLinkLen = 500;
  int seqLen = 0;

  seqLen=1;
  while (text[position+seqLen] != '\0') {
    if ((text[position+seqLen] == '<') && (text[position+seqLen+1] == '/') &&
        (text[position+seqLen+2] == 'a'))
      return seqLen+1;
    seqLen++;
    if (seqLen > maxLinkLen)
      return 0;
  }
  return 0;
}

static int lnk_ignoreHtmlImageRef (char *text,int position) {
  /**
     @param[in] text -
     @param[in] position - position of char ' ' of "<img "
     @return length of the link if exists, else 0
  */
  // max length of the link. if it is longer it will not be linked
  int maxLinkLen = 500;
  int seqLen = 0;

  seqLen=1;
  while (text[position+seqLen] != '\0') {
    if (text[position+seqLen] == '>')
      return seqLen+1;
    seqLen++;
    if (seqLen > maxLinkLen)
      return 0;
  }
  return 0;
}

static int lnk_checkLink (char *text,int position,int *startPosLeft,
                          int includeHTTP,int *type) {
  /**
     Checks if a word in 'text' including ':' could be a link
     @param[in] text -
     @param[in] position - position of char ':' in text
     @param[in] includeHTTP - 1: http-urls will be also extracted;
                              0: ignore http-urls
     @param[in] type - valid pointer
     @param[in] startPosLeft - valid pointer
     @param[out] *startPosLeft - number of character leftsided to position,
                                 where the link starts
     @param[out] *type - type of the link (TL_SEQ_ID, TL_URL, TL_MARKED_URL,
                         TL_RANNO_ID)
     Return length of the link if exists, else 0
  */
  /*
     example:
           01234567890
     text="a sw:p12345 xyz"
     position=4,
     returns 6, startPosLeft=2, type=TL_SEQ_ID
  */
  int i = 1;
  int j;
  char dbname[LEN_class+1] = "";
  char c;
  int maxSeqLen = LEN_obj;
  // max length of a NON-http link. if it is longer it will not be linked
  int maxHttpLen = 255;
  // max length of a http link. if it is longer it will not be linked
  int seqLen = 0;

  text[position] = '\0'; // mark ':' as end of string
  // get dbname
  c = text[position-i];
  while ((position-i) >=0 && (isalpha (c) || isdigit (c) || c == '_' )) {
    if (i > LEN_class) { // if dbname > database dbname size limit
      text[position] = ':'; // restore to original string
      return 0;
    }
    i++;
    c = text[position-i];
  }
  i--;
  hlr_strcpys (dbname,text+position-i);
  text[position] = ':'; // restore to original string
  /* skip ignored dbnames.
     Add odther dbnames to char *dbnamesIgnored[] if necessary */
  for (j=0;j<NUMELE (dbnamesIgnored);j++)
    if (strCaseEqual (dbname,dbnamesIgnored[j]))
      return 0;
  // because: http://www.abc.COM:8080/ =>  com:8080/
  if (((i==2) && strNCaseEqual (text+position-i,"de",2)) ||
      ((i==3) && (strNCaseEqual (text+position-i,"com",3) ||
                  strNCaseEqual(text+position-i,"net",3) ||
                  strNCaseEqual(text+position-i,"gov",3) ||
                  strNCaseEqual(text+position-i,"org",3))))
    return 0;
  // get seqid or http-url
  if (strCaseEqual (dbname,"http") || strCaseEqual (dbname,"file") ||
      strCaseEqual (dbname,"https")) {
    if (includeHTTP) {
      *type = TL_URL;
      *startPosLeft = i;
      seqLen=1;
      c = text[position+seqLen];
      while (c != '\0' &&
             !(c == '(' || c == ')' || c == '[' || c == ']' ||
               c == '{' || c == '}' || c == ',' || c == ';' ||
               c == '<' || c == '>' || isspace (c))) {
        seqLen++;
        c = text[position+seqLen];
        if (seqLen > maxHttpLen)
          return 0;
      }
      seqLen--;
    }
  }
  else if (dbname[0] != '\0' && !lnk_isNumber (dbname)) {
    int dsmzBlanksCnt=0;
    *startPosLeft = i;
    *type = TL_SEQ_ID;
    seqLen=1;
    tolowerStr (dbname);
    c = text[position+seqLen];
    while (c != '\0' &&
           (c == '_' || c == '/' || c == '-' || isalpha (c) ||
            c == '.' || c == '+' || c == '\\' || c == ';' ||
            c == ',' || c == '-' || c == '(' || c==')' ||
            c == '#' || c == '*' || c == ' ' || isdigit (c))) {
      // allow '.' only if genexis id (e.g. genexis:hs.11)
      if (c == '.' &&
          !((seqLen == 3 && strStartsWithC (dbname,"genexis")) ||
            strStartsWithC (dbname,"ec") ||
            strStartsWithC (dbname,"protcryst") ||
            strStartsWithC (dbname,"protbatch") ||
            strStartsWithC (dbname,"nmrd") ||
            strStartsWithC (dbname,"tox") ||
            strStartsWithC (dbname,"cell") ||
            strStartsWithC (dbname,"sasum") ||
            strStartsWithC (dbname,"ants") ||
            strStartsWithC (dbname,"expvector") ||
            strStartsWithC (dbname,"crystalexp") ||
            strStartsWithC (dbname,"crystal") ||
            strStartsWithC (dbname,"tml")))
        break;
      if (c == ' ') {
        if (strStartsWithC (dbname,"dsmz") && dsmzBlanksCnt <= 1)
          dsmzBlanksCnt++;
        else if (strStartsWithC (dbname,"protcryst") ||
                 strStartsWithC (dbname,"crystalexp") ||
                 strStartsWithC (dbname,"crystal") ||
                 strStartsWithC (dbname,"expvec") ||
                 strStartsWithC (dbname,"antibody"))
          ;
        else
          break;
      }
      if (c == '\\' && !strStartsWithC (dbname,"protcryst"))
        break;
      if (c==';' && !strStartsWithC (dbname,"alco"))
        break;
      if ((c == '(' || c == ')') &&
          !(strStartsWithC (dbname,"pdtproject") ||
            strStartsWithC (dbname,"alco") ||
            strStartsWithC (dbname,"tml") ||
            strStartsWithC (dbname,"expvector") ||
            strStartsWithC (dbname,"antibody") ||
            strStartsWithC (dbname,"protcryst")))
        break;
      if ((c == '+') && !((seqLen==3 && strStartsWithC (dbname,"proteomics")) ||
                          strStartsWithC (dbname,"crystalexp") ||
                          strStartsWithC (dbname,"protcryst") ||
                          strStartsWithC (dbname,"crystal") ||
                          strStartsWithC (dbname,"expvector")))
        break;
      if (((c == ',') || (c == '*')) &&
          !(strStartsWithC (dbname,"tml") ||
            strStartsWithC (dbname,"protcryst") ||
            strStartsWithC (dbname,"expvector") ||
            strStartsWithC (dbname,"alco")))
        break;
      if ((c == '#') &&
          !(strStartsWithC (dbname,"tml") ||
            strStartsWithC(dbname,"alco") ||
            strStartsWithC (dbname,"expvector") ||
            strStartsWithC (dbname,"protcryst")))
        break;
      seqLen++;
      c = text[position+seqLen];
      // check if srn and ern number are correct
      if (seqLen > 14 && strStartsWithC (dbname,"srn"))
        return 0;
      if (seqLen > 18 && strStartsWithC (dbname,"ern"))
        return 0;
      if (seqLen == 14 && c != '-' && strStartsWithC (dbname,"ern"))
        return 0;
      if (seqLen > maxSeqLen &&
          !(strStartsWithC (dbname,"protcryst") ||
            strStartsWithC (dbname,"expvector") ||
            strStartsWithC (dbname,"alco")))
        return 0;
    }
    seqLen--;
  }
  return seqLen > 0 ? seqLen+i+1 : 0;
}

TextLinks lnk_textParse (char *text,int includeHTTP) {
  /**
     Parses 'text' and returns the included dbseqnames/classObjectIds/urls
     and their positions. As an additional case, &lt;&lt;url&gt;&gt;,
     e.g. &lt;&lt;www.roche.com&gt;&gt;
     is recognized as a URL.
     @param[in] text - text to be parsed
     @param[in] includeHTTP - if NOT '0' http-urls will also be recognized
     @return Array of TextLink with the dbseqnames/classObjectIds/urls and
             their position in 'text' and link type; for use in
             lnk_addURLs2text() or lnk_addSWAcc2text()
             User is responsible for freeing the Array of textLink (use
             textLinksDestroy() ).
  */
  Array textLinks = NULL;
  int run = 1;
  int textLen;
  int len = 0;
  int position = 0;
  int startPosLeft = 0;
  int type = 0;
  static Stringa word = NULL;
  stringCreateOnce (word,10);
  textLinks = arrayCreate (20,TextLink);
  textLen = strlen (text);
  while (run) {
    if (position > textLen)
      break;
    else if (includeHTTP &&
             (text[position] == 'l' || text[position] == 'L') &&
             (text[position+1] == 'i' || text[position+1] == 'I') &&
             (text[position+2] == 'n' || text[position+2] == 'N') &&
             (text[position+3] == 'k' || text[position+3] == 'K') &&
             position+5 < textLen &&
             text[position+4] == '=') { // avoid unnecessary substring() calls
      len = returnPdtLink (text,textLen,position);
      if (len > 0)
        type = TL_PDT_URL;
      startPosLeft=0;
    }
    else if (text[position] == '<' && text[position+1] == '<' && includeHTTP) {
      len = lnk_returnMarkedLink (text,position);
      if (len > 0)
        type = TL_MARKED_URL;
      startPosLeft=0;
    }
    else if (text[position] == '<' && text[position+1] == 'a' && includeHTTP) {
      len = lnk_ignoreHtmlHref (text,position);
      position = position + len;
      len = 0; // do not add this link to textLinks
    }
    else if (text[position] == '<' &&
             (text[position+1] == 'i' || text[position+1] == 'I') &&
             (text[position+2] == 'm' || text[position+2] == 'M') &&
             (text[position+3] == 'g' || text[position+3] == 'G') &&
             includeHTTP) {
      len = lnk_ignoreHtmlImageRef (text,position);
      position = position + len;
      len = 0; // do not add this link to textLinks
    }
    else if (text[position] == ':') {
      len = lnk_checkLink (text,position,&startPosLeft,includeHTTP,&type);
    }
    if (len > 0) {
      stringNCpy (word,text+position-startPosLeft,len);
      array (textLinks,arrayMax (textLinks),TextLink) =
        lnk_textLinkCreate (string (word),position-startPosLeft,type);
      position = position + len - startPosLeft;
      len = 0;
    }
    position++;
  }
  return textLinks;
}

static int lnk_checkIfSWNameEqualAcc (char *swname) {
  /**
     Checks if 'swname' is an accession number
     @return 1 if swname is an acc nr. (e.g. sw:P00374),
             else 0 (e.g. sw:dyr_human)
  */
  char *id;
  if (!(id = strchr (swname,':'))) // do not include database name
    id = swname;
  if (strchr (id,'_')) // sw acc's cannot contain '_'
    return 0;
  return 1;
}

static SWName2SWAccFunc gSWName2SWAccFunc = NULL;

void lnk_registerSWName2SWAccFunc (SWName2SWAccFunc f) {
  /**
     Registers a functon to be called that can translate a swissprot name to
     an accession number
     @param[in] f - the function
  */
  gSWName2SWAccFunc = f;
}

Stringa lnk_addURLsOrSW2text (Array textLinks,char* text,int addOnlySWname) {
  /**
     Transforms the links in textLinks to a http-url and inserts it into 'text'
     if there is a seqname instead of an seqid, the corresponding seqid is
     also hyperlinked and added to text, e.g. sw:lipe_human [sw:Q9Y5X9].<br>
     Precondition: lnk_textParse(text)
     @param[in] textLinks - Array of TextLink including position, name and
                            type of links in text.
     @param[in] *text - text to modified
     @param[in] addOnlySWname - if '1' and if there is a sw seqname instead of
                                a seqid, the according seqid is hyperlinked
                                and added to text, e.g.
                                sw:lipe_human[sw:Q9Y5X9].
                                The other links are not considered.
     @return new Stringa containing modified text;
             user is responsible for freeing the Stringa returned.
  */
  int i;
  int position = 0;
  int preposition = 0; // position of the end of the last modified dbseqname
  int type;
  int dbseqnameLen;
  char dbseqname[255];
  char *acc;
  char *cp;
  Stringa modText;
  Stringa modURL;
  modText = stringCreate (100);
  modURL = stringCreate (10);
  for (i=0;i<arrayMax (textLinks);i++) {
    cp = arru (textLinks,i,TextLink)->url;
    dbseqnameLen = strlen (cp);
    hlr_assert (dbseqnameLen < sizeof (dbseqname),dbseqname);
    strcpy (dbseqname,cp);
    position = arru (textLinks,i,TextLink)->position;
    type = arru (textLinks,i,TextLink)->type;
    if (addOnlySWname && strNCaseEqual (dbseqname,"sw",2) &&
        (acc = gSWName2SWAccFunc (dbseqname)) &&
        !lnk_checkIfSWNameEqualAcc (dbseqname)) {
      char *start = NULL;
      stringCpy (modURL,dbseqname);
      // check if acc already included in text; text+position+dbseqnameLen=" [...]"
      // don't forget to check if length acc == length[...]
      if (!((start = strchr (text+position+dbseqnameLen,'[')) &&
            strchr (text+position+dbseqnameLen,']') &&
            strNCaseEqual (start+1,acc,strlen (acc)))) {
        stringCat (modURL," [");
        stringCat (modURL,acc);
        stringCatChar (modURL,']');
      }
      stringNCat (modText,text+preposition,position-preposition);
      stringCat (modText,string (modURL));
      preposition = position + dbseqnameLen;
    }
    else if (!addOnlySWname) {
      if (type==TL_SEQ_ID || type == TL_SEQ_NAME) {
        biurl_buildHtml (dbseqname,NULL);
        stringCpy (modURL,biurl_buildHtml (dbseqname,NULL));
      }
      else if (type == TL_URL) {
        stringCpy (modURL,"<a href=\"");
        stringCat (modURL,dbseqname);
        stringCat (modURL,"\">");
        stringCat (modURL,dbseqname);
        stringCat (modURL,"</a>");
      }
      else if (type == TL_MARKED_URL) {
        dbseqname[dbseqnameLen-2] = '\0';
        stringCpy (modURL,"<a href=\"");
        stringCat (modURL,dbseqname+2);
        stringCat (modURL,"\">");
        stringCat (modURL,dbseqname+2);
        stringCat (modURL,"</a>");
      }
      else if (type == TL_PDT_URL) {
        char *url = strchr (dbseqname,'"')+1;
        char *sep = strchr (dbseqname,';');
        char *name = NULL;
        if (sep) {
          *sep='\0';
          name = sep+1;
        }
        else
          name = url;
        strTrim (url,"\t\n\r  ","\t\n\r  \"");
        strTrim (name,"\t\n\r  ","\t\n\r  \"");
        stringCpy (modURL,"<a href=\"");
        stringCat (modURL,url);
        stringCat (modURL,"\">");
        stringCat (modURL,name);
        stringCat (modURL,"</a>");
      }
      else
        die ("lnk: unknown TextLink->type %d", type);
      stringNCat (modText,text+preposition,position-preposition);
      stringCat (modText,string (modURL));
      preposition = position + dbseqnameLen;
    }
  }
  stringCat (modText,text+preposition);
  stringDestroy (modURL);
  return modText;
}

Stringa lnk_addURLs2text (Array textLinks,char* text) {
  /**
     Transforms the links in textLinks to a http-url and inserts it into 'text'
     @param[in] textLinks - Array of TextLink created by textParse(text).
     @param[in] text - text to modified
     @return modified copy of text; user is responsible for freeing the Stringa
             returned.
  */
  return lnk_addURLsOrSW2text (textLinks,text,/*addOnlySWname*/0);
}

void lnk_text2html (char *text,Stringa outText) {
  /**
     Search words in a text which might be object names and hyperlink them;
     if 'text' starts with &lt;html&gt; it is assumed that 'text' is already
     hyperlinked; then 'outText' is made a mere copy of 'text'.
     @param[in] text - text to hyperlink
     @param[in] outText - Stringa, must exist
     @param[out] outText - hyperlinked version of text
  */
  TextLinks textLinks = lnk_textParse (text,1/*includeHTTP*/);
  Stringa s = lnk_addURLs2text (textLinks,text);
  stringCpy (outText,string (s));
  lnk_textLinksDestroy (textLinks);
  stringDestroy (s);
}
