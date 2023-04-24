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
/** @file format.c
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
#include <ctype.h>
#include <stdint.h>
#include "hlrmisc.h"
#include "log.h"
#include "format.h"

/* ---------- part 1: String = Array of char with
                      arru(string,arrayMax(string)-1,char) == '\0' ----- */

Array stringCreate (int initialSize) {
  /**
     Create an array of char and make it null-terminated,
     for use with stringCat() and C string functions
     @param[in] initialSize - the initial number of elements
     @return the Array
  */
  Array a = arrayCreate (initialSize,char);
  array (a,0,char) = '\0';
  return a;
}

void stringTerminate (Array s /* of char */) {
  /**
     @param[in] s - Array of char, not yet with trailing '\0'
     @param[out] s - now of type Stringa, i.e. an Array of char
                     with trailing '\0', suitable for use in
                     all string* functions;
  */
  if (s == NULL)
    die ("stringTerminate: null input");
  if (arrayMax (s) > 0 && arru (s,arrayMax (s)-1,char) == '\0')
    die ("stringTerminate: already terminated");
  array (s,arrayMax (s),char) = '\0';
}

void stringTerminateP (Array s /* of char */,char *cp) {
  /**
     Terminate string at position 'cp'<br>
     Note: stringTerminateP (s,string (s)) is equivalent to stringClear(s).
     @param[in] s - a valid Stringa
     @param[in] cp - a pointer to a position within the string
     @param[out] s - *cp in s will be '\0' and stringLen(s) is adjusted
  */
  stringTerminateI (s,cp-string (s));
}

void stringTerminateI (Array s /* of char */,int i) {
  /**
     Terminate string at the i+1. char<br>
     Note: stringTerminateI(s,0) is equivalent to stringClear(s)
     @param[in] s - a valid Stringa of length i
     @param[in] i - an index in s, 0<=i<=stringLen(s);
                    if i>stringLen(s) or i<0 is an error
     @param[out] s - *cp in s will be '\0' and stringLen(s) is adjusted
  */
  if (i < 0 || i > stringLen (s))
    die ("stringTerminateI: s='%s', i=%d",string (s),i);
  arru (s,i,char) = '\0';
  arraySetMax (s,i+1);
}

void stringAdjust (Stringa s1) {
  /**
     After having messed around in a string array by interting
     a '\0', the invariant arru(s1, arrayMax(s1)-1, char) == '\0'
     is violated.<br>
     This function cures it.
     @param[in] s1 - a Stringa
     @param[out] s1 - up to first '\0'
  */
  int l;
  if (s1 == NULL)
    die ("stringAdjust: null");
  if (arrayMax (s1) == 0) {
    stringClear(s1);
    return;
  }
  l = strlen (string(s1));
  if (l >= arrayMax (s1))
    die ("stringAdjust: memory allocation error? actual string length is %d, but array knows only about %d chars",
         l,arrayMax (s1)-1);
  arraySetMax (s1,l+1);
}

void stringChop (Stringa s,int n) {
  /**
     Remove the trailing n chars from s; if n is equal or larger than the
     length of s, the result is the empty string
     @param[in] s - a Stringa (arru (s,arrayMax (s)-1, char) == '\0')
     @param[in] n - how many chars to chop off, n>=0
     @param[out] s - characters chopped off
  */
  int l = arrayMax (s);
  if (n >= l) {
    stringClear (s);
    return;
  }
  arru (s,l-n-1,char) = '\0';
  arraySetMax (s,l-n);
  return;
}

char *stringCut (Stringa s,int pos,int len) {
  /**
     Removes the segment [pos..pos+len-1] from s and returns a pointer
     to a copy of this segment. If pos is beyond the end the string,
     nothing is done and NULL returned;<br>
     If pos is within the string but pos+len-1 is beyond the end of the string
     or len is -1, the tail of string starting at pos is removed.<br>
     Note: This memory stays stable until the next call to this routine.<br>
           This memory belongs to this routine; it may be read and<br>
           modified but not realloc'd or free'd by the user of this routine.
     @param[in] s - a Stringa
     @param[in] pos - start position of the segment to extract (based on 0);
                      pos >= 0
     @param[in] len - number of chars to cut out, -1 means as many as possible;
                      it is ok if len asks for more than is actually present;
                      if len is 0 s is not changed and an empty string returned.
     @param[out] s - without specified segment
     @return pointer to segment cut out
  */
  static Array r = NULL;
  int l = arrayMax (s) - 1; // don't count the terminating '\0'
  char *from;
  char *to;
  char c;

  if (l < 0 || arru (s,l,char) != '\0')
    die ("stringCut: not a proper Stringa");
  if (pos >= l)
    return NULL;
  if (len < 0 || len > l - pos)
    len = l - pos;
  stringCreateClear (r,len+1);
  if (len > 0) {
    stringNCpy (r,arrp (s,pos,char),len);
    to = arrp (s,pos,char);
    from = to + len - 1;
    while ((c = *++from) != '\0')
      *to++ = c;
    *to = '\0';
    arraySetMax (s,to - string (s) + 1);
  }
  return string (r);
}

void stringInsert (Stringa s,int p,char *i) {
  /**
     Insert string 'i' at position p into array string 's'
     @param[in] s - destination
     @param[in] p - 0 .. arrayMax(s)  (=strlen(string(s)))<br>
                    0 means prepending 'i' to 's',<br>
                    arrayMax(s) means appending (same as stringCat())<br>
     @param[in] i - source
     @param[out] s - the Stringa with i inserted
  */
  int il = strlen (i);
  int k = arrayMax (s) - p; // number of chars to move
  int oldlen = arrayMax (s)-1;
  int maxnew = oldlen+il;
  char *to;
  char *from;

  if (il == 0)
    return;
  array (s,maxnew,char) = '\0'; // allocate
  to = arrp (s,maxnew,char);
  from = arrp (s,oldlen,char);
  if (k < 0)
    die ("stringInsert: k=%d",k);
  while (k-- > 0)
    *to-- = *from--; // shuffle up to make space
  memcpy (arrp (s,p,char),i,il); // fill the space
  /*
    if (strlen (string (s)) != arrayMax (s)-1)
      die ("oops");
  */
}

void stringCat (Stringa s1,char *s2) {
  /**
     Appends null-terminated string s2 to s1, which is a Stringa,
     where always arru(s1, arrayMax(s1)-1, char) == '\0'<br>
     Equivalent functionality to strcat()
     @param[in] s1 - a Stringa
     @param[in] s2 - '\0'-terminated string
     @param[out] s1 - with contents of s2 appended
  */
  int i = arrayMax (s1) - 1; // index of the trailing \0
  int l = strlen (s2);
  if (arru (s1,i,char) != '\0')
    die ("stringCat: s1 is not null-terminated (i=%d)",i);
  array (s1,i + l,char) = '\0'; // allocate
  memcpy (arrp (s1,i,char),s2,l);
}

void stringCatInt (Stringa s,int i) {
  /**
     Convert i into a string and append it to Strings s
     @param[in] s - a Stringa
     @param[in] i - integer to be be appended to s
     @param[out] s - the Stringa with i appended
  */
  char c[30];
  hlr_itoa (c,i);
  stringCat (s,c);
}

void stringCatChar (Stringa s,char c) {
  /**
     Append single character 'c' to a Stringa 's'
     @param[in] s - a Stringa
     @param[in] c - character to be be appended to s
     @param[out] s - the Stringa with c appended
  */
  arru (s,arrayMax (s)-1,char) = c;
  array (s,arrayMax (s),char) = '\0';
}

void stringNCat (Stringa s1,char *s2,int n) {
  /**
     Appends first n chars from string s2 to s1, which is an Array of char,
     where always arru(s1, arrayMax(s1)-1, char) == '\0'<br>
     Same functionality as strncat()<br>
     Note: this is the same functionality, but slower:
     strncpy (arrp (s1,i,char),s2,l);<br>
     Example:<br>
     Stringa s = stringCreate (10);<br>
     stringClear (a);<br>
     stringNCat (a,"a",1); - result: "a"<br>
     stringNCat (a,"abc",5); - result: "aabc"<br>
     stringNCat (a,"abc",2); - result: "aabcab"<br>
     stringNCat (a,"a",0); - result: "aabcab"<br>
     stringNCat (a,"",1); - result: "aabcab"<br>
     stringNCat (a,"",0); - result: "aabcab"<br>
     @param[in] s1 - a Stringa
     @param[in] s2 - a C string; need not be null-terminated, but may not
                     contain internal nulls
     @param[in] n - number of chars from s2 to be copied,
                    if n <= 0, s1 is not changed.
                    n larger than the length of s2 is ok, too.
     @param[out] s1 - with first min(n,strlen(s2)) chars from s2 appended
  */
  int i;
  int l = 0; // length of s2 --> number of chars to copy
  char *cp = s2 - 1;
  if (n <= 0)
    return;
  while (*++cp != '\0')
    if (++l >= n)
      break;
  // now l holds number of chars to copy
  i = arrayMax (s1) - 1; // index of the trailing \0
  if (arru (s1,i,char) != '\0')
    die ("stringNCat: s1 is not null-terminated (i=%d)",i);
  array (s1,i + l,char) = '\0'; // allocate and terminate string
  memcpy (arrp (s1, i, char),s2,l); // as fast as possible
}

void stringCpy (Stringa s1,char *s2) {
  /**
     Copies null-terminated string s2 to s1, which is an Array of char,
     where always arr(s1, arrayMax(s1)-1, char) == '\0'.<br>
     Equivalent functionality as strcpy()
     @param[in] s1 - a Stringa
     @param[in] s2 - a C string
     @param[out] s1 - with contents of s2
  */
  arraySetMax (s1,0);
  array (s1,strlen (s2),char) = '\0'; // allocate
  strcpy (arrp (s1,0,char),s2);
}

void stringNCpy (Stringa s1,char *s2,int n) {
  /**
     Analogous to strncpy() from the C library<br>
     Analogous to stringNCat() from this module, except that
     s1 is cleared before appending
  */
  stringClear (s1);
  stringNCat (s1,s2,n);
}

void stringClear (Stringa s1) {
  /**
     Erases the contents of Stringa s1, leaving an empty
     string suitable for use in stringCat() etc.
     @param[in] s1 - a Stringa
     @param[out] s1 - the Stringa cleared
  */
  arraySetMax (s1,0);
  array (s1,0,char) = '\0';
}

int stringTrim (Stringa s,char *left,char *right) {
  /**
     Exactly like strTrim(), but with a Stringa
     @param[in] s - must exist
     @param[in] left - characters to be trimmed on the left
     @param[in] right - characters to be trimmed on the right
     @param[out] s - trimmed
     @return final length of s
  */
  int len = strTrim (string (s),left,right);
  arraySetMax (s,len+1);
  return len;
}

static int strTranslate_resultLen = 0; // set by strTranslate

int stringTranslate (Stringa s,char *fromChars,char *toChars) {
  /**
     Exactly like strTranslate(), but with a Stringa
     @param[in] s - must exist
     @param[in] fromChars - characters to be translated
     @param[in] toChars - resulting characters
     @param[out] s - some characters translated (or deleted)
     @return number of chars translated or modified
  */
  int cnt = strTranslate (string (s),fromChars,toChars);
  arraySetMax (s,strTranslate_resultLen+1);
  return cnt;
}

int strEndsWith (char *s,char *suffix) {
  /**
     Does string 's' end with 'suffix'?
     @param[in] s - a string, not NULL
     @param[in] suffix - not NULL; if empty, 1 will be returned
     @return 1 if yes, 0 if no
  */
  int len = strlen (s);
  int slen = strlen (suffix);
  if (slen > len)
    return 0;
  return strEqual (s + len - slen,suffix);
}

static int computeResultLength (const char *format,va_list args) {
  /**
     For use in stringPrintf() and stringAppendf() --
     function estimates maximum length of output by looking at
     format string and arguments and extends string before printing
     into it
  */
  int deflength = 18; // default size for int/float/pointer types
  int maxlength = 0; // estimated maximum string length
  const char *cp = format;
  int isPercent = 0; // flag for inside/outside conversion specification

  // parse format string to estimate maximum length
  while (*cp != '\0') {
    // toggle percent flag to handle escaped '%'s correctly
    if (*cp == '%')
      isPercent = (isPercent) ? 0 : 1;
    else if (isPercent) {
      // handle one conversion specification e.g. %20.10s, %5.2f, etc
      intptr_t width = 0;
      intptr_t prec = 0;
      int dot = 0; // flag for dot found
      int asteriskCnt = 0; // flag/counter for value substitution by '*'
      long int intConv = 0;
      char *intConvEnd = NULL;
      char *arg = NULL;
      int arglength = 0;

      for (;;) {
        // try to read an integer
        intConv = strtol (cp,&intConvEnd,10);
        if (intConvEnd != cp) { // if an integer was found
          cp = intConvEnd; // step over integer found
          // assign to width or prec depending on dot-found flag
          if (dot)
            prec = intConv;
          else
            width = intConv;
        }
        else if (*cp == '*') // if substitution count asterisks to avoid crash
          asteriskCnt ++;
        if (strchr ("lLh",*cp) != NULL) // skip potential extensions of conversion specifier
          cp++;
        if (*cp == 'l')
          cp ++;
        if (strchr ("pdinouxXeEfgGcCsS",*cp) != NULL) { // if conversion type specifier found
          // reset flags and get pointer to current argument
          isPercent = 0;
          if (strchr ("eEfgG",*cp) != NULL) {
            /* on IRIX 6.5 using the O32 ABI there is a bug in
               va_arg(x,t): it always sizeof(t) from the arg list
               instead of the actual argument size; obviously this
               leads to garbarge with 8byte types like float when
               using va_arg(x,char*).
               The problem has been fixed by SGI in the N32 ABI.
               The workaround for O32 is this code:
            */
            va_arg (args,double);
            arg = NULL;
          }
          else
            arg = va_arg (args,char *);
          if (asteriskCnt > 0) {
            if (asteriskCnt == 2) {
              width = (intptr_t)arg;
              arg = va_arg (args,char *);
              prec = (intptr_t)arg;
              arg = va_arg (args,char *);
            }
            else if (asteriskCnt == 1) {
              if (dot)
                prec = (intptr_t)arg;
              else
                width = (intptr_t)arg;
              arg = va_arg (args,char *);
            }
            else
              die ("stringPrintf(): cannot handle format string %s",format);
          }
          if (*cp == 's') {
            // estimate max length for strings
            if (prec > 0)
              arglength = prec;
            else
              arglength = arg ? strlen(arg) : 6; // (null)
            maxlength += MAX (arglength,width);
          }
          else if (*cp == 'S') {
            /* do we need that? it's not tested. #include <widec.h>
               if (prec > 0)
                 arglength = prec * sizeof(wchar_t);
               else
                 arglength = wssize((wchar_t *) arg) * sizeof(wchar_t);
               maxlength += MAX(arglength, width * sizeof(wchar_t));
            */
            die ("stringPrintf(): wide character strings not handled");
          }
          else if (*cp == 'c') {
            maxlength ++;
          }
          else if (*cp == 'C') {
            /* see above
               maxlength += sizeof(wchar_t);
            */
            die ("stringPrintf(): wide characters not handled");
          }
          else {
            // estimate max length for all other types
            maxlength += (deflength + MAX (width,prec));
          }
          // end of conversion type specification, exit loop
          break;
        }
        else {
          // if stopped on a dot set flag
          if (*cp == '.')
            dot = 1;
          // if stopped on a dollar complain
          if (*cp == '$')
            warn ("stringPrintf(): argument position manipulation not supported");
          cp++;
        }
      }
    }
    maxlength++;
    cp++;
  }
  return maxlength;
}

int stringPrintf (Stringa str,const char *format,...) {
  /**
     Formatted printing into a Stringa (similar to sprintf())
     @param[in] str - target string which receives formatted printing
                      as in sprintf(): must not be NULL;
                      must be created using stringCreate()
     @param[in] format - format string as in sprintf(), only difference:
                         argument position manipulation e.g. "%2$15.12s"
                         or "%*3$s" is not supported
     @param[in] ... - variable argument list, must match format string, else
                      behaviour is undefined
     @param[out] str - contains formatted output
     @return number of characters printed into str not counting trailing '\0'
  */
  va_list args;
  int maxlength;
  int resultLen;

  va_start (args,format);
  maxlength = computeResultLength (format,args);
  va_end (args);
  array (str,maxlength + 1,char) = '\0'; // allocate space
  va_start (args,format);
  resultLen = vsprintf (string (str),format,args);
  va_end (args);
  if (resultLen > maxlength || resultLen < 0)
    die ("stringPrintf(): result length prediction failed. Memory corrupted. Abort for safety. Possible cause: %%f specified and printed representation of numeric value requires more then deflength = 18 bytes. Possible fix: use %%g as format string.");
  arraySetMax (str,resultLen+1);
  return resultLen;
}

int stringAppendf (Stringa str,const char *format,...) {
  /**
     Same as stringPrintf(), but the result is appended to 'str';
     @return number of chars appended
  */
  va_list args;
  int maxlength;
  int len = stringLen (str);
  int resultLen;

  va_start (args,format);
  maxlength = computeResultLength (format,args);
  va_end (args);
  array (str,len + maxlength + 1,char) = '\0'; // allocate space
  va_start (args,format);
  resultLen = vsprintf (string (str)+len,format,args);
  va_end (args);
  if (resultLen > maxlength || resultLen < 0)
    die ("stringAppendf(): oops");
  arraySetMax (str,len+resultLen+1); // turn into Stringa
  return resultLen;
}

char *stringPrintBuf (const char *format,...) {
  /**
     Very much like sprintf() from the standard C library, but
     with unlimited length and own memory management.<br>
     Memory managed by this routine; may be written to by user,
     but not free'd or realloced; stable until next call to this routine.<br>
     NOTE THAT THIS IS A SINGLETON. THEREFORE:
     DO NOT USE THIS FUNCTION WITHIN BIOS -- OTHERWISE A USER MIGHT
     HAVE SURPRISES WHEN DOING THINGS LIKE someBiosFunc(stringPrintBuf(...)).<br>
     ALSO someFunc(stringPrintBuf(...), stringPrintBuf(...)) DOES NOT WORK.
     @param[in] format - template to be filled
     @param[in] ... - variable number of arguments, must match 'format'
     @return formatted string;
  */
  static Stringa str = NULL;
  va_list args;
  int maxlength;
  int resultLen;

  stringCreateOnce (str,100);
  va_start (args,format);
  maxlength = computeResultLength (format,args);
  va_end (args);
  array (str,maxlength + 1,char) = '\0'; // allocate space
  va_start (args,format);
  resultLen = vsprintf (string (str),format,args);
  va_end (args);
  if (resultLen > maxlength || resultLen < 0)
    die ("stringPrintBuf(): oops");
  arraySetMax (str,resultLen+1); // turn into Stringa
  return string (str);
}

/* ------------- part 2: char str[], '\0'-terminated --------- */

void toupperStr (char *s) {
  /**
     Converts Stringa to uppercase
     @param[in] s the Stringa
  */
  register char *cp = s - 1;
  while (*++cp != '\0')
    *cp = toupper (*cp);
}

void tolowerStr (char *s) {
  /**
     Converts Stringa to lowercase
     @param[in] s the Stringa
  */
  register char *cp = s - 1;
  while (*++cp != '\0')
    *cp = tolower (*cp);
}

char *strCaseStr (char *s,char *t) {
  /**
     Case-insensitive version of strstr(3C) from the C-libarary
     @param[in] s - string to be searched in (subject)
     @param[in] t - string to look for in s (query)
     @return if t is the empty string return s,
             else if t does not occur in s return NULL,
             else pointer to first position of t in s
  */
  char *p,*r;
  if (*t == '\0')
    return s;
  for (;*s!='\0';s++) {
    for (p=s,r=t;*r!='\0' && tolower (*p) == tolower (*r);p++,r++);
    if (r > t && *r == '\0')
      return s;
  }
  return NULL;
}

void strReplace (char **s1,char *s2) {
  /**
     Replace previous contents of s1 with copy of s2. s2 can be NULL.<br>
     This function is the same as strdup() from the C library, except
     that it free()s the target string before duplicating.
     @param[in] s1 - place where a pointer to a C-string is stored
     @param[in] s2 - contents of s2 will replace contents of s1; s2 can be NULL
     @param[out] s1 - previous contents free()d, new memory allocated
  */
  if (s1 == NULL)
    die ("strReplace: NULL");
  hlr_free (*s1);
  if (s2 != NULL)
    *s1 = hlr_strdup (s2);
}

char *strCopySubstr (char *string,char begin,char end,Stringa substr) {
  /**
     From a supplied string copy a substring delimited by two
     supplied characters, excluding these characters
     @param[in] string - string to copy from
     @param[in] begin - start copying after the leftmost
                        occurrence of this character in string
     @param[in] end - stop copying before the leftmost occurrence
                      of this character from occurrence of begin on;
                      may be '\0' to copy to the end of string
     @param[in] substr - a Stringa, must exist
     @param[out] substr - filled with string extracted;
                          empty string if nothing extracted;
     @return position after location of end, NULL if no substring extracted
  */
  char *pbegin = NULL;
  char *pend = NULL;
  char *nextPos = NULL;

  stringClear (substr);
  if ((pbegin = strchr (string,begin)) != NULL) {
    pbegin++;
    if ((pend = strchr (pbegin,end)) != NULL) {
      nextPos = pend + 1;
      pend--;
      stringNCpy (substr,pbegin,pend - pbegin + 1);
    }
  }
  return nextPos;
}

int strTranslate (char *s,char *fromChars,char *toChars) {
  /**
     Translates each character from 's' which matches one
     of the characters in 'fromChars' with the corresponding
     character from 'toChars' or, if this position in 'toChars'
     is not filled, deletes this character from s, thus
     shortening 's'.<br>
     This function resembles the Unix command and the Perl function 'tr'.<br>
     Example: strTranslate ("abc","ac","b") modifies "abc" into "bb" and
                                            returns 2<br>
              strTranslate ("a|b|c","|","|") just counts the number of '|' chars<br>
     Postcondition: strTranslate_resultLen contains the length of the string
     after processing; used by stringTranslate() to avoid using strlen()
     @param[in] s - a \0 terminated string
     @param[in] fromChars -
     @param[in] toChars -
     @param[out] s = the modified string
     @return number of chars translated or modified
  */
  char *from = s - 1;
  char *to = s;
  char c;
  int toLen = strlen (toChars);
  char *hit;
  int cnt = 0;

  while ((c = *++from) != '\0') {
    if ((hit = strchr (fromChars,c)) != NULL) {
      cnt++;
      if (hit - fromChars < toLen)
        *to++ = toChars[hit - fromChars];
    }
    else
      *to++ = c;
  }
  *to = '\0';
  strTranslate_resultLen = to - s;
  return cnt;
}

int strTrim (char *s,char *left,char *right) {
  /**
     Remove leading and trailing characters from s<br>
     Example: strTrim ("<<=text=>>","=<","=")
              returns 7 and leaves output "text=>>"
     @param[in] s - zero-terminated string of char
                    or NULL (nothing will happen)
     @param[in] left - set of chars to be removed from left end of s
                       NULL or empty string to leave beginning of s as is
     @param[in] right - set of chars to be removed from right end of s
                        NULL or empty string to leave tail of s as is
     @param[out] s - changed
     @return length of s after trim
  */
  int len;
  char *cp;
  if (s == NULL)
    return 0;
  len = strlen (s);
  if (len > 0 && right != NULL) {
    /* move to last char of sequence, then run as long as
       there are only chars from 'right' or we bump into the
       beginning of the string */
    cp = s + len - 1;
    while (cp >= s && strchr (right,*cp) != NULL)
      *cp-- = '\0';
    len = cp - s + 1;
  }
  if (len > 0 && left != NULL) {
    /* move cp to the first char not in 'left'; then start
       shuffling chars from this position onward down to the beginning
       of the string */
    cp = s;
    while (*cp && strchr (left,*cp) != NULL)
      ++cp;
    len = len - (cp - s);
    while (*cp != '\0')
      *s++ = *cp++;
    *s = '\0';
  }
  return len;
}

void strScramble (char *s) {
  /**
     'encrypt' the input string such that it is unreadable to humans
     and can easily be strUnscrambled() again
     @param[in] s - must not contain 0xFF
     @param[out] s - scrambled
  */
  char *cp = s - 1;
  while (*++cp != '\0') {
    if ((unsigned char)*cp == 255)
      die ("strScramble: cannot scramble 0xFF");
    *cp = *cp ^ 255;
  }
}

void strUnscramble (char *s) {
  /**
     antidot for strScramble()
     @param[in] s - scrambled
     @param[out] s - unscrambled
  */
  strScramble (s); // scramble + scramble = unscramble
}

int isBlankStr (char *s) {
  /**
     Returns 1, if string consists of whitespace only or is of length 0,
     else returns 0
     @param[in] s - the string
  */
  char *cp = s-1;
  while (*++cp != '\0' && isspace (*cp));
  return *cp == '\0';
}

int isEmptyString (Stringa s) {
  /**
     Returns 1 if the Stringa is not empty, otherwise 0
     @param[in] s - the Atringa
  */
  if (s == NULL || arrayMax (s) < 1)
    die ("isEmptyString: NULL or corrupt string");
  return arru (s,0,char) == '\0';
}

/* -------- part 3: text handling, where text is an Array of char[] ----- */

void textClear (Texta a) {
  /**
     Free the strings in 'a' and make the array empty.
     the strings must have been allocated using hlr_malloc()
     or hlr_strdup() if you expect hlr_getAllocCnt() to work.
     @param[in] a - the Texta
     @param[out] a - the Texta memory freed and zero elements
  */
  int i = arrayMax (a);
  while (i-- > 0)
    hlr_free (textItem (a,i));
  arrayClear (a);
}

void textDestroyFunc (Texta a) {
  /**
     Free storage allocated to Texta.<br>
     Note: This function is private to this module.
           Don't call it from your program.
           Use macro textDestroy() instead
     @param[in] a - a Texta; the char* should have been allocated
                    with hlr_malloc()
  */
  if (a == NULL)
    return;
  textClear (a);
  arrayDestroy (a);
}

Texta textClone (Texta a) {
  /**
     Produce a clone of the Array of strings 'a'.<br>
     Note: New memory is allocated, the user of this routine is responsible for
     freeing the memory allocated (textDestroy())
     @param[in] a - Array of strings, NULL ok
     @return Array of char* with same contents as 'a'
  */
  int i;
  Texta b;
  char *s;
  if (a == NULL)
    return NULL;
  i = arrayMax (a);
  b = textCreate (i); // original size
  if (i == 0)
    return b;
  array (b,i-1,char*) = NULL; // access to make arru() work
  while (i-- > 0) {
    s = arru (a,i,char*);
    arru (b,i,char*) = (s ? hlr_strdup (s) : NULL);
  }
  return b;
}

static int gWidthOffset1;

static void addLine (Texta res,char *begin,char *upto,int *width) {
  /**
     Special function for use by textFillG() only --
     create a null-terminated string consisting of chars
     from 'begin' upto but excluding 'upto' and append it to Array 'res'.<br>
     Precondition: gWidthOffset1 is set.<br>
     Postcondition: gWidthOffset1 is 0.
     @param[in] res - an Array of null-terminated strings (Texta)
     @param[in] begin - pointer to the beginning of a string
     @param[in] end - pointer to a position within the string 'begin'
     @param[in] *width - address of line width
     @param[out] res - with new string appended; initial tabs and
                 spaces from 'begin' are not included.
     @param[out] *width - on first call, changed by negative gWidthOffset1
  */
  char *to,*from,*s;
  int l;
  while (*begin == ' ' || *begin == '\t')
    begin++;
  l = upto - begin + 1;
  s = (char *)hlr_malloc((l > 1) ? l : 1);
  to = s;
  from = begin;
  while (from < upto)
    *(to++) = *(from++);
  *to = '\0';
  array (res,arrayMax (res),char*) = s;
  *width -= gWidthOffset1;
  gWidthOffset1 = 0; // do it only for first line
}

Texta textFillG (int width,int widthOffset1,char *s,char *splitControl) {
  /**
     Splits the string 's' at whitespace into lines of maximum width 'width',
     if possible. If there is no whitespace to split on, splits on
     the chars given in 'splitControl'. If even this does not help,
     cuts at line width, resulting in cuts within words.<br>
     If 'splitControl' starts with ' ' (blank), splitting on white space
     (tabs and blanks) is suppressed, i.e. only the remaining 'splitControl'
     char are recognized as break points.<br>
     New line in the input string forces a break.<br>
     The resulting strings do not contain any line breaks and have
     no leading tabs or spaces.<br>
     If a break occurs within a region of several whitespaces this
     region is treated as one single whitespace. Otherwise stretches
     of whitespace are preserved, e.g. two spaces remain two spaces.<br>
     This means that using textFill on a string and concatenating the pieces
     returned by textFill with intervening line breaks will only reconstruct
     the original string, if it did not contain series of whitespace chars.<br>
     A split on one of the 'splitControl' leaves the split character
     on the current line.<br>
     Bugs: two line breaks in a row are not always handled correctly
           (we don't even know what would be correct)
     @param[in] width - maximum line width allowed
     @param[in] widthOffset1 - how much the width of first line should
                               differ from the width of the subsequent lines
     @param[in] s - string to split
     @param[in] splitControl - NULL or empty string: split on whitespace only;
                               "./" split on dot and slash, too, 2nd priority;
                               " ;" if 'splitControl' starts with blank,
                               split on whitespace is turned off - only
                               chars after the blank are used to split.
     @return an Array of pointers to strings.<br>
             The user of this routine is responsible for free'ing
             the Array returned after use; (e.g. using textDestroy() )
  */

  char *cp;
  char *spos; // look back position
  char c;
  int altSplitFound; // 1, if one of the splitControl found
  char *ws_begin = NULL; // whitespace begin
  char *ws_end = NULL;
  Texta res; // result array
  char *line; // start of current line in s
  int ws = 0; // true, if last char was a whitespace, else false
  int wsSplit = 1; // 1 if split on blank and tab ok; 0 = suppress

  cp = s - 1;
  res = textCreate (1);
  ws_begin = 0;
  line = s;
  width += widthOffset1;
  gWidthOffset1 = widthOffset1;
  if (splitControl != NULL && *splitControl == ' ') {
    splitControl++;
    wsSplit = 0;
  }
  while ((c = *++cp) != '\0') {
    if (c == '\n') {
      addLine (res,line,cp,&width);
      line = cp + 1;
      ws = 0;
      if (*++cp == '\0') // skip the \n;
        break; // are we at end of input?
      if (*cp == '\n') // but don't skip in case of consecutive \n
        cp--;
    }
    else if (wsSplit > 0 && isspace (c)) {
      if (ws == 0) {
        ws_begin = cp;
        ws = 1;
      }
      ws_end = cp;
    }
    else { // c is a non-whitespace-char and not \n
      ws = 0;
      if (cp - line >= width) { // char does not fit anymore on line
        if (ws_begin != NULL) {
          addLine (res,line,ws_begin,&width);
          line = ws_end+1; // skip the whitespace
          ws_begin = 0; // flag as done
        }
        else { // there was no withspace on the line, so try other cutters
          altSplitFound = 0;
          if (splitControl != NULL && *splitControl != '\0') {
            spos = cp; // look backwards for one of the alternate split chars
            while (--spos > line)
              if (strchr (splitControl,*spos) != NULL) {
                altSplitFound = 1;
                break;
              }
          }
          if (altSplitFound)
            cp = spos+1;
          addLine (res,line,cp,&width); // up to, but not including cp
          line = cp;
        }
      } // max. width reached
    } // non-whitespace char
  } // while
  addLine (res,line,ws ? ws_begin : cp,&width);
  return res;
}

Texta textStrtok (char *s,char *sep) {
  /**
     Splits string 's' in words separated by one or more characters
     from string 'sep'.<br>
     Same result as repeated calls to strtok() from the standard C libary.
     @param[in] s - input string
     @param[in] sep - separation character(s)
     @param[out] s - the contents changed!
     @return an Array of pointers to C-strings<br>
             The user of this routine is responsible for freeing
             the Array returned after use; (e.g. using textDestroy() )
  */
  WordIter wi = wordIterCreate (s,sep,1);
  char *pos;

  Texta a = textCreate (10);
  while ((pos = wordNext (wi)) != NULL)
    array (a,arrayMax (a),char*) = hlr_strdup (pos);
  wordIterDestroy (wi);
  return a;
}

Texta textStrtokP (char *s,char *sep) {
  /**
     Same as textStrtok() but does not alter its input 's'.<br>
     Suffix 'P' stands for 'Preserving its input'
  */
  char *cp = hlr_strdup (s);
  Texta t = textStrtok (cp,sep);
  hlr_free (cp);
  return t;
}

Texta textFieldtok (char *s,char *sep) {
  /**
     Splits string 's' in words separated by any of the characters
     from string 'sep'.<br>
     Note: the user of this routine is responsible for freeing
           the Array returned after use; (e.g. using textDestroy())
     @param[in] s - input string
     @param[in] sep - separation character(s)
     @param[out] s - the contents changed!
     @return an Array of pointers to C-strings
  */
  WordIter wi=wordIterCreate (s,sep,0);
  char *pos;

  Texta a = textCreate (10);
  while ((pos = wordNext (wi)) != NULL)
    array (a,arrayMax(a),char*) = hlr_strdup (pos);
  wordIterDestroy (wi);
  return a;
}

Texta textFieldtokP (char *s,char *sep) {
  /**
     Same as textFieldtok() but does not alter its input 's'.
     Suffix 'P' stands for 'Preserving its input'
  */
  char *cp = hlr_strdup (s);
  Texta t = textFieldtok (cp,sep);
  hlr_free (cp);
  return t;
}

void textJoin (Stringa s,char *sep,Texta t) {
  /**
     Joins strings in a Texta, optionally inserting a separator between elements<br>
     Example:<br>
     Texta t = textStrtok ("a b c"," ");<br>
     Stringa s = stringCreate (10);<br>
     textJoin (s,"-",t);<br>
     puts (string (s));<br>
     Prints "a-b-c"<br>
     @param[in] s - a Stringa, contents will be overridden
     @param[in] sep - separator between elements, e.g. ","
     @param[in] t - a Texta (can be empty, but not NULL)
     @param[out] s - filled; empty Stringa if empty Texta
  */
  int i;

  stringClear (s);
  if (arrayMax (t) == 0)
    return;
  stringCpy (s,arru (t,0,char *));
  for (i=1;i<arrayMax (t);i++) {
    stringCat (s,sep);
    stringCat (s,arru (t,i,char *));
  }
}

void textUniqKeepOrder (Texta t) {
  /**
     Remove duplicate strings from t without changing the order<br>
     Note: it must be ok to free elements from t<br>
     Execution time: O(n*n*log n) (where n is arrayMax(t)):<br>
     The implementation below has a lot of room for improvments, e.g.<br>
     - using a hashed lookup table for seen values<br>
       would reduce runtime complexity to O(n)<br>
     - if no hashed lookup table availabe:<br>
       s = arrayCopy(t)       -- O(n)<br>
       arraySort(s)           -- O(n * log n)<br>
       remove duplicates in s -- O(n)<br>
       f = arrayCopy(s)       -- O(n)<br>
       remove duplicates in t with lookup in s and flagging in f<br>
                               -- O(n * log n) <br>
       would trade space for time<br>
     @param[in] t - a Texta
     @param[out] t - duplicates removed, first occurences kept
  */
  Texta b = textCreate (arrayMax (t)); // index of elements seen
  int from = -1;
  int to = -1;
  char *cp;
  while (++from < arrayMax (t)) {
    cp = arru (t,from,char*);
    if (arrayFindInsert (b,&cp,NULL,(ARRAYORDERF)arrayStrcmp)) {
      to++; // new
      arru (t,to,char*) = cp;
    }
    else
      hlr_free (cp); // already present
  }
  arraySetMax(t,to + 1);
  arrayDestroy (b);
}

int textFind (Texta t,char *query) {
  /**
     Find first occurence of 'query' in Texta 't'
     @param[in] t - a Texta
     @param[in] query - a query string
     @return -1 if not found, else [0 .. arrayMax(t)-1]
  */
  int i;
  for (i=0;i<arrayMax (t);i++)
    if (strEqual (query,textItem (t,i)))
      return i;
  return -1;
}

/* --------------- part 4: line and word handling -------------- */

char *dbseqDissect (char *dbseq) {
  /**
     Dissects 'dbseq', a string of the form databasename:sequencename
     into its parts by replacing the ':' with '\0'<br>
     Note: the original string can be restored by:<br>
        char *seqname;<br>
        if (seqname = dbseqDissect (somestring))<br>
          *(seqname-1) = ':';<br>
     Note: colonDissect() handles also cases of the form classname:objectname<br>
           and does not alter its input, but has more overhead
  @param[in] dbseq - something like sw:dyr_ecoli
  @param[out] dbseq - only the database name left;
                      if dbseq had the right format, else unchanged
  @return NULL if dbseq had not the right format,
          otherwise pointer the beginning the sequencename
  */
  char *colonp;

  if ((colonp = strchr (dbseq,':')) == NULL)
    return NULL; // no ':' found
  if (colonp == dbseq)
    return NULL; // ':' is first char
  if (*(colonp+1) == '\0')
    return NULL; // ':' is last char
  *colonp = '\0';
  return colonp+1;
}

int colonDissect (char *word,Stringa s1,Stringa s2) {
  /**
     Dissects a word of the form s1:s2 into its parts
     s1 and s2; the form s1:"s2" is also allowed, in this
     case the surrounding double quotes are stripped.
     s1 and s2 must have at least one character each.
     @param[in] word - something like sw:dyr_ecoli
     @param[in] s1 - a Stringa to hold part before colon
     @param[in] s2 - a Stringa to hold part after colon
     @param[out] s1 - filled with part before colon
     @param[out] s2 - filled with part after colon
     @return 1, if word has from s1:s2 or s1:"s2";
             0 else (then s1 and s2 are undefined)
  */
  char *colonp;
  char *lastp;

  if (s1 == NULL || s2 == NULL)
    die ("colonDissect: s1 or s2 missing");
  if ((colonp = strchr (word,':')) == NULL)
    return 0;
  if (colonp == word)
    return 0;
  *colonp = '\0';
  stringCpy (s1,word);
  *colonp = ':'; // restore word
  lastp = word + strlen (word) - 1; // last character of word
  if (colonp == lastp)
    return 0; // first colon is last char
  if (*lastp == '"') {
    if (lastp - colonp < 3)
      return 0; // too close together
    if (*(colonp+1) != '"')
      return 0; // ??? xx:yy" ???
    *lastp = '\0';
    stringCpy (s2,colonp+2);
    *lastp = '"'; // restore word
    return 1;
  }
  stringCpy (s2,colonp+1);
  return 1;
}

char *strField (char *str,char sep) {
  /**
     Analogous to strtok(), but meant for extracting also empty strings
     between two separators.<br>
     Note: the input string str is destroyed<br>
     Usage:<br>
     char *cp;<br>
     cp = strField ("....",'x');<br>
     while (cp) {<br>
       ... treat possibly emtpy word cp ...<br>
       cp = strField (0,'x');<br>
     }<br>
     If possible, don't use this old routine, but the new
     wordIter given below.
  */
  static char *s; // remember across invocations
  static int atEnd;
  char *item;
  char *cp;
  if (str != NULL) { // initialize new scan
    s = str;
    atEnd = 0;
  }
  item = s;
  cp = strchr (s,sep); // find end of current item
  if (cp != NULL) {
    *cp = 0; // mark end
    s = cp+1; // advance for next invocation
  }
  else {
    if (atEnd)
      return 0;
    atEnd = 1;
  }
  return item;
}

/* ---------------------------------------------------- */
/// Initial size of buffer
#define GETLINE_MIN 80
/// Size increment of buffer
#define GETLINE_INC 1024

int getLine (FILE *stream,char **buffer,int *buflen) {
  /**
     Read an arbitrary long line from stream,
     functionality analog to gets(3S)<br>
     Postcondition: next call to getLine gives next line.<br>
     Usage example:<br>
     char *line = NULL;<br>
     int buflen;<br>
     int len;<br>
     while (getLine (stdin,&line,&buflen)) {<br>
       len = stripNlGetLength (line);<br>
       printf ("%d %s",len,line);<br>
     }<br>
     hlr_free (line);<br>
     Notes:<br>
     - 'buflen' is not the length of the returned line<br>
     - getLine() does not strip the trailing newline<br>
     - 'line' and 'buflen' belong tightly together<br>
     @param[in] stream - from where to read
     @param[in] buffer - pointer to a string
     @param[out] buffer - pointer to a string, might be re-allocated
     @param[out] buflen - current length of buffer, for interal use of getLine()
     @return number of chars put into buffer, including trailing \n
             (except for the last line);
             0 if EOF encountered
  */
  int buffree;
  char *startp;
  char *bufp;
  int c;

  if (buffer == NULL)
    die ("getLine() without buffer");
  if (*buffer == NULL) {
    *buflen = GETLINE_MIN;
    *buffer = (char *)hlr_malloc (*buflen);
  }
  buffree = (*buflen) -1;
  bufp = *buffer;
  startp = bufp;
  while ((c = getc(stream)) != EOF) {
    if (--buffree == 0) {
      *buflen = *buflen + GETLINE_INC;
      buffree += GETLINE_INC;
      *buffer = (char *)realloc (*buffer,*buflen);
      if (**buffer == '\0')
        die ("getLine: realloc");
      bufp = *buffer + (bufp - startp); // adjust to new location
      startp = *buffer;
    }
    if (c == 0)
      warn ("getLine: read a NULL character"); // warn on binary data
    *(bufp++) = c; // append char to buffer
    if (c == '\n')
      break;
  }
  *bufp = '\0';
  return bufp - startp;
}

int stripNlGetLength (char *line) {
  /**
     Strip the trailing new line character of line, if present
     and return the length of the resulting line;<br>
     This function is typically used in conjunction with getLine()
     from above.<br>
     Implementation note: since strlen() is a slow function if
     lines are long, I try to use it only when unavoidable.<br>
     Since stripNlGetLength() has to find the end of the string
     I pass its finding back, so programs using this function
     would not need to call strlen() again.
     @param[in] line - the line
     @param[out] line - the line without trailing \n
     @return length of output line
  */
  int len = strlen (line);
  if (len == 0)
    return 0;
  len--;
  if (line[len] == '\n')
    line[len] = '\0';
  else
    len++;
  return len;
}

/* ------------------ module word parser ------------ begin */
/*
dissects strings into words.
a word consists either of a sequence of
1. alphanumeric characters and whitespace, with double quotes
   around the word if it contains whitespace
2. a sequence of whitespace characters only, not occuring within
   double quotes
a double quote within a word must be escaped using '\'
'\\' is read as '\'
blank, \n, \t, ".", "," and ";" are considered whitespace.
use function isBlankStr() to check whether a word is pure whitespace.

test strings:
"a\"b" c
"a\"b" cc d e
*/

static int wordmax = 255;
static char *wordLinep = NULL;
static int inquote;
static int inword;
static int inescape;
static int wordPos;
static int wordLen;
static char *wordOut = NULL;
static char *wordOutBuf;
static char *wordOutp;
static char *wordDelims;
/// Escape to be used in a string
#define wordEscape '\\'

void wordSet (char *line,char *delims) {
  /**
     Initializes the module with the word to be dissected.<br>
     Postcondition: wordGet() can be called.
     @param[in] line - string to be parsed
     @param[in] delims - characters that act as word delimiters
  */
  wordLinep = line;
  wordDelims = delims;
  wordPos = 0; // current position in wordLinep
  wordLen = strlen (wordLinep);
  inquote = 0;
  inword = 0;
  inescape = 0;
  if (wordOut == NULL) {
    wordOut = (char *)hlr_malloc (wordmax);
    wordOutBuf = (char *)hlr_malloc (wordmax);
  }
  wordOutp = wordOut;
  *wordOutp = '\0'; // output word constructed so far
}

char *wordGet (void) {
  /**
     Returns the next word.<br>
     Precondition: call to wordSet() or wordGet()<br>
     Postcondition: next call to wordGet gives next word<br>
     Note: the memory for 'word' is managed by wordGet()
     @return word or NULL if end of input
  */
  int c;
  if (wordLinep == NULL)
    die ("wordGet: no line to get from; use wordSet() first");
  for (;;) {
    if (wordLen >= wordmax) {
      int outpos = wordOutp - wordOut;
      wordmax += 1024;
      wordOut = (char *)realloc (wordOut,wordmax);
      wordOutBuf = (char *)realloc (wordOutBuf,wordmax);
      if (wordOut == NULL || wordOutBuf == NULL)
        die ("wordGet:realloc: %s bytes",wordmax);
      wordOutp = wordOut + outpos;
    }
    if (wordPos >= wordLen) { // end of input
      if (inescape) {
        *(wordOutp++) = wordEscape;
        inescape = 0;
      }
      *wordOutp = '\0';
      strcpy (wordOutBuf,wordOut);
      wordOutp = wordOut;
      *wordOutp = '\0';
      if (wordOutBuf[0] == '\0') {
        wordLinep = NULL; // protect against calling again
        return 0;
      }
      return wordOutBuf;
    }
    c = *(wordLinep++);
    ++wordPos;
    if (c == '"') {
      if (inescape) {
        inescape = 0;
        *(wordOutp++) = wordEscape;
        *(wordOutp++) = '"';
        continue;
      }
      if (inquote) {
        *(wordOutp++) = '"';
        inquote = 0;
        continue;
      }
      if (inword) {
        *(wordOutp++) = '"';
        inquote = 1;
        continue;
      }
      inword = 1;
      inquote = 1;
      *wordOutp = '\0';
      strcpy (wordOutBuf,wordOut);
      wordOutp = wordOut;
      *(wordOutp++) = '"';
      if (strlen (wordOutBuf) > 0)
        return wordOutBuf;
      continue;
    }
    if (c == wordEscape) {
      if (inescape) {
         *(wordOutp++) = wordEscape;
         *(wordOutp++) = wordEscape;
         inescape = 0;
         continue;
      }
      inescape = 1;
      continue;
    }
    if (strchr (wordDelims,c) != NULL) { // c is a delimiter
      if (inquote || !inword) {
        *(wordOutp++) = c;
        continue;
      }
      *wordOutp = '\0';
      strcpy (wordOutBuf,wordOut);
      wordOutp = wordOut;
      *(wordOutp++) = c;
      inword = 0;
      if (strlen (wordOutBuf) > 0)
        return wordOutBuf;
      continue;
    }
    // not whitespace, not \, not " -- a non-special char
    if (!inword) {
      *wordOutp = '\0';
      strcpy (wordOutBuf,wordOut);
      wordOutp = wordOut;
      *(wordOutp++) = c;
      inword = 1;
      if (strlen (wordOutBuf) > 0)
        return wordOutBuf;
      continue;
    }
    *(wordOutp++) = c; // continue word
  } // 'endless' for loop
}

char *wordGetF (void) {
  /**
     Like wordGet() but skips words consisting entirely of whitespace
     @return the next word
  */
  char *cp;
  char *w;

  while ((w = wordGet ()) != NULL) {
    if (*w == '\0')
      die ("wordGetF got empty string");
    cp = w - 1;
    while (*++cp != '\0' && isspace (*cp));
    /* if string is only whitespace, we are now at the \0
       else we stopped on some non-whitespace */
    if (*cp != '\0')
      return w;
  }
  return NULL;
}

/* ------------------ module word parser ------------ end */

/* ------------------ module word parser 2 ---------- begin
analogous to strtok() from the standard C library,
but a bit more flexible and allows multiple independent
interleaved scans in different strings.

usage:
  WordIter wi = wordTokIterCreate("hallo welt", " \n\t");
  char *w;
  while (w = wordNext (wi))
    printf ("w='%s'\n",w);
  wordIterDestroy (wi);

#define wordTokIterCreate(s,seps) wordIterCreate(s,seps,1)
#define wordFldIterCreate(s,seps) wordIterCreate(s,seps,0)
#define wordIterDestroy(this) (wordIterDestroy_func(this), this=0)
*/

WordIter wordIterCreate (char *s,char *seps,int manySepsAreOne) {
  /**
     Create an iterator returning the words of 's', broken at any
     char from 'seps'. If 'manySepsAreOne' is 1, consequtive stretches
     of separators are treated as one separator (so there are no empty words).<br>
     Note: sep must be kept stable during the whole scan.<br>
           If 's' is the empty string, then<br>
           in mode manySepsAreOne==1, wordNext() will immediately return NULL,<br>
           in mode manySepsAreOne==0, wordNext() will return one empty string, then NULL
     @param[in] s - string to break
     @param[in] seps - set of word separator chars
     @param[in] manySepsAreOne - 1 or 0
     @param[out] s - is destroyed
     @return WordIter object for use in wordNext() and wordIterDestroy()
  */
  WordIter this1;
  if (s == NULL || seps == NULL || *seps == '\0')
    die ("wordIterCreate: some null/empty input");
  this1 = (WordIter)hlr_malloc (sizeof (struct wordIterStruct));
  this1->seps = seps;
  this1->cp = s;
  this1->manySepsAreOne = manySepsAreOne;
  this1->atEnd = 0;
  return this1;
}

void wordIterDestroy_func (WordIter this1) {
  /**
     Destroy the WordIter object.<br>
     Postcondition: 'this1' is no more accessible.<br>
     Do not call in your programs, use wordIterDestroy() instead.
     @param[in] this1 - created by wordIterCreate
  */
  if (this1 == NULL)
    die ("wordIterDestroy: null");
  hlr_free (this1);
}

char *wordNextG (WordIter this1,int *lenP) {
  /**
     @param[in] this1 - created by wordTokIterCreate() or wordFldIterCreate()
     @param[in] lenP - valid place to put an int; NULL if no interest
     @return NULL if no more word, else pointer to beginning
             of null-terminated word
     @param[out] *lenP - (if lenP not NULL): strlen() of result,
                 computed efficiently
  */
  char *cp;
  char *word;

  if (this1 == NULL)
    die ("wordNext: null");
  if (lenP != NULL)
    *lenP = 0;
  if (this1->atEnd)
    return NULL;
  cp = this1->cp;
  if (this1->manySepsAreOne) { // skip to first non-sep
    cp--;
    while (*++cp != '\0' && strchr (this1->seps,*cp) != NULL);
  }
  else {
    if (*cp == '\0') {
      this1->atEnd = 1;
      return cp;
    }
    if (strchr (this1->seps,*cp) != NULL) {
      this1->cp++;
      *cp = '\0';
      return cp;
    }
  }
  if (*cp == '\0') {
    this1->atEnd = 1;
    return NULL;
  }

  /* here holds: we are on the beginning of a word, on a non-separator char
     now run until end of this word */
  word = cp;
  cp--;
  while (*++cp != '\0' && strchr (this1->seps,*cp) == NULL);
  if (lenP != NULL)
    *lenP = cp - word;
  if (*cp == '\0')
    this1->atEnd = 1;
  else {
    *cp = '\0'; // mark end of word
    this1->cp = cp + 1; // next time we start after the end of this separator
  }
  return word;
}
