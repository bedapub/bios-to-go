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
/** @file affyfileHandler.c
    @brief Knows how to parse several kinds of Affymetrix files.
    Module prefixes cfo_, lfo_, ifo_, dfo_, efo_
*/
#include <ctype.h>
#include "log.h"
#include "hlrmisc.h"
#include "affyfileHandler.h"

/// structure of a row
typedef struct {
  Texta cols; //!< values in each column of the row
}Row;

ChipFileObject cfo_create (LineStream ls) {
  /**
     Creates new chip file object by reading LineStream ls.<br>
     Use cfo_destroy() the dispose of the ChipFileObject
     @param[in] ls - the line stream
     @return ChipFileObject or NULL if contents of ls did not have proper format
  */
  ChipFileObject cfo;
  WordIter wIter;
  char *line;
  char *word;
  Row *currRow;
  int i;

  cfo = (ChipFileObject)hlr_malloc (sizeof (struct _chipFileObject_));
  cfo->parameters = stringCreate (1000);
  cfo->headers = textCreate (10);
  cfo->rows = arrayCreate (100,Row);
  // look for headers
  while ((line = ls_nextLine (ls)) != NULL) {
    if ((strstr (line,"Avg Diff\t") || strstr (line,"Signal\t")) &&
        strTranslate (line,"\t","\t") > 3)
      break;
    stringCat (cfo->parameters,line);
    stringCat (cfo->parameters,"\n");
  }
  if (line == NULL) {
    warn ("cfo_create: no header found: was looking for 'Avg Diff' or 'Signal' and at least 4 columns");
    cfo_destroy (cfo);
    return NULL;
  }
  wIter = wordIterCreate (line,"\t\r\n",0);
  while ((word = wordNext (wIter)) != NULL) {
    // 'Gene Name' and 'Probe Set' were older names for 'Probe Set Name'
    if (strEqual (word,"Gene Name") || strEqual (word,"Probe Set Name"))
      word = "Probe Set Name";
    // 'Experiment Name' was an older name for 'Analysis Name'
    if (strEqual (word,"Experiment Name"))
      word = "Analysis Name";
    textAdd (cfo->headers,word);
  }
  wordIterDestroy (wIter);
  while ((line = ls_nextLine (ls)) != NULL) {
    if (line[0] == '\0')
      continue;
    currRow = arrayp (cfo->rows,arrayMax (cfo->rows),Row);
    currRow->cols = textCreate (10);
    wIter = wordIterCreate (line,"\t\r\n",0);
    while ((word = wordNext (wIter)) != NULL)
      textAdd (currRow->cols,word);
    wordIterDestroy (wIter);
  }
  cfo->chipfileType = CHIPFILE_LIMS2;
  for (i=0;i<arrayMax (cfo->headers);i++)
    if (strEqual (textItem (cfo->headers,i),"Signal"))
      break;
  if (i < arrayMax (cfo->headers)) {
    currRow = arrp (cfo->rows,0,Row);
    if (textItem (currRow->cols,i)[0] != '\0')
      cfo->chipfileType = CHIPFILE_LIMS3;
  }
  return cfo;
}

int cfo_getChipfileType (ChipFileObject cfo) {
  /**
     Returns the type of the chip file
     @param[in] cfo - the ChipFile object
     @return CHIPFILE_LIMS2 or CHIPFILE_LIMS3
  */
  return cfo->chipfileType;
}

void cfo_destroy_func (ChipFileObject cfo) {
  /**
     Destroys the chipfile object passed as argument.
     do not call in your programs -- use cfo_destroy () instead
     @param[in] cfo - the ChipFile object
  */
  Row *currRow;
  int i;

  if (cfo == NULL)
    die ("cfo_destroy_func: no chipfile object");
  stringDestroy (cfo->parameters);
  textDestroy (cfo->headers);
  for (i=0;i<arrayMax (cfo->rows);i++) {
    currRow = arrp (cfo->rows,i,Row);
    textDestroy (currRow->cols);
  }
  arrayDestroy (cfo->rows);
  hlr_free (cfo);
}

static int cfoGetIndexOfColumn (ChipFileObject cfo,char *header)
{
  /**
     Get the index of the column with the given header
     @param[in] cfo - the ChipFile object
     @param[in] header - the header of the column of interest
     @return index of column; dies if column does not exist
  */
  int k;

  for (k=0;k<arrayMax (cfo->headers);k++)
    if (strEqual (textItem (cfo->headers,k),header))
      return k;
  die ("cfoGetIndexOfColumn: Column %s not found in chip file",header);
  return -1; // keep compiler happy
}

int cfo_sumAvgDiffGet (ChipFileObject cfo) {
  /**
     @param[in] cfo - the ChipFile object
     @return the sum of average differences
  */
  int i,k;
  int sum;
  Row *currRow;

  if (cfo == NULL)
    die ("cfo_getSumAvgDiff: no chipfile object");
  k = cfoGetIndexOfColumn (cfo,"Avg Diff");
  sum = 0;
  for (i=0;i<arrayMax (cfo->rows);i++) {
    currRow = arrp (cfo->rows,i,Row);
    sum += atoi (textItem (currRow->cols,k));
  }
  return sum;
}

int cfo_sumSignalGet (ChipFileObject cfo) {
  /**
     @param[in] cfo - the ChipFile object
     @return the number of signals in the file
  */
  int i,k;
  int sum;
  Row *currRow;

  if (cfo == NULL)
    die ("cfo_sumSignalGet: no chipfile object");
  k = cfoGetIndexOfColumn (cfo,"Signal");
  sum = 0;
  for (i=0;i<arrayMax (cfo->rows);i++) {
    currRow = arrp (cfo->rows,i,Row);
    sum += atoi (textItem (currRow->cols,k));
  }
  return sum;
}

char *cfo_parametersGet (ChipFileObject cfo) {
  /**
     @param[in] cfo - the ChipFile object
     @return pointer to parameter text of cfo, NULL if cfo has no
             parameter header; the memory returned is managed by this routine
             and is read-only to the user of this routine; it stays stable
             until cfo_destroy()
  */
  if (cfo == NULL)
    die ("cfo_getParameters: no chipfile object");
  if (string (cfo->parameters)[0] == '\0')
    return NULL;
  return string (cfo->parameters);
}

int cfo_isRelative (ChipFileObject cfo) {
  /**
     @param[in] cfo - the ChipFile object
     @return whether the file contains signals (0) or ratios (1)
  */
  int i;

  for (i=0;i<arrayMax (cfo->headers);i++)
    if (strEqual (textItem (cfo->headers,i),"Inc Ratio"))
      return 1;
  return 0;
}

int cfo_numColGet (ChipFileObject cfo) {
  /**
     @param[in] cfo - the ChipFile object
     @return number of columns in the file
  */
  return arrayMax (cfo->headers);
}

int cfo_numRowGet (ChipFileObject cfo) {
  /**
     @param[in] cfo - the ChipFile object
     @return number of rows in the file
  */
  return arrayMax (cfo->rows);
}

char *cfo_headerGet (ChipFileObject cfo,int col) {
  /**
     Returns header of column 'col' (col starting at 0)
     @param[in] cfo - the ChipFile object
     @param[in] col - the column number
     @return header of col th column
  */
  if (col >= arrayMax (cfo->headers))
    die ("cfo_headerGet: Fewer columns than requested column number");
  return (textItem (cfo->headers,col));
}

char *cfo_valueGet (ChipFileObject cfo,int row,char *col) {
  /**
     Returns the value at row 'row' and of column 'col' (both starting at 0)
     @param[in] cfo - the ChipFile object
     @param[in] row - the row number
     @param[in] col - the column number
     @return the value
  */
  Row *currRow;
  int k;

  if (row >= arrayMax (cfo->rows))
    die ("cfo_valueGet: Fewer rows than requested row number");
  currRow = arrp (cfo->rows,row,Row);
  k = cfoGetIndexOfColumn (cfo,col);
  return (textItem (currRow->cols,k));
}

int cfo_checkColumn (ChipFileObject cfo,char *header) {
  /**
     Check whether a certain column exists
     @param[in] cfo - the ChipFile object
     @param[in] header - the header of the column of interest
     @return 1 if found, 0 if not
  */
  int k;

  for (k=0;k<arrayMax (cfo->headers);k++)
    if (strEqual (textItem (cfo->headers,k),header))
      return 1;
  return 0;
}

/************************************************************************/

/// an intensity
typedef struct {
  int x; //!< x coordinate
  int y; //!< y coordinate
  float mean; //!< mean intensity
  float stddev; //!< std deviation
  int npix; //!< number of pinels in cell
}Intensity;

/// coordinates
typedef struct {
  int x; //!< x coordinate
  int y; //!< y coordinate
}XY;

/// coordinates with original mean
typedef struct {
  int x; //!< x coordinate
  int y; //!< y coordinate
  float origmean; //!< original mean
}XYm;

/// structure of an Item
typedef struct {
  char *name; //!< name
  char *value; //!< value
}Item;

CellFileObject lfo_create (LineStream ls) {
  /**
     Create new cell file object by reading LineStream ls;<br>
     Use lfo_destroy() the dispose of the CellFileObject
     @return CellFileObject or NULL if contents of ls did not
     have proper format
  */
  CellFileObject lfo;
  char *line;
  char section[100];
  char *pos;
  Item *currItem;

  lfo = (CellFileObject)hlr_malloc (sizeof (struct _cellFileObject_));
  lfo->cel = arrayCreate (2,Item);
  lfo->header = arrayCreate (10,Item);
  lfo->intensity = arrayCreate (100,Intensity);
  lfo->masks = arrayCreate (100,XY);
  lfo->outliers = arrayCreate (100,XY);
  lfo->modified = arrayCreate (100,XYm);
  lfo->maxX = lfo->maxY = 0;

  while ((line = ls_nextLine (ls)) != NULL) {
    if (line[0] == '\0')
      continue;
    if (line[0] == '[') {
      char *pos;

      pos = strchr (line+1,']');
      if (!pos)
        die ("lfo_create: section id does not end with ]");
      *pos = '\0';
      strcpy (section,line+1);
    }
    else
      continue;
    if (strEqual (section,"CEL")) {
      while ((line = ls_nextLine (ls)) != NULL) {
        if (line[0] == '\0')
          break;
        pos = strchr (line,'=');
        if (!pos) {
          warn ("lfo_create: = expected between name and value in %s",line);
          lfo_destroy (lfo);
          return NULL;
        }
        *pos = '\0';
        currItem = arrayp (lfo->cel,arrayMax (lfo->cel),Item);
        currItem->name = hlr_strdup (line);
        currItem->value = hlr_strdup (pos+1);
      }
    }
    else if (strEqual (section,"HEADER")) {
      while ((line = ls_nextLine (ls)) != NULL) {
        if (line[0] == '\0')
          break;
        pos = strchr (line,'=');
        if (!pos) {
          warn ("lfo_create: = expected between name and value in %s",line);
          lfo_destroy (lfo);
          return NULL;
        }
        *pos = '\0';
        currItem = arrayp (lfo->header,arrayMax (lfo->header),Item);
        currItem->name = hlr_strdup (line);
        currItem->value = hlr_strdup (pos+1);
      }
    }
    else if (strEqual (section,"INTENSITY")) {
      int numCell;
      Intensity *currInt;

      line = ls_nextLine (ls);
      sscanf (line,"NumberCells=%d",&numCell);
      line = ls_nextLine (ls); // header
      while ((line = ls_nextLine (ls)) != NULL) {
        if (line[0] == '\0')
          break;
        currInt = arrayp (lfo->intensity,arrayMax (lfo->intensity),Intensity);
        if (sscanf (line,"%d %d %f %f %d",&currInt->x,&currInt->y,
                    &currInt->mean,&currInt->stddev,&currInt->npix) != 5)
          die ("lfo_create: Format error in INTENSITY section, line %s",line);
        if (currInt->x > lfo->maxX)
          lfo->maxX = currInt->x;
        if (currInt->y > lfo->maxY)
          lfo->maxY = currInt->y;
      }
      if (arrayMax (lfo->intensity) != numCell) {
        warn ("lfo_create: Error in INTENSITY section of cell file");
        lfo_destroy (lfo);
        return NULL;
      }
    }
    else if (strEqual (section,"MASKS")) {
      int numCell;
      XY *currXY;

      line = ls_nextLine (ls);
      sscanf (line,"NumberCells=%d",&numCell);
      line = ls_nextLine (ls); // header
      while ((line = ls_nextLine (ls)) != NULL) {
        if (line[0] == '\0')
          break;
        currXY = arrayp (lfo->masks,arrayMax (lfo->masks),XY);
        sscanf (line,"%d %d",&currXY->x,&currXY->y);
      }
      if (arrayMax (lfo->masks) != numCell) {
        warn ("lfo_create: Error in MASKS section of cell file");
        lfo_destroy (lfo);
        return NULL;
      }
    }
    else if (strEqual (section,"OUTLIERS")) {
      int numCell;
      XY *currXY;

      line = ls_nextLine (ls);
      sscanf (line,"NumberCells=%d",&numCell);
      line = ls_nextLine (ls); // header
      while ((line = ls_nextLine (ls)) != NULL) {
        if (line[0] == '\0')
          break;
        currXY = arrayp (lfo->outliers,arrayMax (lfo->outliers),XY);
        sscanf (line,"%d %d",&currXY->x,&currXY->y);
      }
      if (arrayMax (lfo->outliers) != numCell) {
        warn ("lfo_create: Error in OUTLIERS section of cell file");
        lfo_destroy (lfo);
        return NULL;
      }
    }
    else if (strEqual (section,"MODIFIED")) {
      int numCell;
      XYm *currXYm;

      line = ls_nextLine (ls);
      sscanf (line,"NumberCells=%d",&numCell);
      line = ls_nextLine (ls); // header
      while ((line = ls_nextLine (ls)) != NULL) {
        if (line[0] == '\0')
          break;
        currXYm = arrayp (lfo->modified,arrayMax (lfo->modified),XYm);
        sscanf (line,"%d %d %f",&currXYm->x,&currXYm->y,&currXYm->origmean);
      }
      if (arrayMax (lfo->modified) != numCell) {
        warn ("lfo_create: Error in MODIFIED section of cell file");
        lfo_destroy (lfo);
        return NULL;
      }
    }
    if (line == NULL)
      break;
  }
  return lfo;
}

void lfo_destroy_func (CellFileObject lfo) {
  /**
     Destroys the cellfile object passed as argument.
     do not call in your programs -- use lfo_destroy () instead
     @param[in] lfo - the CellFile object
  */
  if (lfo == NULL)
    die ("lfo_destroy_func: no cellfile object");
  arrayDestroy (lfo->cel);
  arrayDestroy (lfo->header);
  arrayDestroy (lfo->intensity);
  arrayDestroy (lfo->masks);
  arrayDestroy (lfo->outliers);
  arrayDestroy (lfo->modified);
  hlr_free (lfo);
}

int lfo_numHeaderParamGet (CellFileObject lfo) {
  /**
     @param[in] lfo - the CellFile object
     @return number of parameters in the header
  */
  if (lfo == NULL)
    die ("lfo_numHeaderParamGet: no cellfile object");
  if (lfo->header == NULL)
    return 0;
  return arrayMax (lfo->header);
}

void lfo_headerParamGet (CellFileObject lfo,int index,char **name,char **val) {
  /**
     Get one parameter
     @param[in] lfo - the CellFile object
     @param[in] index - index of the parameter
     @param[in] name - pointer where the name can be returned
     @param[in] val - pointer where the value can be returned
     @param[out] name - the name of the parameter
     @param[out] val - the value of the parameter
  */
  Item *currItem;

  if (lfo == NULL)
    die ("lfo_headerParamGet: no cellfile object");
  if (index < 0 || index >= arrayMax (lfo->header))
    die ("lfo_headerParamGet: no such index");
  currItem = arrp (lfo->header,index,Item);
  *name = currItem->name;
  *val = currItem->value;
}

char *lfo_valueGet (CellFileObject lfo,char *section,char *name) {
  /**
     Get a value from the cell file
     @param[in] lfo - the CellFile object
     @param[in] section - from which section
     @param[in] name - name of the value
     @return the value
  */
  Item *currItem;
  int i;

  if (lfo == NULL)
    die ("lfo_valueGet: no cellfile object");
  if (strEqual (section,"CEL")) {
    for (i=0;i<arrayMax (lfo->cel);i++) {
      currItem = arrp (lfo->cel,i,Item);
      if (strEqual (currItem->name,name))
        return (currItem->value);
    }
    warn ("lfo_valueGet: item %s not found",name);
    return NULL;
  }
  else if (strEqual (section,"HEADER")) {
    for (i=0;i<arrayMax (lfo->header);i++) {
      currItem = arrp (lfo->header,i,Item);
      if (strEqual (currItem->name,name))
        return (currItem->value);
    }
    warn ("lfo_valueGet: item %s not found",name);
    return NULL;
  }
  else {
    warn ("lfo_valueGet: no section %s",section);
    lfo_destroy (lfo);
    return NULL;
  }
}

void lfo_maxCoordGet (CellFileObject lfo,int *x,int *y) {
  /**
     Get the maximum x and y coordinates
     @param[in] lfo - the CellFile object
     @param[in] x,y - pointers to store the x and y coordinates
     @param[out] x,y - the x and y coordinates
  */
  if (lfo == NULL)
    die ("lfo_maxCoordGet: no cellfile object");
  *x = lfo->maxX;
  *y = lfo->maxY;
}

int lfo_numIntensityGet (CellFileObject lfo) {
  /**
     Get the number of intensities
     @param[in] lfo - the CellFile object
     @return the number of intensities
  */
  if (lfo == NULL)
    die ("lfo_numIntensityGet: no cellfile object");
  return arrayMax (lfo->intensity);
}

int lfo_intensityXYGet (CellFileObject lfo,int x,int y,
                        float *mean,float *stddev,int *npix) {
  /**
     Get the intensity at x and y
     @param[in] lfo - the CellFile object
     @param[in] x,y - x and y coordinates
     @param[in] mean - pointer to which the mean can be written,
                       NULL if not interested
     @param[in] stddev - pointer to which the standard deviation can be written,
                         NULL if not interested
     @param[in] npix - pointer to which the number of pixels can be written,
                       NULL if not interested
     @param[out] mean - the mean intensity
     @param[out] stddev - the standard deviation of the intensity
     @param[out] npix - the number of pixels in the cell
     @return 1 if an intensity was returned, 0 otherwise
  */
  Intensity *currInt;
  int i;

  if (lfo == NULL)
    die ("lfo_intensityXYGet: no cellfile object");
  if (x > lfo->maxX)
    die ("lfo_intensityXYGet: x is larger than maximum (%d)",lfo->maxX);
  if (y > lfo->maxY)
    die ("lfo_intensityXYGet: y is larger than maximum (%d)",lfo->maxY);
  for (i=0;i<arrayMax (lfo->intensity);i++) {
    currInt = arrp (lfo->intensity,i,Intensity);
    if (currInt->x == x && currInt->y == y) {
      if (mean != NULL)
        *mean = currInt->mean;
      if (stddev != NULL)
        *stddev = currInt->stddev;
      if (npix != NULL)
        *npix = currInt->npix;
      return 1;
    }
  }
  return 0;
}

void lfo_intensityGet (CellFileObject lfo,int index,int *x,int *y,
                       float *mean,float *stddev,int *npix) {
  /**
     Get the intensity at a given index
     @param[in] lfo - the CellFile object
     @param[in] index - the index
     @param[in] x,y - pointers to which the coordinates can be written,
                      NULL if not interested
     @param[in] mean - pointer to which the mean can be written,
                       NULL if not interested
     @param[in] stddev - pointer to which the standard deviation can be written,
                         NULL if not interested
     @param[in] npix - pointer to which the number of pixels can be written,
                       NULL if not interested
     @param[out] x,y - the coordinates
     @param[out] mean - the mean intensity
     @param[out] stddev - the standard deviation of the intensity
     @param[out] npix - the number of pixels in the cell
     @return 1 if an intensity was returned, 0 otherwise
  */
  Intensity *currInt;

  if (lfo == NULL)
    die ("lfo_intensityGet: no cellfile object");
  if (index >= arrayMax (lfo->intensity))
    die ("lfo_intensityGet: index is larger than maximum (%d)",arrayMax (lfo->intensity));
  currInt = arrp (lfo->intensity,index,Intensity);
  if (x != NULL)
    *x = currInt->x;
  if (y != NULL)
    *y = currInt->y;
  if (mean != NULL)
    *mean = currInt->mean;
  if (stddev != NULL)
    *stddev = currInt->stddev;
  if (npix != NULL)
    *npix = currInt->npix;
}

int lfo_isMasked (CellFileObject lfo,int x,int y) {
  /**
     Returns whether the cell at coordinates x and y is masked
     @param[in] lfo - the CellFile object
     @param[in] x,y - the coordinates
     @return 1 if masked, 0 if not
  */
  int i;
  XY *currXY;

  if (lfo == NULL)
    die ("lfo_isMasked: no cellfile object");
  for (i=0;i<arrayMax (lfo->masks);i++) {
    currXY = arrp (lfo->masks,i,XY);
    if (currXY->x == x && currXY->y == y)
      return 1;
  }
  return 0;
}

int lfo_isOutlier (CellFileObject lfo,int x,int y) {
  /**
     Returns whether the cell at coordinates x and y is an outlier
     @param[in] lfo - the CellFile object
     @param[in] x,y - the coordinates
     @return 1 if outlier, 0 if not
  */
  int i;
  XY *currXY;

  if (!lfo)
    die ("lfo_isOutlier: no cellfile object");
  for (i=0;i<arrayMax (lfo->outliers);i++) {
    currXY = arrp (lfo->outliers,i,XY);
    if (currXY->x == x && currXY->y == y)
      return 1;
  }
  return 0;
}

int lfo_isModified (CellFileObject lfo,int x,int y,float *origmean)
{
  /**
     Returns whether the cell at coordinates x and y is modified
     @param[in] lfo - the CellFile object
     @param[in] x,y - the coordinates
     @param[in] origmean - the original mean
     @param[out] - the modified mean
     @return 1 if modified, 0 if not
  */
  int i;
  XYm *currXYm;

  if (!lfo)
    die ("lfo_isModified: no cellfile object");
  for (i=0;i<arrayMax (lfo->modified);i++) {
    currXYm = arrp (lfo->modified,i,XYm);
    if (currXYm->x == x && currXYm->y == y) {
      *origmean = currXYm->origmean;
      return 1;
    }
  }
  return 0;
}

/************************************************************************/

/// structure of SectionI
typedef struct {
  char *sHeader; //!< header
  Array items; //!< of Item
}SectionI;

CifFileObject ifo_create (LineStream ls) {
  /**
     Create new cif file object by reading LineStream ls.<br>
     Use ifo_destroy() the dispose of the CifFileObject
     @param[in] ls - line stream over the file
     @return CifFileObject or NULL if contents of ls did not
             have proper format
  */
  CifFileObject ifo;
  char *line;
  SectionI *currSection = malloc(sizeof(SectionI));
  Item *currItem = malloc(sizeof(Item));
  char *pos;

  ifo = (CifFileObject)hlr_malloc (sizeof (struct _cifFileObject_));
  ifo->sections = arrayCreate (10,SectionI);
  while ((line = ls_nextLine (ls)) != NULL) {
    if (line[0] == '\0')
      continue;
    if (line[0] == '[') {
      pos = strchr (line+1,']');
      if (!pos)
        die ("ifo_create: section id does not end with ]");
      *pos = '\0';
      currSection = arrayp (ifo->sections,arrayMax (ifo->sections),SectionI);
      currSection->sHeader = hlr_strdup (line+1);
      currSection->items = arrayCreate (10,Item);
    }
    else {
      currItem = arrayp (currSection->items,arrayMax (currSection->items),Item);
      pos = strchr (line,'=');
      if (!pos) {
        warn ("ifo_create: = expected between name and value in %s",line);
        ifo_destroy (ifo);
        return NULL;
      }
      *pos = '\0';
      currItem->name = hlr_strdup (line);
      currItem->value = hlr_strdup (pos+1);
    }
  }
  return ifo;
}

void ifo_destroy_func (CifFileObject ifo) {
  /**
     Destroys the ciffile object passed as argument.
     Do not call in your programs -- use ifo_destroy () instead
     @param[in] ifo - a cif file object
  */
  SectionI *currSection;
  Item *currItem;
  int i,k;

  if (ifo == NULL)
    die ("ifo_destroy_func: no ciffile object");
  for (i=0;i<arrayMax (ifo->sections);i++) {
    currSection = arrp (ifo->sections,i,SectionI);
    for (k=0;k<arrayMax (currSection->items);k++) {
      currItem = arrp (currSection->items,k,Item);
      hlr_free (currItem->name);
      hlr_free (currItem->value);
    }
    hlr_free (currSection->sHeader);
    arrayDestroy (currSection->items);
  }
  arrayDestroy (ifo->sections);
  hlr_free (ifo);
}

char *ifo_valueGet (CifFileObject ifo,char *section,char *name) {
  /**
     Get the value from a given section and a given name
     @param[in] ifo - a cif file object
     @param[in] section - name of the section
     @param[in] name - name of the item
  */
  SectionI *currSection = malloc(sizeof(SectionI));
  Item *currItem = malloc(sizeof(Item));
  int i;

  if (ifo == NULL)
    die ("ifo_valueGet: no ciffile object");
  for (i=0;i<arrayMax (ifo->sections);i++) {
    currSection = arrp (ifo->sections,i,SectionI);
    if (strEqual (currSection->sHeader,section))
      break;
  }
  if (i == arrayMax (ifo->sections)) {
    warn ("ifo_valueGet: section %s not found",section);
    return NULL;
  }
  for (i=0;i<arrayMax (currSection->items);i++) {
    currItem = arrp (currSection->items,i,Item);
    if (strEqual (currItem->name,name))
      return currItem->value;
  }
  warn ("ifo_valueGet: item %s not found",name);
  return NULL;
}

int ifo_numItemGet (CifFileObject ifo,char *section) {
  /**
     Get the number of items in a section
     @param[in] ifo - a cif file object
     @param[in] section - name of the section
  */
  SectionI *currSection = malloc(sizeof(SectionI));
  int i;

  if (ifo == NULL)
    die ("ifo_numItemGet: no ciffile object");
  for (i=0;i<arrayMax (ifo->sections);i++) {
    currSection = arrp (ifo->sections,i,SectionI);
    if (strEqual (currSection->sHeader,section))
      break;
  }
  if (i == arrayMax (ifo->sections)) {
    warn ("ifo_numItemGet: section %s not found",section);
    return 0;
  }
  return arrayMax (currSection->items);
}

int ifo_nameValueGet (CifFileObject ifo,char *section,int index,
                      char **name,char **value) {
  /**
     Get the name and value of an item at a given index in a given section
     @param[in] ifo - a cif file object
     @param[in] section - name of the section
     @param[in] index - the index of the item in the section
     @param[in] name,value - pointers to write name and value to
     @param[out] name,value - the name and value
     @return 1 if something valid returned, otherwise 0
  */
  SectionI *currSection = malloc(sizeof(SectionI));
  Item *currItem = malloc(sizeof(Item));
  int i;

  if (ifo == NULL)
    die ("ifo_nameValueGet: no ciffile object");
  for (i=0;i<arrayMax (ifo->sections);i++) {
    currSection = arrp (ifo->sections,i,SectionI);
    if (strEqual (currSection->sHeader,section))
      break;
  }
  if (i == arrayMax (ifo->sections)) {
    warn ("ifo_nameValueGet: section %s not found",section);
    return 0;
  }
  if (index >= arrayMax (currSection->items)) {
    warn ("ifo_nameValueGet: less than %d items in section",index);
    return 0;
  }
  currItem = arrp (currSection->items,index,Item);
  *name = currItem->name;
  *value = currItem->value;
  return 1;
}

/************************************************************************/

CdfFileObject dfo_create (LineStream ls) {
  /**
     Create new cdf file object by reading LineStream ls.<br>
     use dfo_destroy() the dispose of the CdfFileObject
     @param[in] ls - line stream over a cdf file
     @return CdfFileObject or NULL if contents of ls did not
             have proper format
  */
  CdfFileObject dfo;
  char *line;
  SectionD currSection = malloc(sizeof(SectionD));
  Item *currItem = malloc(sizeof(Item));
  char *pos;
  int inCells = 0;

  dfo = (CdfFileObject)hlr_malloc (sizeof (struct _cifFileObject_));
  dfo->sections = arrayCreate (10,struct _sectionD_);
  while ((line = ls_nextLine (ls)) != NULL) {
    if (line[0] == '\0') {
      inCells = 0;
      continue;
    }
    if (line[0] == '[') {
      pos = strchr (line+1,']');
      if (!pos)
        die ("dfo_create: section id does not end with ]");
      *pos = '\0';
      currSection = arrayp (dfo->sections,arrayMax (dfo->sections),struct _sectionD_);
      currSection->sHeader = hlr_strdup (line+1);
      currSection->items = arrayCreate (10,Item);
    }
    else if (strNEqual (line,"CellHeader=",11)) {
      WordIter wi;
      char *word;

      inCells = 1;
      currSection->headers = arrayCreate (10,char *);
      // special column
      textAdd (currSection->headers,"CELLNUMBER");
      wi = wordIterCreate (line+11,"\t\r\n",0);
      while ((word = wordNext (wi)) != NULL)
        textAdd (currSection->headers,word);
      wordIterDestroy (wi);
      currSection->rows = arrayCreate (100,Row);
      continue;
    }
    else if (!inCells) {
      currItem = arrayp (currSection->items,arrayMax (currSection->items),Item);
      pos = strchr (line,'=');
      if (!pos) {
        warn ("dfo_create: = expected between name and value in %s",line);
        dfo_destroy (dfo);
        return NULL;
      }
      *pos = '\0';
      currItem->name = hlr_strdup (line);
      currItem->value = hlr_strdup (pos+1);
    }
    else {
      WordIter wi;
      char *word;
      Row *currRow;

      currRow = arrayp (currSection->rows,arrayMax (currSection->rows),Row);
      currRow->cols = arrayCreate (5,char *);
      if (!strNEqual (line,"Cell",4))
        die ("dfo_create: line should start with 'Cell'");
      pos = strchr (line,'=');
      if (!pos)
        die ("dfo_create: = expected after Cellnnn");
      *pos = '\0';
      // special column CELLNUMBER
      textAdd (currRow->cols,line+4);
      wi = wordIterCreate (pos+1,"\t\r\n",0);
      while ((word = wordNext (wi)) != NULL)
        textAdd (currRow->cols,word);
      wordIterDestroy (wi);
    }
  }
  return dfo;
}

void dfo_destroy_func (CdfFileObject dfo) {
  /**
     Destroys the cdffile object passed as argument.<br>
     do not call in your programs -- use ifo_destroy () instead
     @param[in] dfo - a cdf file object
  */
  SectionD currSection;
  Item *currItem;
  Row *currRow;
  int i,k;

  if (dfo == NULL)
    die ("ifo_destroy_func: no cdffile object");
  for (i=0;i<arrayMax (dfo->sections);i++) {
    currSection = arrp (dfo->sections,i,struct _sectionD_);
    hlr_free (currSection->sHeader);
    if (currSection->items) {
      for (k=0;k<arrayMax (currSection->items);k++) {
        currItem = arrp (currSection->items,k,Item);
        hlr_free (currItem->name);
        hlr_free (currItem->value);
      }
      arrayDestroy (currSection->items);
    }
    textDestroy (currSection->headers);
    if (currSection->rows) {
      for (k=0;k<arrayMax (currSection->rows);k++) {
        currRow = arrp (currSection->rows,k,Row);
        textDestroy (currRow->cols);
      }
      arrayDestroy (currSection->rows);
    }
  }
  arrayDestroy (dfo->sections);
  hlr_free (dfo);
}

int dfo_numSectGet (CdfFileObject dfo) {
  /**
     Get the number of sections
     @param[in] dfo - a cdf file object
     @return number of sections
  */
  return arrayMax (dfo->sections);
}

SectionD dfo_sectionGetByIndex (CdfFileObject dfo,int index) {
  /**
     Get a section of a cdf file
     @param[in] dfo - a cdf file object
     @param[in] index - the index
     @return a section of a cdf file
  */
  if (dfo == NULL)
    die ("dfoSectionGetByIndex: no cdffile object");
  if (index >= arrayMax (dfo->sections))
    die ("dfoSectionGetByIndex: not this many sections");
  return arrp (dfo->sections,index,struct _sectionD_);
}

char *dfo_sectionNameGet (SectionD section) {
  /**
     Get the name of a section
     @param[in] section - a SectionD object
     @return the name of the section
  */
  if (section == NULL)
    die ("dfo_sectionNameGet: no such section");
  return section->sHeader;
}

char *dfo_sectionValueGet (SectionD section,char *name) {
  /**
     Get the value of an item of a section with the given name
     @param[in] section - a SectionD object
     @param[in] name - the name of the item
     @return the value of the item or NULL if item not found
  */
  Item *currItem;
  int i;

  if (section == NULL)
    die ("dfo_sectionValueGet: no such section");
  for (i=0;i<arrayMax (section->items);i++) {
    currItem = arrp (section->items,i,Item);
    if (strEqual (currItem->name,name))
      return currItem->value;
  }
  warn ("dfo_valueGet: item %s not found",name);
  return NULL;
}

char *dfo_sectionColGet (SectionD section,int row,char *header) {
  /**
     From a given section get the value at a given row for a given column
     @param[in] section - a SectionD object
     @param[in] row - the row number
     @param[in] header - the header of the column
     @return the value
  */
  Row *currRow;
  int i;

  if (section == NULL)
    die ("dfo_sectionColGet: no such section");
  for (i=0;i<arrayMax (section->headers);i++)
    if (strEqual (textItem (section->headers,i),header))
      break;
  if (i == arrayMax (section->headers))
    die ("Column %s not found",header);
  if (row > arrayMax (section->rows))
    die ("dfo_colGet: not so many rows");
  currRow = arrp (section->rows,row,Row);
  return textItem (currRow->cols,i);
}

/************************************************************************/

/// structure of SectionE
typedef struct {
  char *sHeader; //!< header
  Array items; //!< of Item
}SectionE;

ExpFileObject efo_create (LineStream ls) {
  /**
     Create new exp file object by reading LineStream ls.<br>
     use efo_destroy() the dispose of the ExpFileObject
     @param[in] ls - line stream over the exp file
     @return ExpFileObject or NULL if contents of ls did not
             have proper format
  */
  ExpFileObject efo;
  char *line;
  SectionE *currSection;
  Item *currItem;
  char *pos;

  efo = (ExpFileObject)hlr_malloc (sizeof (struct _expFileObject_));
  efo->sections = arrayCreate (10,SectionE);

  currSection = NULL;
  while ((line = ls_nextLine (ls)) != NULL) {
    if (line[0] == '\0')
      continue;
    if (line[0] == '[') {
      pos = strchr (line+1,']');
      if (!pos)
        die ("efo_create: section id does not end with ]");
      *pos = '\0';
      currSection = arrayp (efo->sections,arrayMax (efo->sections),SectionE);
      currSection->sHeader = hlr_strdup (line+1);
      currSection->items = arrayCreate (10,Item);
    }
    else if (currSection) {
      currItem = arrayp (currSection->items,arrayMax (currSection->items),Item);
      pos = strchr (line,'\t');
      if (!pos) {
        warn ("efo_create: \t expected between name and value in %s",line);
        efo_destroy (efo);
        return NULL;
      }
      *pos = '\0';
      currItem->name = hlr_strdup (line);
      currItem->value = hlr_strdup (pos+1);
    }
  }
  return efo;
}

void efo_destroy_func (ExpFileObject efo) {
  /**
     Destroys the expfile object passed as argument.
     Do not call in your programs -- use efo_destroy () instead
     @param[in] efo - an experiment file object
  */
  SectionE *currSection;
  Item *currItem;
  int i,k;

  if (efo == NULL)
    die ("efo_destroy_func: no expfile object");
  for (i=0;i<arrayMax (efo->sections);i++) {
    currSection = arrp (efo->sections,i,SectionE);
    for (k=0;k<arrayMax (currSection->items);k++) {
      currItem = arrp (currSection->items,k,Item);
      hlr_free (currItem->name);
      hlr_free (currItem->value);
    }
    hlr_free (currSection->sHeader);
    arrayDestroy (currSection->items);
  }
  arrayDestroy (efo->sections);
  hlr_free (efo);
}

char *efo_valueGet (ExpFileObject efo,char *section,char *name) {
  /**
     Get the value of a the item with the given name in the given section
     @param[in] efo - an experiment file object
     @param[in] section - name of the section
     @param[in] name - name of the item
     @return the value of the item, NULL if item not found
  */
  SectionE *currSection = malloc(sizeof(SectionE));
  Item *currItem = malloc(sizeof(Item));
  int i;

  if (efo == NULL)
    die ("efo_valueGet: no expfile object");
  for (i=0;i<arrayMax (efo->sections);i++) {
    currSection = arrp (efo->sections,i,SectionE);
    if (strEqual (currSection->sHeader,section))
      break;
  }
  if (i == arrayMax (efo->sections)) {
    warn ("efo_valueGet: section %s not found",section);
    return NULL;
  }
  for (i=0;i<arrayMax (currSection->items);i++) {
    currItem = arrp (currSection->items,i,Item);
    if (strEqual (currItem->name,name))
      return currItem->value;
  }
  warn ("efo_valueGet: item %s not found",name);
  return NULL;
}

int efo_numSectionParamGet (ExpFileObject efo,char *section) {
  /**
     Get the number of items in the section
     @param[in] efo - an experiment file object
     @param[in] section - name of the section
     @return number of items
  */
  SectionE *currSection = malloc(sizeof(SectionE));
  int i;

  if (efo == NULL)
    die ("efo_numSectionParamGet: no expfile object");
  for (i=0;i<arrayMax (efo->sections);i++) {
    currSection = arrp (efo->sections,i,SectionE);
    if (strEqual (currSection->sHeader,section))
      break;
  }
  if (i == arrayMax (efo->sections)) {
    warn ("efo_numSectionParamGet: section %s not found",section);
    return 0;
  }
  return arrayMax (currSection->items);
}

int efo_sectionParamGet (ExpFileObject efo,char *section,int index,
                         char **name,char **val) {
  /**
     Get name and value of item at index in section
     @param[in] efo - an experiment file object
     @param[in] section - name of the section
     @param[in] index - number of the item in the section
     @param[out] name,val - pointers to which name and value can be saved
     @param[out] name,val - name and value
     @return 1 if something valid returned, otherwise 0
  */
  SectionE *currSection = malloc(sizeof(SectionE));
  Item *currItem = malloc(sizeof(Item));
  int i;

  if (efo == NULL)
    die ("lfo_sectionParamGet: no expfile object");
  for (i=0;i<arrayMax (efo->sections);i++) {
    currSection = arrp (efo->sections,i,SectionE);
    if (strEqual (currSection->sHeader,section))
      break;
  }
  if (i == arrayMax (efo->sections)) {
    warn ("efo_sectionParamGet: section %s not found",section);
    *name = NULL;
    *val = NULL;
    return 0;
  }
  if (index < 0 || index >= arrayMax (currSection->items))
    die ("efo_sectionParamGet: no such index");
  currItem = arrp (currSection->items,index,Item);
  *name = currItem->name;
  *val = currItem->value;
  return 1;
}
