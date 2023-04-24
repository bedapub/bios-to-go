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
/** @file format.h
    @brief The functions collected here belong into several groups:<br>
    1. string handling, where string is Array of char<br>
    2. str handling, where str is a '\0'-terminated char[]<br>
    3. text handling, where text is an Array of char*<br>
    4. line and word handling
    (reading a line of arbitrary length,
    reading quotes strings,
    nestable strtok(),
    dissecting strings of the form dbname:seqname)
*/
#ifndef FORMAT_H
#define FORMAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include "array.h"
#include "hlrmisc.h"
#include "plabla.h"

/* ------------ string handling functions / Array of char -------------- */
/**
   do not use 'String' since this is already used in e.g.
   /usr/include/X11/Intrinsic.h:
   typedef char *String;
*/
#define Stringa Array

extern Stringa stringCreate (int initialSize);

/**
  stringDestroy is a synonym of arrayDestroy
*/
#define stringDestroy arrayDestroy

/**
   assumes stringa is an Array of char with arr(a,arrayMax(a)-1,char)=='\0',
   i.e. containing a null-terminated string;<br>
   string(a) is just convenience
*/
#define string(stringa) arrp(stringa,0,char)

/**
  Returns a pointer to the character at index 'index'
*/
#define stringCp(stringa,index) arrp(stringa,index,char)

/**
  Returns the character at index 'index'
*/
#define stringC(stringa,index) arru(stringa,index,char)

/**
   A convenience function to obtain the length of a Stringa
*/
#define stringLen(stringa) (arrayMax(stringa)-1)
extern void stringTerminate (Array s /* of char */);
extern void stringTerminateP (Array s /* of char */,char *cp);
extern void stringTerminateI (Array s /* of char */,int i);
extern void stringAdjust (Stringa s1);
extern void stringChop (Stringa s,int n);
extern char *stringCut (Stringa s,int pos,int len);
extern void stringInsert (Stringa s,int p,char *i);
extern void stringCat (Stringa s1,char *s2);
extern void stringCatInt (Stringa s,int i);
extern void stringCatChar (Stringa s,char c);
extern void stringNCat (Stringa s1,char *s2,int n);
extern void stringCpy (Stringa s1,char *s2);
extern void stringNCpy (Stringa s1,char *s2,int n);
extern void stringClear (Stringa s1);
extern int stringTrim (Stringa s /* of char */,char *left,char *right);
extern int stringTranslate (Stringa s,char *fromChars,char *toChars);
extern int stringPrintf (Stringa str,const char *format,...);
extern int stringAppendf (Stringa str,const char *format,...);
extern char *stringPrintBuf (const char *format,...);

/**
   Create a string if it does not exist (is NULL) or clear it if it exists<br>
   This pattern occurs very often:<br>
   Stringa a = NULL;<br>
   if (a != NULL)<br>
     stringClear (a);<br>
   else<br>
     a = stringCreate (n);<br>
   therefore there is this shortcut.
   @param[in] s - the Stringa (can be NULL)
   @param[in] n - initial number of elements if it needs to be created
*/
#define stringCreateClear(s,n) {if(s) stringClear(s); else s=stringCreate(n);}

/**
   Create a string if it does not exist<br>
   Very similar stringCreateClear() if you don't need the string to be
   cleared<br>
   @param[in] s - the Stringa (can be NULL)
   @param[in] n - initial number of elements if it needs to be created
*/
#define stringCreateOnce(s,n) {if(!s) s=stringCreate(n);}

/* ------------ string handling functions / zero-terminated string --------- */
extern void toupperStr (char *s);
extern void tolowerStr (char *s);
extern char *strCaseStr (char *s,char *t);
extern void strReplace (char **s1,char *s2);
extern char *strCopySubstr (char *string,char begin,char end,Stringa substr);
extern int strTranslate (char *s,char *fromChars,char *toChars);
extern int strTrim (char *s,char *left,char *right);

/**
   A convenience function to find out whether two strings are different
*/
#define strDiffer(s1,s2) (strcmp((s1),(s2)) != 0)

/**
   A convenience function to find out whether two strings are equal
*/
#define strEqual(s1,s2) (strcmp((s1),(s2)) == 0)

#if BIOS_PLATFORM == BIOS_PLATFORM_IRIX || BIOS_PLATFORM == BIOS_PLATFORM_SOLARIS || BIOS_PLATFORM == BIOS_PLATFORM_LINUX || BIOS_PLATFORM == BIOS_PLATFORM_APPLE
/**
   A convenience function to find out whether two strings are case-insensitive
   different
*/
#define strCaseDiffer(s1,s2) (strcasecmp((s1),(s2)) != 0)

/**
   A convenience function to find out whether two strings are case-insensitive
   equal
*/
#define strCaseEqual(s1,s2) (strcasecmp((s1),(s2)) == 0)

/**
   A convenience function to find out whether the first n characters of two
   strings are case-insensitive different
*/
#define strNCaseDiffer(s1,s2,n) (strncasecmp((s1),(s2),n) != 0)

/**
   A convenience function to find out whether the first n characters of two
   strings are case-insensitive equal
*/
#define strNCaseEqual(s1,s2,n) (strncasecmp((s1),(s2),n) == 0)
#endif

#if BIOS_PLATFORM == BIOS_PLATFORM_WINNT
/**
   A convenience function to find out whether two strings are case-insensitive
   different
*/
#define strCaseDiffer(s1,s2) (stricmp((s1),(s2)) != 0)

/**
   A convenience function to find out whether two strings are case-insensitive
   equal
*/
#define strCaseEqual(s1,s2) (stricmp((s1),(s2)) == 0)

/**
   A convenience function to find out whether the first n characters of two
   strings are case-insensitive different
*/
#define strNCaseDiffer(s1,s2,n) (strnicmp((s1),(s2),n) != 0)

/**
   A convenience function to find out whether the first n characters of two
   strings are case-insensitive equal
*/
#define strNCaseEqual(s1,s2,n) (strnicmp((s1),(s2),n) == 0)
#endif

/**
   A convenience function to find out whether the first n characters of two
   strings are different
*/
#define strNDiffer(s1,s2,n) (strncmp((s1),(s2),n) != 0)

/**
   A convenience function to find out whether the first n characters of two
   strings are equal
*/
#define strNEqual(s1,s2,n) (strncmp((s1),(s2),n) == 0)

extern int strEndsWith (char *s,char *suffix);

/**
   Does s1 start with s2?
   strStartsWith() always works, but is slower than strStartsWithC()<br>
                   s2 must not be an expression with side effects<br>
                   since it is evaluated twice<br>
   strStartsWithC() same as strStartsWith, but can only be used<br>
                    if s2 is a string constant, e.g.<br>
                    if (strStartsWithC (s,"CC   ")) ...<br>
*/
#define strStartsWith(s1,s2) (strncmp((s1),(s2),strlen(s2)) == 0)

/**
   Does s1 start with s2?
   strStartsWith() always works, but is slower than strStartsWithC()<br>
                   s2 must not be an expression with side effects<br>
                   since it is evaluated twice<br>
   strStartsWithC() same as strStartsWith, but can only be used<br>
                    if s2 is a string constant, e.g.<br>
                    if (strStartsWithC (s,"CC   ")) ...<br>
*/
#define strStartsWithC(s1,s2) (strncmp((s1),(s2),sizeof(s2)-1) == 0)

/**
   Similar to strStartsWith
*/
#define strcaseStartsWith(s1,s2) (strncasecmp((s1),(s2),strlen(s2)) == 0)

/**
   Similar to strStartsWithC
*/
#define strcaseStartsWithC(s1,s2) (strncasecmp((s1),(s2),sizeof(s2)-1) == 0)

extern void strScramble (char *s);
extern void strUnscramble (char *s);
extern int isBlankStr (char *s);
extern int isEmptyString (Stringa s);

/* ----- Texta -- dealing with Arrays of strings -------------- */

/**
   a Texta is an Array of char*, where the memory referenced by
   the char pointers belongs to the Texta
*/
#define Texta Array

/**
   Creates a Texta for initially 'initialSize' elements
*/
#define textCreate(initialSize) arrayCreate(initialSize,char*)

/**
   Creates a Texta for initially 'initialSize' elements if it did not exist,
   otherwise clears the Texta (including de-allocating the memory of all its
   strings
*/
#define textCreateClear(s,n) {if(s) textClear(s); else s=textCreate(n);}

/**
   Creates a Texta for initially 'initialSize' elements if it did not exist,
   otherwise does not do anything
*/
#define textCreateOnce(s,n) {if(!s) s=textCreate(n);}

extern void textClear (Texta a);
extern void textDestroyFunc (Texta a);

/**
   Free a Texta: frees all string elements of the array. Use this,
   not testDestroyFunc
*/
#define textDestroy(x) ((x) ? textDestroyFunc(x), x=NULL, 1 : 0)

/**
   Adds a new element to a Texta, shortcut for an otherwise relatively
   complicated expression
*/
#define textAdd(t,s) (array((t),arrayMax(t),char*)=hlr_strdup(s))

/**
   Convenience function to access the element at index 'index' of a Texta
*/
#define textItem(text,index)  arru((text),(index),char*)

extern Texta textClone (Texta a);
extern Texta textFillG (int width,int widthOffset1,
                        char *s,char *splitControl);

/**
   A convenience function to call textFillG if widthOffset1 is 0
*/
#define textFill(width,s,splitControl) textFillG(width,0,s,splitControl)

extern Texta textStrtok (char *s,char *sep);
extern Texta textStrtokP (char *s,char *sep);
extern Texta textFieldtok (char *s,char *sep);
extern Texta textFieldtokP (char *s,char *sep);
extern void textJoin (Stringa s,char *sep,Texta t);
extern void textUniqKeepOrder (Texta t);  /* duplicate removal */
extern int textFind (Texta t,char *query);

/* --- dissecting a string -------------------------- */
extern char *dbseqDissect (char *dbseq);
extern int colonDissect (char *word,Stringa s1,Stringa s2);
extern char *strField (char *str,char sep);

/* --------- getLine(): like gets(), but with arbitrary length lines ------
  usage:
    static char *line = 0;
    static int buflen;
    int len;
    while (getLine (stdin,&line,&buflen)) {
      len = stripNlGetLength (line);
      printf ("%d %s\n",len,line);
    }
    hlr_free (line);
  notes:
   - 'buflen' is not the length of the returned line
   - getLine() does not strip the trailing newline
   - 'line' and 'buflen' belong tightly together
*/
extern int getLine (FILE *stream,char **buffer,int *buflen);
extern int stripNlGetLength (char *line);

/* --- dissect string into words, obey double quotes, e.g.: "one word" --- */
extern void wordSet (char *line,char *delims);
extern char *wordGet (void);
extern char *wordGetF (void);  /* get only filled words */

/* --- combination of strtok() and strField() with multiple instances --- */
// do not refer to anything inside this struct from your programs
/**
   Structure of the WordIter
*/
typedef struct wordIterStruct {
  char *cp; //!< current position
  char *seps; //!< separators: " \t" means that space or tab are separators
  int manySepsAreOne; //!< distinguish between field parsing (0) and token parsing (1)
  int atEnd; //!< whether parsing has arrived at the end
}*WordIter;

extern WordIter wordIterCreate (char *s,char *seps,int manySepsAreOne);
extern void wordIterDestroy_func (WordIter this1);
// ------------------------------------- end of private section

/**
   Convenience function for wordIterCreate in case 'manySeparatorsAreOne is 1
*/
#define wordTokIterCreate(s,seps) wordIterCreate(s,seps,1)

/**
   Convenience function for wordIterCreate in case 'manySeparatorsAreOne is 0
*/
#define wordFldIterCreate(s,seps) wordIterCreate(s,seps,0)

/**
   Not only destroys the wordIter but also sets it to NULL.
   Usage:<br>
   WordIter wi = wordTokIterCreate ("hallo welt"," \n\t");<br>
   char *w;<br>
   while (w = wordNext (wi))<br>
     printf ("w='%s'\n",w);<br>
   wordIterDestroy (wi);<br>
*/
#define wordIterDestroy(this1) (wordIterDestroy_func(this1),this1=NULL)

char *wordNextG (WordIter this1,int *lenP);
/**
   Convenience function for wordNextG in case we are not interested in the
   length of the word
*/
#define wordNext(this1) (wordNextG((this1),NULL))

#ifdef __cplusplus
}
#endif

#endif
