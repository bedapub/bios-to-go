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
/** @file chemutil.h
    @brief Chemistry tools: Roche numbers, mol/sd files, InChIs.
    Module prefix chem_
*/
#ifndef CHEMUTIL_H
#define CHEMUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "linestream.h"

extern char *chem_rono2html (char *rono);
extern char *chem_rono2url (char *rono);
extern char *chem_ronoNormalize (char *rono,int *parsedLength);
extern void chem_ronoChopBatch (char *rono);

extern void chem_sd_blockInit (LineStream ls);
extern char *chem_sd_block (void);
extern Texta chem_sd_getField (char *text,char *field);
extern Texta chem_sd_getMol (char *text);
extern void chem_sd_getNumAtomBond (Texta mol,int *numAtom,int *numBond);
extern void chem_sd_stripH (Texta mol);

extern void chem_sd_writeTagName (char *tag,char *extRegID,FILE *fp);
extern void chem_sd_writeValue_char (char* value,FILE *fp);
extern void chem_sd_writeValue_double (double value,FILE *fp);
extern void chem_sd_writeValue_int (int value,FILE *fp);
extern void chem_sd_writeTagEnd (FILE *fp);
extern void chem_sd_writeFileSeparator (FILE *fp);
extern void chem_sd_writeTagNameValue_char (char *tag,char *value,
                                            char *extRegID,FILE *fp);
extern void chem_sd_writeTagNameValue_double (char *tag,double value,
                                              char *extRegID,FILE *fp);
extern void chem_sd_writeTagNameValue_int (char *tag,int value,char *extRegID,
                                           FILE* fp);
extern char *chem_sd_extractMolfile (char *molfilePlusTags,char **molFile);
extern void chem_sd_tagBlockInit (LineStream ls);
extern char *chem_sd_tagBlock (void);
extern char *chem_sd_extractTag (char *tag,char **tagField);

/// formula layer
#define CHEM_INCHI_FORMULA  1
/// connectivity layer
#define CHEM_INCHI_CONN     2
/// hydrogen layer
#define CHEM_INCHI_H        4
/// charge layer
#define CHEM_INCHI_CHARGE   8
/// stereochemistry layer
#define CHEM_INCHI_STEREO  16
/// isotope layer
#define CHEM_INCHI_ISO     32
/// fixed hydrogen layer
#define CHEM_INCHI_FIXEDH  64

/// one layer of an InChI
typedef struct {
  char type; //!< one of CHEM_INCHI_FORMULA to CHEM_INCHI_FIXEDH
  char *all; //!< the complete layer (all components)
  Texta components; //!< each component separately
}ChemInchiLayer;

/// structure to contain a complete InChI split into layers and components
typedef struct _chemInchiSplit_ {
  char *id; //!< id of the compoud
  char *inchi; //!< the complete InChI
  Array layers; //!< of type ChemInchiLayer
  char *inchiNew; //!< reconstructed InChI
}*ChemInchiSplit;

extern ChemInchiSplit chem_inchi_split (char *id,char *inchi);
extern void chem_inchi_correctBigCounterIons (ChemInchiSplit this1);
extern void chem_inchi_calcInchi (ChemInchiSplit this1,int layers,
                                  int maxComponents);
extern char *chem_inchi_getInchi (ChemInchiSplit this1);
extern void chem_inchi_destroy_func (ChemInchiSplit this1);
/// use this macro instead of chem_inchi_destroy_func()
#define chem_inchi_destroy(this1) (chem_inchi_destroy_func(this1),this1=NULL)

#ifdef __cplusplus
}
#endif

#endif
