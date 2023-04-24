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
/** @file affyfileHandler.h
    @brief Knows how to parse several kinds of Affymetrix files.
    Module prefixes cfo_, lfo_, ifo_, dfo_, efo_
*/
#ifndef AFFYFILEHANDLER_H
#define AFFYFILEHANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "format.h"
#include "linestream.h"

/// old version of chip file
#define CHIPFILE_LIMS2 1
/// newer version of chip file
#define CHIPFILE_LIMS3 2

/// the chip file object
typedef struct _chipFileObject_ {
  Stringa parameters; //!< the parameters
  Texta headers; //!< the headers of the columns in the file
  Array rows; //!< of Row (see affyfileHandler.c)
  int chipfileType; //!< old or new file type
}*ChipFileObject;

extern ChipFileObject cfo_create (LineStream ls);
extern void cfo_destroy_func (ChipFileObject cfo);
/// call this macro, not cfo_destroy_func()
#define cfo_destroy(cfo) (cfo_destroy_func(cfo),cfo=NULL)
extern int cfo_getChipfileType (ChipFileObject cfo);
extern int cfo_isRelative (ChipFileObject cfo);
extern char *cfo_parametersGet (ChipFileObject cfo);
extern int cfo_sumAvgDiffGet (ChipFileObject cfo);
extern int cfo_sumSignalGet (ChipFileObject cfo);
extern int cfo_numColGet (ChipFileObject cfo);
extern int cfo_numRowGet (ChipFileObject cfo);
extern char *cfo_headerGet (ChipFileObject cfo,int col);
extern char *cfo_valueGet (ChipFileObject cfo,int row,char *col);
extern int cfo_checkColumn (ChipFileObject cfo,char *header);

/// the cell file object
typedef struct _cellFileObject_ {
  int maxX; //!< maximum x cordinate
  int maxY; //!< maximum y cordinate
  Array cel; //!< of Item (see affyfileHandler.c)
  Array header; //!< of Item (see affyfileHandler.c)
  Array intensity; //!< of Intensity (see affyfileHandler.c)
  Array masks; //!< of XY (see affyfileHandler.c)
  Array outliers; //!< of XY (see affyfileHandler.c)
  Array modified; //!< of XYm (see affyfileHandler.c)
}*CellFileObject;

extern CellFileObject lfo_create (LineStream ls);
extern void lfo_destroy_func (CellFileObject lfo);
/// call this macro, not lfo_destroy_func()
#define lfo_destroy(lfo) (lfo_destroy_func(lfo),lfo=NULL)
extern int lfo_numHeaderParamGet (CellFileObject lfo);
extern void lfo_headerParamGet (CellFileObject lfo,int index,
                                char **name,char **val);
extern char *lfo_valueGet (CellFileObject lfo,char *section,char *name);
extern void lfo_maxCoordGet (CellFileObject lfo,int *x,int *y);
extern int lfo_numIntensityGet (CellFileObject lfo);
extern int lfo_intensityXYGet (CellFileObject lfo,int x,int y,
                               float *mean,float *stddev,int *npix);
extern void lfo_intensityGet (CellFileObject lfo,int index,int *x,int *y,
                              float *mean,float *stddev,int *npix);
extern int lfo_isMasked (CellFileObject lfo,int x,int y);
extern int lfo_isOutlier (CellFileObject lfo,int x,int y);
extern int lfo_isModified (CellFileObject lfo,int x,int y,float *origmean);

/// the cif file object
typedef struct _cifFileObject_ {
  Array sections; //!< of SectionI (see affyfileHandler.c)
}*CifFileObject;

extern CifFileObject ifo_create (LineStream ls);
extern void ifo_destroy_func (CifFileObject ifo);
/// call this macro, not ifo_destroy_func()
#define ifo_destroy(ifo) (ifo_destroy_func(ifo),ifo=NULL)
extern char *ifo_valueGet (CifFileObject ifo,char *section,char *name);
extern int ifo_numItemGet (CifFileObject ifo,char *section);
extern int ifo_nameValueGet (CifFileObject ifo,char *section,int index,
                             char **name,char **value);

/// the cdf file object
typedef struct _cdfFileObject_ {
  Array sections; //!< of SectionD
}*CdfFileObject;

/// structure of a section in a cdf file
typedef struct _sectionD_ {
  char *sHeader; //!< the section header
  Array items; //!< of Item (see affyfileHandler.c)
  Array headers; //!< of char* (see affyfileHandler.c)
  Array rows; //!< of Row (see affyfileHandler.c)
}*SectionD;

extern CdfFileObject dfo_create (LineStream ls);
extern void dfo_destroy_func (CdfFileObject dfo);
/// call this macro, do not call dfo_destroy_func()
#define dfo_destroy(dfo) (dfo_destroy_func(dfo),dfo=NULL)
extern int dfo_numSectGet (CdfFileObject dfo);
extern SectionD dfo_sectionGetByIndex (CdfFileObject dfo,int index);
extern char *dfo_sectionNameGet (SectionD section);
extern char *dfo_sectionValueGet (SectionD section,char *name);
extern char *dfo_sectionColGet (SectionD section,int row,char *header);

/// structure of the experiment file object
typedef struct _expFileObject_ {
  Array sections; //!< of SectionE (see affyfileHandler.c)
}*ExpFileObject;

extern ExpFileObject efo_create (LineStream ls);
extern void efo_destroy_func (ExpFileObject efo);
/// call this macro, do not call efo_destroy_func()
#define efo_destroy(efo) (efo_destroy_func(efo),efo=NULL)
extern char *efo_valueGet (ExpFileObject efo,char *section,char *name);
extern int efo_numSectionParamGet (ExpFileObject efo,char *section);
extern int efo_sectionParamGet (ExpFileObject efo,char *section,int index,char **name,char **val);

#ifdef __cplusplus
}
#endif

#endif

