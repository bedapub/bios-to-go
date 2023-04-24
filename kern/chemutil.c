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
/** @file chemutil.c
    @brief Chemistry tools: Roche numbers, mol/sd files, InChIs.
    Module prefix chem_
*/
#include "log.h"
#include "format.h"
#include "hlrmisc.h"
#include "biurl.h"
#include "chemutil.h"

char *chem_rono2html (char *rono) {
  /**
     Construct html that shows a rono picture
     @param[in] rono - the Roche number
     @return html hyperlink text; memory managed by this routine
  */
  char *normRono;
  static Stringa s = NULL;
  stringCreateOnce (s,100);
  normRono = chem_ronoNormalize (rono,NULL);
  if (normRono == NULL)
    stringCpy (s,rono);
  else {
    // chem_ronoChopBatch (normRono);
    stringPrintf (s,biurl_build2Html ("ern",rono,
                                      stringPrintBuf ("show chemical structure of %s",rono)));
  }
  return string (s);
}

char *chem_rono2url (char *rono) {
  /**
     Construct URL that when clicked, shows a rono picture
     @param[in] rono - Roche number
     @return hyperlink; memory managed by this routine
  */
  char *normRono;
  static Stringa s = NULL;
  stringCreateOnce (s,100);
  normRono = chem_ronoNormalize (rono,NULL);
  if (normRono == NULL)
    stringCpy (s,rono);
  else {
    // chem_ronoChopBatch (normRono);
    stringPrintf (s,biurl_build2 ("ern",rono));
  }
  return string (s);
}

char *chem_ronoNormalize (char *rono,int *parsedLength) {
  /**
     check whether 'rono' looks like a roche number and if
     yes convert it to normalized format, e.g.
     'RO192-3710/000' --> 'RO1923710-000-000'
     'RO-92-3710'     --> 'RO0923710-000'
     'RO0657199-000-001' --> 'RO0657199-000-001'
     'RO0657199-000' --> 'RO0657199-000'
     @param[in] rono - Roche number
     @param[in] parsedLength - place for an integer, NULL if 'rono' must not
                               have trailing chars not part of a rono
     @param[out] parsedLength - length of result string, if requested
                                only valid if non-NULL was returned
     @return NULL if not a proper input rono
             else formatted rono; the memory returned is
             managed by this routine; it may be written to
             but not free()d or realloc()d by the user of this routine
  */
  int first,second,third;
  int n;
  int offset;
  static Stringa ronoRet = NULL;

  stringCreateOnce (ronoRet,20);
  offset = 0;
  if (strNCaseEqual (rono,"ro",2))
    offset = 2;
  if (*(rono+offset) == '-')
    offset++;
  if (sscanf (rono+offset,"%d-%d/%d%n",&first,&second,&third,&n) == 3) {
    // RO192-3710/000 or RO-92-3710/000
    if (first > 999 || second > 9999 || third > 999)
      return NULL;
    stringPrintf (ronoRet,"RO%07d-%03d-%03d",first*10000+second,third,0);
    n += offset;
  }
  else if (sscanf (rono+offset,"%d-%d-%d%n",&first,&second,&third,&n) == 3) {
    // RO1234567-123-123
    if (first > 9999999 || second > 999 || third > 999)
      return NULL;
    stringPrintf (ronoRet,"RO%07d-%03d-%03d",first,second,third);
    n += offset;
  }
  else if (sscanf (rono+offset,"%d-%d%n",&first,&second,&n) == 2) {
    // RO192-3710 or RO-92-3710
    if (first <= 999 && second <= 9999)
      stringPrintf (ronoRet,"RO%07d-%03d",first*10000+second,0);
    else if (first <= 9999999 && second <= 999)
      stringPrintf (ronoRet,"RO%07d-%03d",first,second);
    else
      return NULL;
    n += offset;
  }
  else
    return NULL;
  if (parsedLength != NULL)
    *parsedLength = n;
  else
    if (*(rono+n))
      return NULL; // don't allow leftovers
  return string (ronoRet);
}

void chem_ronoChopBatch (char *rono) {
  /**
     Chop off the trailing batch number
     from a Roche number, e.g.
     'RO0923710-000-000'  --> 'RO0923710-000'
     'RO0923710-000'  --> 'RO0923710-000'
     @param[in] rono - in normalized format RO0923710-000-000 or RO0923710-000
     @param[out] rono - without batch number
  */
  char *pos;
  int sl,i,n;

  if (rono == NULL)
    die ("chem_ronoChopBatch: null");
  sl = strlen (rono);
  n=0;
  for (i=0;i<sl;i++)
    if (rono[i] == '-')
      n++;
  if (n < 2)
    return;
  pos = strrchr (rono,'-');
  if (!pos)
    die ("chem_ronoChopBatch: not a complete Roche number: %s",rono);
  *pos = '\0';
}

/* ------------------------------------------------------------------- */

static LineStream g_lsM = NULL;
static int g_doneM = 0;

void chem_sd_blockInit (LineStream ls) {
  /**
     Intializes interator over an sd file.<br>
     Postcondition: chem_sd-block can be called
     @param[in] ls - a line stream over the sd file
  */
  g_lsM = ls;
  g_doneM = 0;
}

char *chem_sd_block (void) {
  /**
     Returns next block from the sd linestream.<br>
     Precondition: chem_sd_blockInit ()
     Postcondition: next call to chem_sd_block () will return the next block
     @return pointer to text, if block read or
             NULL if no more block could be read
             The memory for the text returned is managed by this
             routine; it may not be free'd or realloc'd by the user
             of this routine.
  */
  char *line;
  static Stringa text = NULL;

  stringCreateClear (text,2000);
  if (g_lsM == NULL)
    die ("use chem_sd_blockInit()");
  if (g_doneM)
    return NULL;
  while ((line = ls_nextLine (g_lsM)) != NULL) {
    if (strStartsWithC (line,"$$$$") && !isBlankStr (string (text)))
      return string (text);
    stringCat (text,line);
    stringCat (text,"\n");
  }
  g_doneM = 1;
  if (!isBlankStr (string (text)))
    return string (text);
  return NULL; // keep compiler happy
}

Texta chem_sd_getField (char *text,char *field) {
  /**
     Return the content of a given field.
     @param[in] text - the mol file as a buffer
     @param[in] field - name of the field
     @return a Texta containing the content of the field; memory belongs to this
     function
  */
  static Texta lines = NULL;
  char *t;
  LineStream ls;
  char *line;
  int found = 0;
  char *pos;

  if (lines == NULL)
    lines = textCreate (10);
  else
    textClear (lines);
  t = hlr_strdup (text);
  ls = ls_createFromBuffer (t);
  while ((line = ls_nextLine (ls)) != NULL) {
    if (!found) {
      if (!strStartsWithC (line,"> "))
        continue;
      pos = strchr (line,'<');
      if (!pos)
        continue;
      if (!strNEqual (pos+1,field,strlen (field)) &&
          !strNEqual (pos,field,strlen (field)))
        continue;
      found = 1;
    }
    else {
      if (line[0] == '\0')
        break;
      textAdd (lines,line);
    }
  }
  ls_destroy (ls);
  hlr_free (t);
  return lines;
}

Texta chem_sd_getMol (char *text) {
  /**
     Returns the molecule from a mol file without fields
     @param[in] text - the mol file as a buffer
     @return the molecule as a Texta; memory belongs to this function
  */
  static Texta lines = NULL;
  char *t;
  LineStream ls;
  char *line;

  if (lines == NULL)
    lines = textCreate (10);
  else
    textClear (lines);
  t = hlr_strdup (text);
  ls = ls_createFromBuffer (t);
  while ((line = ls_nextLine (ls)) != NULL) {
    textAdd (lines,line);
    if (strStartsWithC (line,"M  END"))
      break;
  }
  ls_destroy (ls);
  hlr_free (t);
  return lines;
}

void chem_sd_getNumAtomBond (Texta mol,int *numAtom,int *numBond) {
  /**
     Returns the number of atoms and bonds in a molecule.
     @param[in] mol - a Texta containing the molecule
     @param[in] numAtom,numBond - pointers to integers, NULL if not interested
     @param[out] numAtom,numBond - filled if input was not NULL
  */
  int na,nb;

  if (arrayMax (mol) < 4)
    na = nb = 0;
  else
    sscanf (textItem (mol,3),"%d %d",&na,&nb);
  if (numAtom)
    *numAtom = na;
  if (numBond)
    *numBond = nb;
}

void chem_sd_stripH (Texta mol) {
  /**
     Strips hydrgen atoms from a mol file
     @param[in] mol - the molecule as a Texta
  */
  int na,nb;
  Array ats;
  int a1,a2;
  unsigned int a1n, a2n; // unsigned char would also suffice, however using snprintf with %hhu meets complaints in Windows
  int i,k;
  int nbDel;
  char s[7];
  int n;

  chem_sd_getNumAtomBond (mol,&na,&nb);
  if (arrayMax (mol) < 4 + na + nb)
    return;
  ats = arrayCreate (10,int);
  for (i=4;i<4+na;i++) {
    if (textItem (mol,i)[31] == 'H' && textItem (mol,i)[32] == ' ') {
      array (ats,arrayMax (ats),int) = i-3;
      hlr_free (textItem (mol,i));
    }
  }
  nbDel = 0;
  for (i=4+na;i<4+na+nb;i++) {
    sscanf (textItem (mol,i),"%d %d",&a1,&a2);
    for (k=0;k<arrayMax (ats);k++)
      if (arru (ats,k,int) == a1 || arru (ats,k,int) == a2)
        break;
    if (k<arrayMax (ats)) {
      hlr_free (textItem (mol,i));
      nbDel++;
    }
    else {
      a1n = 0;
      for (k=4;k<4+a1;k++) {
        if (textItem (mol,k) == NULL)
          continue;
        a1n++;
      }
      a2n = 0;
      for (k=4;k<4+a2;k++) {
        if (textItem (mol,k) == NULL)
          continue;
        a2n++;
      }
      snprintf (s, 7, "%3u%3u", a1n, a2n);
      n = snprintf (textItem (mol,i), 6, "%s", s);
      if (n<0) {
        die("snprintf failed in chem_sd_stripH");
      }
    }
  }
  snprintf (s, 7, "%3d%3d",na-arrayMax (ats), nb-nbDel);
  n = snprintf (textItem (mol,3),6, "%s", s);
  if (n<0) {
    die("snprintf failed in chem_sd_stripH");
  }
  k = 0;
  for (i=0;i<arrayMax (mol);i++) {
    if (textItem (mol,i) == NULL)
      continue;
    arru (mol,k,char *) = arru (mol,i,char *);
    k++;
  }
  arraySetMax (mol,k);
  hlr_free (ats);
}

/* ------------------------------------------------------------------- */

static LineStream g_lsT = NULL;
static int g_doneT = 0;

void chem_sd_writeTagName (char *tag,char *extRegID,FILE *fp) {
  /**
     Writes the name of a tag to file
     @param[in] tag - the tag name
     @param[in] extRegID - ???; can be NULL
     @param[in] fp - the file pointer to write to
  */
  if (fp == NULL)
    return;
  if (tag != NULL)
    fprintf (fp,">  <%s>",tag);
  if (extRegID != NULL)
    fprintf (fp,"  (%s)",extRegID);
  fprintf (fp,"\n");
}

void chem_sd_writeValue_char (char *value,FILE *fp) {
  /**
     Write characters as value
     @param[in] value - the value
     @param[in] fp - the file pointer to write to
  */
  if (fp == NULL)
    return;
  if (value != NULL)
    fprintf (fp,"%s",value);
}

void chem_sd_writeValue_double (double value,FILE *fp) {
  /**
     Write a double as value
     @param[in] value - the value
     @param[in] fp - the file pointer to write to
  */
  if (fp == NULL)
    return;
  fprintf (fp,"%.3lf",value);
}

void chem_sd_writeValue_int (int value,FILE *fp) {
  /**
     Write an int as value
     @param[in] value - the value
     @param[in] fp - the file pointer to write to
  */
  if (fp == NULL)
    return;
  fprintf (fp,"%d",value);
}

void chem_sd_writeTagEnd (FILE *fp) {
  /**
     Finish off a tag
     @param[in] fp - the file pointer to write to
  */
  if (fp == NULL)
    return;
  fprintf (fp,"\n");
}

void chem_sd_writeFileSeparator (FILE *fp) {
  /**
     Write a separator between individual mol files
     @param[in] fp - the file pointer to write to
  */
  if (fp == NULL)
    return;
  fprintf (fp,"$$$$\n");
}

void chem_sd_writeTagNameValue_char (char *tag,char *value,char *extRegID,
                                     FILE *fp) {
  /**
     Write characters as a complete tag
     @param[in] tag - the tag name
     @param[in] value - the value
     @param[in] extRegID - ???; can be NULL
     @param[in] fp - the file pointer to write to
  */
  if (fp == NULL)
    return;
  chem_sd_writeTagName (tag,extRegID,fp);
  chem_sd_writeValue_char (value,fp);
  fprintf (fp,"\n");
  chem_sd_writeTagEnd (fp);
}

void chem_sd_writeTagNameValue_double (char *tag,double value,char *extRegID,
                                       FILE *fp) {
  /**
     Write a double as a complete tag
     @param[in] tag - the tag name
     @param[in] value - the value
     @param[in] extRegID - ???; can be NULL
     @param[in] fp - the file pointer to write to
  */
  if (fp == NULL)
    return;
  chem_sd_writeTagName (tag,extRegID,fp);
  chem_sd_writeValue_double (value,fp);
  fprintf (fp,"\n");
  chem_sd_writeTagEnd (fp);
}

void chem_sd_writeTagNameValue_int (char *tag,int value,char *extRegID,
                                    FILE* fp) {
  /**
     Write an int as a complete tag
     @param[in] tag - the tag name
     @param[in] value - the value
     @param[in] extRegID - ???; can be NULL
     @param[in] fp - the file pointer to write to
  */
  if (fp == NULL)
    return;
  chem_sd_writeTagName (tag,extRegID,fp);
  chem_sd_writeValue_int (value,fp);
  fprintf (fp,"\n");
  chem_sd_writeTagEnd (fp);
}

char *chem_sd_extractMolfile (char *molfilePlusTags,char **molFile) {
  /**
     From a complete mol file extract the molecule and tags separately
     @param[in] molfilePlusTags - the complete mol file
     @param[out] molFile - the molecule part
     @return the concatenated tags
  */
  static Stringa tags=NULL;
  static Stringa file=NULL;
  LineStream ls;
  char *line;

  stringCreateClear (tags,2000);
  stringCreateClear (file,2000);
  ls = ls_createFromBuffer (molfilePlusTags);
  while ((line = ls_nextLine (ls)) != NULL) {
    stringCat (tags,line);
    stringCat (tags,"\n");
    if (strStartsWithC (line,"M  END")) {
      stringCat (file,string (tags));
      *molFile = string (file);
      stringClear (tags);
    }
  }
  ls_destroy (ls);
  return string (tags);
}

void chem_sd_tagBlockInit (LineStream ls) {
  /**
     Start an iterator over tags in a mol file
     @param[in] ls - a line stram over the file
  */
  g_lsT = ls;
  g_doneT = 0;
}

char *chem_sd_tagBlock (void) {
  /**
     Returns the next complete tag
     @return the tag as a string; memory belongs to this function
  */
  char *line;
  static Stringa text=NULL;

  if (g_doneT)
    return NULL;
  stringCreateClear (text,2000);
  if (g_lsT == NULL)
    die ("use chem_sd_tagBlockInit()");
  while (1) {
    line = ls_nextLine (g_lsT);
    if (line == NULL || line[0] == '\0') {
      if (line == NULL)
        g_doneT = 1;
      return string (text);
    }
    else {
      stringCat (text,line);
      stringCat (text,"\n");
    }
  }
}

char *chem_sd_extractTag (char *tag,char **tagField) {
  /**
     Separates a tag as obtained from chem_sd_tagBlock() into the name and
     the value.
     @param[in] tag - the complete tag
     @param[out] tagField - the value of the field
     @return the name of the field
  */
  static Stringa text=NULL;
  static Stringa tagName=NULL;
  LineStream ls;
  char *line,*btag,*etag;

  stringCreateClear (text,2000);
  stringCreateClear (tagName,10);
  ls = ls_createFromBuffer (tag);
  while ((line = ls_nextLine (ls)) != NULL) {
    if (strStartsWithC (line,">  <")) {
      btag = line+4;
      if ((etag = strchr (btag,'>')) != NULL)
        *etag = '\0';
      stringCat (tagName,btag);
      continue;
    }
    stringCat (text,line);
    stringCat (text,"\n");
  }
  ls_destroy (ls);
  *tagField = string (text);
  return string (tagName);
}

/**********************************************************************/

ChemInchiSplit chem_inchi_split (char *id,char *inchi) {
  /**
     Splits an InChI into its layers.
     @param[in] id - name of the molecule
     @param[in] inchi - the complete InChI
     @return a split InChI object
  */
  ChemInchiSplit this1;
  Texta toks;
  int i;
  ChemInchiLayer *currLayer;

  this1 = (ChemInchiSplit)hlr_calloc (1,sizeof (struct _chemInchiSplit_));
  this1->id = hlr_strdup (id);
  this1->inchi = hlr_strdup (inchi);
  this1->layers = arrayCreate (10,ChemInchiLayer);
  toks = textStrtokP (inchi,"/");
  for (i=1;i<arrayMax (toks);i++) {
    currLayer = arrayp (this1->layers,arrayMax (this1->layers),ChemInchiLayer);
    if (strchr ("chpqbtmsif",textItem (toks,i)[0])) {
      currLayer->type = textItem (toks,i)[0];
      currLayer->all = hlr_strdup (textItem (toks,i)+1);
      currLayer->components = textFieldtokP (currLayer->all,";");
    }
    else { // formula
      currLayer->type = ' ';
      currLayer->all = hlr_strdup (textItem (toks,i));
      currLayer->components = textFieldtokP (currLayer->all,".");
    }
  }
  textDestroy (toks);
  return this1;
}

void chem_inchi_correctBigCounterIons (ChemInchiSplit this1) {
  /**
     Knows some common large counter ions, that may appear as first component,
     eventhough they should just be the salt. Reverses the first and second
     components
     @param[in] this1 - a split InChI object
  */
  ChemInchiLayer *layerC = malloc(sizeof(ChemInchiLayer));
  ChemInchiLayer *currLayer = malloc(sizeof(ChemInchiLayer));
  int i;

  for (i=0;i<arrayMax (this1->layers);i++) {
    layerC = arrp (this1->layers,i,ChemInchiLayer);
    if (layerC->type == 'c')
      break;
  }
  if (i==arrayMax (this1->layers))
    return;
  // some common counter ions that are larger than the real compound
  if (strEqual (textItem (layerC->components,0),
                "1-11-3-7-13(8-4-11)19(25)27-15(17(21)22)16(18(23)24)28-20(26)14-9-5-12(2)6-10-14") ||
      strEqual (textItem (layerC->components,0),
                "1-6-2-4-7(5-3-6)11(8,9)10") ||
      strEqual (textItem (layerC->components,0),
                "1-3-13-12-22-9-7-14(13)10-19(22)20(23)16-6-8-21-18-5-4-15(24-2)11-17(16)18") ||
      strEqual (textItem (layerC->components,0),
                "43-1-8-29-15(50)22(57)36(64-8)72-30-9(2-44)66-38(24(59)17(30)52)74-32-11(4-46)68-40(26(61)19(32)54)76-34-13(6-48)70-42(28(63)21(34)56)77-35-14(7-49)69-41(27(62)20(35)55)75-33-12(5-47)67-39(25(60)18(33)53)73-31-10(3-45)65-37(71-29)23(58)16(31)51") ||
      strEqual (textItem (layerC->components,0),
                "19-15(20)13(25-17(23)11-7-3-1-4-8-11)14(16(21)22)26-18(24)12-9-5-2-6-10-12")) {
    // exchange first and second components
    int i;
    char *tmp;

    for (i=0;i<arrayMax (this1->layers);i++) {
      currLayer = arrp (this1->layers,i,ChemInchiLayer);
      if (currLayer->components != NULL &&
          arrayMax (currLayer->components) > 1) {
        tmp = arru (currLayer->components,0,char *);
        arru (currLayer->components,0,char *) =
          arru (currLayer->components,1,char *);
        arru (currLayer->components,1,char *) = tmp;
      }
    }
  }
}

void chem_inchi_calcInchi (ChemInchiSplit this1,int layers,int maxComponents) {
  /**
     Reconstructs an InChI from the pieces but adds only the requested layers
     and not more than a given number of coponents (for example no counter ion)
     @param[in] this1 - a split InChI object
     @param[in] layers - a bitmap of requested layers
     @param[in] maxComponents - maximum number of components to add to new
                InChI, 0=all
  */
  static Stringa inchi = NULL;
  int i,i1;
  ChemInchiLayer *currLayer;
  int numComponents;

  if (maxComponents == 0)
    maxComponents = 1000;
  stringCreateOnce (inchi,100);
  stringCpy (inchi,"InChI=1/");
  for (i=0;i<arrayMax (this1->layers);i++) {
    currLayer = arrp (this1->layers,i,ChemInchiLayer);
    if (currLayer->type == ' ' && (layers & CHEM_INCHI_FORMULA) == 0)
      continue;
    else if (currLayer->type == 'c' && (layers & CHEM_INCHI_CONN) == 0)
      continue;
    else if (currLayer->type == 'h' && (layers & CHEM_INCHI_H) == 0)
      continue;
    else if (strchr ("pq",currLayer->type) && (layers & CHEM_INCHI_CHARGE) == 0)
      continue;
    else if (strchr ("bmst",currLayer->type) && (layers & CHEM_INCHI_STEREO) == 0)
      continue;
    else if (currLayer->type == 'i' && (layers & CHEM_INCHI_ISO) == 0)
      continue;
    else if (currLayer->type == 'f' && (layers & CHEM_INCHI_FIXEDH) == 0)
      continue;
    if (currLayer->components == NULL)
      continue;
    if (currLayer->type != ' ')
      stringAppendf (inchi,"/%c",currLayer->type);
    numComponents = MIN (arrayMax (currLayer->components),maxComponents);
    for (i1=0;i1<numComponents;i1++) {
      if (currLayer->type == ' ')
        stringAppendf (inchi,"%s%s",textItem (currLayer->components,i1),
                       i1 < numComponents-1 ? "." : "");
      else
        stringAppendf (inchi,"%s%s",textItem (currLayer->components,i1),
                       i1 < numComponents-1 ? ";" : "");
    }
  }
  this1->inchiNew = hlr_strdup (string (inchi));
}

char *chem_inchi_getInchi (ChemInchiSplit this1) {
  /**
     Retrieves the reconstructed InChI
     @param[in] this1 - a split InChI object
     @return the reconstructed InChI as a string; memory belongs to this
     function
  */
  if (this1 == NULL)
    die ("chem_inchi_getInchi: no ChemInchiSplit");
  if (this1->inchiNew == NULL)
    die ("chem_inchi_getInchi: no InChI calculated");
  return this1->inchiNew;
}

void chem_inchi_destroy_func (ChemInchiSplit this1) {
  /**
     Destroys a split InChI object.<br>
     Note: do not use this function but the corresponding macro
     @param[in] this1 - a split InChI object
  */
  int i;
  ChemInchiLayer *currLayer;

  if (this1 == NULL)
    return;
  hlr_free (this1->id);
  hlr_free (this1->inchi);
  for (i=0;i<arrayMax (this1->layers);i++) {
    currLayer = arrp (this1->layers,i,ChemInchiLayer);
    hlr_free (currLayer->all);
    textDestroy (currLayer->components);
  }
  arrayDestroy (this1->layers);
  hlr_free (this1->inchiNew);
  hlr_free (this1);
}
