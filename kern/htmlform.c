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
/** @file htmlform.c
    @brief Modification of HTML forms.
    Module prefix htmlform_
*/

#include <ctype.h>
#include "log.h"
#include "htmlform.h"

static void htmlform_getTagName (char *tag,Stringa name) {
  char *cp = tag;

  while (*cp && !isspace ((int)*cp))
    cp++;
  stringNCpy (name,tag,cp - tag);
}

static char *htmlform_findAttrName (char *tag,char *attr) {
  /*
    return position of first char of attribute name 'attr' in HTML tag 'tag'
    (without <>)
  */
  char *cp = tag;
  char *end = NULL;

  int len = strlen (attr);
  // can we find the valid attribute name in the HTML tag at all?
  while ((cp = strCaseStr (cp,attr)) != NULL) {
    end = cp + len;
    if ((cp > tag) && // attribute must not be at beginning of tag
        isspace ((int)*(cp - 1)) && // AND attribute must be preceded by whitespace
        (isspace ((int)*end) || // AND attribute must be followed by whitespace ...
         (*end == '=') || // OR by '=' ...
         (*end == '\0'))) // OR by end of string
      break;
    else
      cp++;
  }
  return cp;
}

static char *htmlform_findAttrValue (char *tag,char *attr) {
  /*
    return position of first char of value of attribute 'attr' in HTML tag
    'tag' (maybe '"')
  */
  char *cp = htmlform_findAttrName (tag,attr);
  if (cp != NULL) {
    cp += strlen (attr);
    while (isspace ((int)*cp))
      cp ++; // skip ws
    // do we have an assignment, i.e. a non-empty attribute?
    if (*cp == '=') {
      cp++;
      while (isspace ((int)*cp))
        cp++;
    }
    else
      cp = NULL;
  }
  return cp;
}

static char *htmlform_getAttrValue (char *tag,char *attr,Stringa value) {
  /* return value of attribute 'attr' in HTML tag 'tag' (without <>) (name for
     empty attributes)
  */
  char *begin = NULL;
  char *end = NULL;

  int len = strlen (attr);
  stringClear (value);
  // do we have an assignment, i.e. a non-empty attribute?
  if ((begin = htmlform_findAttrValue (tag,attr)) != NULL) {
    // is our attribute value in double quotes?
    if (*begin == '"')
      end = strCopySubstr (begin,'"','"',value); // delimited by double quotes
    else {
      end = begin;
      while (*end && !isspace ((int)*end))
        end ++; // delimited by ws or end of string
      stringNCpy (value,begin,end - begin);
    }
  }
  // do we have an empty attribute e.g. CHECKED?
  else if ((begin = htmlform_findAttrName (tag,attr)) != NULL) {
    stringNCpy (value,begin,len);
    end = begin + len;
  }
  return end;
}

int htmlform_fieldFind (char *html,char *form,char *name,
                       char **begin,char ** end,
                        Stringa tagType,Stringa inputType) {
  /**
     Return position of first char of tag 'name' in HTML form 'form'
     (if any, else first match)
  */
  int result = HTMLFORM_INVALID_FORM;
  Stringa tag = stringCreate (256);
  Stringa tagname = stringCreate (16);
  Stringa val = stringCreate (256);
  int insideForm = 0;
  char *cp = html;

  // foreach tag in HTML document
  while ((cp = strCopySubstr (cp,'<','>',tag)) != NULL) {
    if (strStartsWithC (string (tag),"!")) // skip HTML comment tags
      continue;
    // is this the form we're looking for?
    htmlform_getTagName (string (tag),tagname);
    if (strCaseEqual (string (tagname),"FORM")) {
      if (form) {
        htmlform_getAttrValue (string (tag),"NAME",val);
        insideForm = strEqual (form,string (val));
      }
      else
        insideForm = 1;
      if (insideForm)
        result = HTMLFORM_INVALID_FIELD;
    }
    else if (insideForm && strCaseEqual (string (tag),"/FORM")) // end of form
      insideForm = 0;
    else if (insideForm) {
      htmlform_getTagName (string (tag),tagname);
      // is the current tag for data input, i.e. INPUT, TEXTAREA, or SELECT?
      if(strCaseEqual (string (tagname),"INPUT") ||
         strCaseEqual (string (tagname),"TEXTAREA") ||
         strCaseEqual (string (tagname),"SELECT")) {
        // ... and does the current tag's name attribute value match?
        htmlform_getAttrValue (string (tag),"NAME",val);
        if (strCaseEqual (string (val),name)) {
          if (begin != NULL)
            *begin = cp - stringLen (tag) - 2;
          if (end != NULL)
            *end = cp;
          if (tagType != NULL)
            stringCpy (tagType,string (tagname));
          if (inputType && strCaseEqual (string (tagname),"INPUT"))
            htmlform_getAttrValue (string (tag),"TYPE",inputType);
          result = HTMLFORM_OK;
          break;
        }
      }
    }
  }
  stringDestroy (val);
  stringDestroy (tagname);
  stringDestroy (tag);
  return result;
}

static char *multiSelectName = NULL;

int htmlform_valueSet (Stringa html,char *form,char *name,char *value) {
  /**
     Set default values in a HTML form. htmlform_valueSet() is currently
     implemented as a sequence of htmlform_valueClear() and
     htmlform_valueAdd(), see there for detailed documentation of their behavior
     @param[in] html - a character array containing the HTML form to be
                       modified; assumed to be __valid__ HTML, must at least
                       contain the form ... end form section; must not be
                       NULL; if empty no action is taken, i.e. html remains
                       unchanged
     @param[in] form - the name of the HTML form to apply changes to; if NULL
                       the first HTML form in html is modified; if non-NULL and
                       no case-insensitive name match within html no action is
                       taken, i.e. html remains unchanged
     @param[in] name - the name of the form field to be cleared; must not be
                       NULL; if no case-insensitive name match within html no
                       action is taken, i.e. html remains unchanged
     @param[in] value - depending on the form field type, the 'value' parameter
                        contains the value to be set as a default value
                        (TEXTAREA, INPUT types TEXT, FILE, HIDDEN, PASSWORD)
                        or a value that must be matched by a tag's VALUE
                        attribute to be switched on (SELECT, INPUT type
                        CHECKBOX, RADIO). the special value "ON" allows for
                        switching on INPUT type CHECKBOX regardless of their
                        VALUE attribute
     @param[out] html - contains the modified HTML form
     @return a status code indicating successful completion (HTMLFORM_OK),
             failure to find a or the specified form (HTMLFORM_INVALID_FORM),
             or failure to find a tag with the specified NAME attribute
             (HTMLFORM_INVALID_FIELD)
  */
  int result;

  if (multiSelectName && strEqual (name,multiSelectName))
    result = HTMLFORM_OK;
  else
    result = htmlform_valueClear (html,form,name);
  if (result == HTMLFORM_OK)
    result = htmlform_valueAdd (html,form,name,value);
  return result;
}

int htmlform_valueAdd (Stringa html,char *form,char *name,char *value) {
  /**
     Add default values to a HTML form. Currently the following form fields are
     handled: TEXTAREA, SELECT and INPUT types TEXT, FILE, HIDDEN, PASSWORD,
     CHECKBOX, and RADIO. The INPUT types BUTTON, IMAGE, RESET, and SUBMIT are
     not handled since we think they are not useful for setting default values
     in HTML forms. Adding to these form fields means: TEXTAREA: insert 'value'
     after any existing text just before the end TEXTAREA tag; SELECT: add a
     SELECTED attribute to the OPTION tag with a VALUE attribute matching
     parameter 'value' (!!) if not yet existing - text between OPTION and end
     OPTION is ignored (!!); INPUT types TEXT, FILE, HIDDEN, PASSWORD: append
     'value' to the existing VALUE tag if any and enclose the new value in
     double quotes in case it contains any whitespace. INPUT type CHECKBOX: add
     a CHECKED attribute to all tags with matching NAME and VALUE attributes if
     not yet existing; if 'value' has the special value "ON" a name match is
     sufficient; INPUT type RADIO: add a CHECKED attribute to all tags with
     matching NAME and VALUE attributes if not yet existing and remove it if
     NAME does match and VALUE doesn't; note that this will potentially switch
     on several radio buttons but only ill-designed forms have more than one
     radio button with identical NAME/VALUE attributes
     @param[in] html - a character array containing the HTML form to be
                       modified; assumed to be __valid__ HTML, must at least
                       contain the form ... end form section; must not be
                       NULL; if empty no action is taken, i.e. html remains
                       unchanged
     @param[in] form - the name of the HTML form to apply changes to; if NULL
                       the first HTML form in html is modified; if non-NULL and
                       no case-insensitive name match within html no action is
                       taken, i.e. html remains unchanged
     @param[in] name - the name of the form field to be cleared; must not be
                       NULL; if no case-insensitive name match within html no
                       action is taken, i.e. html remains unchanged
     @param[in] value - depending on the form field type, the 'value' parameter
                        contains the value to be set as a default value
                        (TEXTAREA, INPUT types TEXT, FILE, HIDDEN, PASSWORD) or
                        a value that must be matched by a tag's VALUE attribute
                        to be switched on (SELECT, INPUT type CHECKBOX, RADIO).
                        The special value "ON" allows for switching on INPUT
                        type CHECKBOX regardless of their VALUE attribute
     @param[out] html - contains the modified HTML form
     @return a status code indicating successful completion (HTMLFORM_OK),
             failure to find a or the specified form (HTMLFORM_INVALID_FORM),
             or failure to find a tag with the specified NAME attribute
             (HTMLFORM_INVALID_FIELD)
  */
  int result = HTMLFORM_OK;
  char *begin = NULL;
  char *end = NULL;
  Stringa tag = stringCreate (256);
  Stringa tagType = stringCreate (16);
  Stringa inputType = stringCreate (16);

  result = htmlform_fieldFind (string (html),form,name,&begin,&end,tagType,
                               inputType);
  if (result == HTMLFORM_OK) {
    strCopySubstr (begin,'<','>',tag);
    hlr_free (multiSelectName);
    // for "normal" INPUT fields...
    if (strCaseEqual (string (tagType),"INPUT")) {
      if (strCaseEqual (string (inputType),"TEXT") ||
         strCaseEqual (string (inputType),"HIDDEN") ||
         strCaseEqual (string (inputType),"PASSWORD") ||
         strCaseEqual (string (inputType),"FILE")) {
        char *cp = htmlform_findAttrValue (string (tag),"VALUE");
        char *cq = cp;
        if (cp != NULL) { // if there is a VALUE attribute at all...
          if (*cp == '"') { // and if it is enclosed in double quotes...
            if ((cq = strchr (cp + 1,'"')) != NULL) // then insert before closing double quote
              stringInsert (html,(begin-string (html)) + (cq-string (tag) + 1),
                            value);
            else // otherwise complain because double quotes __MUST__ match
              die ("htmlform_valueAdd(): cannot find closing quotes for VALUE attribute's value in '<%s>'. abort.",
                   begin);
          }
          else { // insert double quote before existing value, append new value and double quote
            while (*cq && !isspace ((int) *cq))
              cq ++;
            stringInsert (html,(begin-string (html)) + (cp-string (tag)) + 1,
                          "\"");
            stringInsert (html,(begin-string (html)) + (cq-string (tag) + 2),
                          "\"");
            stringInsert (html,(begin-string (html)) + (cq-string (tag) + 2),
                          value);
          }
        }
        else { // insert a new VALUE attribute
          Stringa buf = stringCreate (256);

          stringPrintf (buf," value=\"%s\"",value);
          stringInsert (html,end - string (html) - 1,string (buf));
          stringDestroy (buf);
        }
      }
      else if(strCaseEqual (string (inputType),"CHECKBOX")) {
        // for checkboxes, there may be many with identical NAME/VALUE attributes (!)
        Stringa nameValue = stringCreate (32);
        Stringa valueValue = stringCreate (32);
        char *myEnd = end;

        // if NAME/VALUE match and no CHECKED exists, insert it right at the end
        htmlform_getAttrValue (string (tag),"VALUE",valueValue);
        if (strCaseEqual (string (valueValue),value) ||
            strCaseEqual (value,"ON")) {
          if (!(htmlform_findAttrName (string (tag),"CHECKED"))) {
            stringInsert (html,myEnd - string (html) - 1," checked");
            myEnd += 8;
          }
        }
        // repeat until end of form as there may be many matching checkboxes...
        while ((myEnd = strCopySubstr (myEnd,'<','>',tag)) != NULL) {
          if (strCaseEqual (string (tag),"!"))
            continue;
          htmlform_getTagName (string (tag),tagType);
          if (strCaseEqual (string (tagType),"/FORM"))
            break;
          if (strCaseDiffer (string (tagType),"INPUT"))
            continue;
          htmlform_getAttrValue (string (tag),"TYPE",inputType);
          if(strCaseDiffer (string (inputType),"CHECKBOX"))
            continue;
          htmlform_getAttrValue (string (tag),"NAME",nameValue);
          if(strCaseDiffer (string (nameValue),name))
            continue;
          htmlform_getAttrValue (string (tag),"VALUE",valueValue);
          if (strCaseEqual (string (valueValue),value) ||
              strCaseEqual (value,"ON")) {
            if (!(htmlform_findAttrName (string (tag),"CHECKED"))) {
              stringInsert (html,myEnd - string (html) - 1," checked");
              myEnd += 8;
            }
          }
        }
        stringDestroy (valueValue);
        stringDestroy (nameValue);
      }
      else if (strCaseEqual (string (inputType),"RADIO")) {
        // for radio buttons, there may be many with identical NAME/VALUE attributes (!)
        Stringa nameValue = stringCreate (32);
        Stringa valueValue = stringCreate (32);
        char *myEnd = end;
        char *cp = NULL;

        // if NAME/VALUE match, insert a CHECKED right at the end if none exists...
        htmlform_getAttrValue (string (tag),"VALUE",valueValue);
        if (strCaseEqual (string (valueValue),value)) {
          if (!(cp = htmlform_findAttrName (string (tag),"CHECKED"))) {
            stringInsert (html,myEnd - string (html) - 1," checked");
            myEnd += 8;
          }
        }
        else { // otherwise remove existing CHECKED since only one value may be switched on
          if (cp) {
            stringCut (html,(myEnd - stringLen (tag) - 2 - string (html)) +
                       (cp - string (tag)),8);
            myEnd -= 8;
          }
        }
        // repeat until end of form to make sure all other matching radio buttons are off...
        while ((myEnd = strCopySubstr (myEnd,'<','>',tag)) != NULL) {
          if(strCaseEqual (string (tag),"!"))
            continue;
          htmlform_getTagName (string (tag),tagType);
          if (strCaseEqual (string (tagType),"/FORM"))
            break;
          if (strCaseDiffer (string (tagType),"INPUT"))
            continue;
          htmlform_getAttrValue (string (tag),"TYPE",inputType);
          if (strCaseDiffer (string (inputType),"RADIO"))
            continue;
          htmlform_getAttrValue (string (tag),"NAME",nameValue);
          if (strCaseDiffer (string (nameValue),name))
            continue;
          htmlform_getAttrValue (string (tag),"VALUE",valueValue);
          cp = htmlform_findAttrName (string (tag),"CHECKED");
          if (strCaseEqual (string (valueValue),value)) {
            if (!cp) {
              stringInsert (html,myEnd - string (html) - 1," checked");
              myEnd += 8;
            }
          }
          else {
            if (cp) {
              stringCut (html,(myEnd - stringLen (tag) - 2 - string (html)) +
                         (cp - string (tag)),8);
              myEnd -= 8;
            }
          }
        }
        stringDestroy (valueValue);
        stringDestroy (nameValue);
      }
    }
    else if (strCaseEqual (string (tagType),"TEXTAREA")) {
      char *cp = strCaseStr (end,"</TEXTAREA>");
      if (cp)
        stringInsert (html,cp - string (html),value);
      else
        die ("htmlform_valueAdd(): cannot find tag </TEXTAREA> in '%s'. abort.",
             begin);
    }
    else if (strCaseEqual (string (tagType),"SELECT")) {
      char *myEnd = end;
      int isMultiple = (htmlform_findAttrName (string (tag),"MULTIPLE") != NULL);
      Stringa valueValue = stringCreate (256);
      if (isMultiple)
        multiSelectName = hlr_strdup (name);
      while ((myEnd = strCopySubstr (myEnd,'<','>',tag)) != NULL) {
        if(strCaseEqual (string (tag),"!"))
          continue;
        htmlform_getTagName (string (tag),tagType);
        if (strCaseEqual (string (tagType),"/SELECT") ||
            strCaseEqual (string (tagType),"/FORM"))
          break;
        if (strCaseEqual (string (tagType),"OPTION")) {
          htmlform_getAttrValue (string (tag),"VALUE",valueValue);
          if (strCaseEqual (string (valueValue),value)) {
            char *cp = htmlform_findAttrName (string (tag),"SELECTED");
            if (!cp) {
              stringInsert (html,myEnd - string (html) - 1," selected");
              myEnd += 9;
            }
          }
          else if (!isMultiple) {
            char *cp = htmlform_findAttrName (string (tag),"SELECTED");
            if (cp) {
              stringCut (html,(myEnd - stringLen (tag) - 2 - string (html)) +
                         (cp - string (tag)),9);
              myEnd -= 9;
            }
          }
        }
      }
      stringDestroy (valueValue);
    }
  }
  stringDestroy (inputType);
  stringDestroy (tagType);
  stringDestroy (tag);
  return result;
}

int htmlform_valueClear (Stringa html,char *form,char *name) {
  /**
     Clear default values from a HTML form. Currently the following form fields
     are handled: TEXTAREA, SELECT and INPUT types TEXT, FILE, HIDDEN,
     PASSWORD, CHECKBOX, and RADIO. The INPUT types BUTTON, IMAGE, RESET, and
     SUBMIT are not handled since we think they are not useful for setting
     default values in HTML forms. Clearing these form fields means:
     TEXTAREA: remove all text between TEXTAREA and end TEXTAREA tags;
     SELECT: remove SELECTED attribute from all OPTION tags between SELECT
     and end SELECT tags; INPUT types TEXT, FILE, HIDDEN, PASSWORD: replace the
     value of an existing VALUE attribute by "", leave tags without a VALUE
     attribute unchanged, i.e. no empty value is added INPUT types CHECKBOX and
     RADIO: remove CHECKED attributes from all tags with matching name
     @param[in] html - a character array containing the HTML form to be
                       modified; assumed to be __valid__ HTML, must at least
                       contain the form ... end form section; must not be
                       NULL; if empty no action is taken, i.e. html remains
                       unchanged
     @param[in] form - the name of the HTML form to apply changes to; if NULL
                       the first HTML form in html is modified; if non-NULL and
                       no case-insensitive name match within html no action is
                       taken, i.e. html remains unchanged
     @param[in] name - the name of the form field to be cleared; must not be
                       NULL; if no case-insensitive name match within html no
                       action is taken, i.e. html remains unchanged
     @param[out] html - contains the modified HTML form
     @return a status code indicating successful completion (HTMLFORM_OK),
             failure to find a or the specified form (HTMLFORM_INVALID_FORM),
             or failure to find a tag with the specified NAME attribute
             (HTMLFORM_INVALID_FIELD)
  */
  int result = HTMLFORM_OK;
  char *begin = NULL;
  char *end = NULL;
  Stringa tag = stringCreate (256);
  Stringa tagType = stringCreate (16);
  Stringa inputType = stringCreate (16);

  result = htmlform_fieldFind (string (html),form,name,&begin,&end,tagType,
                               inputType);

  if (result == HTMLFORM_OK) {
    strCopySubstr (begin,'<','>',tag);
    if (strCaseEqual (string (tagType),"INPUT")) {
      if (strCaseEqual (string (inputType),"TEXT") ||
         strCaseEqual (string (inputType),"HIDDEN") ||
         strCaseEqual (string (inputType),"PASSWORD") ||
         strCaseEqual (string (inputType),"FILE")) {
        char *cp = htmlform_findAttrValue (string (tag),"VALUE");
        char *cq = cp;
        if (cp) {
          if (*cp == '"') {
            if ((cq = strchr (cp + 1,'"')) != NULL)
              stringCut (html,(begin - string (html)) + (cp - string (tag)) + 2,
                         cq - cp - 1);
            else
              die ("htmlform_valueClear(): cannot find closing quotes for VALUE attribute's value in '<%s>'. abort.",
                   begin);
          }
          else {
            while (*cq && !isspace ((int)*cq))
              cq ++;
            stringCut (html,(begin - string (html)) + (cp - string (tag)) + 1,
                       cq - cp);
            stringInsert (html,(begin-string (html)) + (cp-string (tag)) + 1,
                          "\"\"");
          }
        }
      }
      else if (strCaseEqual (string (inputType),"CHECKBOX")) {
        Stringa nameValue = stringCreate (32);
        char *myEnd = end;
        char *cp = htmlform_findAttrName (string (tag),"CHECKED");
        if (cp) {
          stringCut (html,(begin - string (html)) + (cp - string (tag)),8);
          myEnd -= 8;
        }
        while ((myEnd = strCopySubstr (myEnd,'<','>',tag)) != NULL) {
          if (strCaseEqual (string (tag),"!"))
            continue;
          htmlform_getTagName (string (tag),tagType);
          if (strCaseEqual (string (tagType),"/FORM"))
            break;
          if (strCaseDiffer (string (tagType),"INPUT"))
            continue;
          htmlform_getAttrValue (string (tag),"TYPE",inputType);
          if (strCaseDiffer (string (inputType),"CHECKBOX"))
            continue;
          htmlform_getAttrValue (string (tag),"NAME",nameValue);
          if (strCaseDiffer (string (nameValue),name))
            continue;
          if ((cp = htmlform_findAttrName (string (tag),"CHECKED")) != NULL) {
            stringCut (html,(myEnd - stringLen (tag) - 2 - string (html)) +
                       (cp - string (tag)),8);
            myEnd -= 8;
          }
        }
        stringDestroy (nameValue);
      }
      else if (strCaseEqual (string (inputType),"RADIO")) {
        Stringa nameValue = stringCreate (32);
        char *myEnd = end;
        char *cp = htmlform_findAttrName (string (tag),"CHECKED");
        if (cp) {
          stringCut (html,(begin - string (html)) + (cp - string (tag)),8);
          myEnd -= 8;
        }
        while ((myEnd = strCopySubstr (myEnd,'<','>',tag)) != NULL) {
          if (strCaseEqual (string (tag),"!"))
            continue;
          htmlform_getTagName (string (tag),tagType);
          if (strCaseEqual (string (tagType),"/FORM"))
            break;
          if (strCaseDiffer (string (tagType),"INPUT"))
            continue;
          htmlform_getAttrValue (string (tag),"TYPE",inputType);
          if (strCaseDiffer (string (inputType),"RADIO"))
            continue;
          htmlform_getAttrValue (string (tag),"NAME",nameValue);
          if (strCaseDiffer (string (nameValue),name))
            continue;
          if ((cp = htmlform_findAttrName (string (tag),"CHECKED")) != NULL) {
            stringCut (html,(myEnd - stringLen (tag) - 2 - string (html)) +
                       (cp - string (tag)),8);
            myEnd -= 8;
          }
        }
        stringDestroy (nameValue);
      }
    }
    else if (strCaseEqual (string (tagType),"TEXTAREA")) {
      char *cp = strCaseStr (end,"</TEXTAREA>");
      if (cp)
        stringCut (html,end - string (html),cp - end);
      else
        die ("htmlform_valueClear(): cannot find tag </TEXTAREA> in '%s'. abort.",
             begin);
    }
    else if (strCaseEqual (string (tagType),"SELECT")) {
      char *myEnd = end;

      while ((myEnd = strCopySubstr (myEnd,'<','>',tag)) != NULL) {
        if (strCaseEqual (string (tag),"!"))
          continue;
        htmlform_getTagName (string (tag),tagType);
        if (strCaseEqual (string (tagType),"/SELECT") ||
            strCaseEqual (string (tagType),"/FORM"))
          break;
        if (strCaseEqual (string (tagType),"OPTION")) {
          char *cp = htmlform_findAttrName (string (tag),"SELECTED");
          if (cp) {
            stringCut (html,(myEnd - stringLen (tag) - 2 - string (html)) +
                       (cp - string (tag)),9);
            myEnd -= 9;
          }
        }
      }
    }
  }
  stringDestroy (inputType);
  stringDestroy (tagType);
  stringDestroy (tag);
  return result;
}
