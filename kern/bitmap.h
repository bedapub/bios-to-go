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
/** @file bitmap.h
    @brief Module dealing with bitmap objects.
    Module prefix bm_
*/
#ifndef BITMAP_H
#define BITMAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "array.h"

/// this is the number of colors that fit into 8 bit
#define BITMAP_MAX_NUM_COLOR 256

/// the BitmapCol object
typedef struct {
  int red; //!< [0,255]
  int green; //!< [0,255]
  int blue; //!< [0,255]
}BitmapCol;

/// the BitmapCol object
typedef struct _bitmap_ {
  unsigned short width; //!< in pixels
  unsigned short height; //!< in pixels
  unsigned int bitCount; //!< pixel depth
  unsigned long size; //!< bitmap size in bytes
  unsigned char *bitmap; //!< pointer to the actual bitmap
  unsigned long clrUsed; //!< number of colors used
  unsigned long clrImportant; //!< number of ??? colors
  BitmapCol colTable[BITMAP_MAX_NUM_COLOR]; //!< the color table
}*BitmapObject;

extern BitmapObject bm_create (unsigned int width,unsigned int height,
                               unsigned int bpp);
extern void bm_destroy_func (BitmapObject this1);
/// use this macro instead of bm_destroy_func()
#define bm_destroy(this1) (bm_destroy_func(this1),this1=NULL)
extern void bm_getSize (BitmapObject this1,
                        unsigned int *width,unsigned int *height,
                        unsigned int *bitPerPix);
extern BitmapObject bm_changeBitPerPix (BitmapObject this1,int b);
extern void bm_setColor (BitmapObject this1,int index,BitmapCol col);
extern int bm_getColors (BitmapObject this1,BitmapCol **col);
extern unsigned char bm_getPix (BitmapObject this1,
                                unsigned int row,unsigned int col);
extern void bm_setPix (BitmapObject this1,
                       unsigned int row,unsigned int col,unsigned char val);
extern int bm_floodFill (BitmapObject this1,
                         unsigned int row,unsigned int col,
                         unsigned char newVal);

extern BitmapCol bm_getMostAbundantColor (BitmapObject this1);
extern unsigned char bm_getColorIdx (BitmapObject this1,
                                     BitmapCol *color,int *found);
extern Array bm_insertColors (BitmapObject this1,
                              Array requiredColors,
                              BitmapCol center);
extern void bm_recolorBox (BitmapObject this1,
                           int xmin,int ymin,int xmax,int ymax,
                           BitmapCol *old,BitmapCol *new1);

#ifdef __cplusplus
}
#endif

#endif
