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
/** @file bitmap.c
    @brief Module dealing with bitmap objects.
    Module prefix bm_
*/
/*
  A bitmap has a width and height in pixels and a depth
  which can be 1, 4 or 8, allowing 2, 16 or 256 colors.
  It has a color map for BITMAP_MAX_NUM_COLOR entries which
  have RGB values in the range [0,255].
*/
#include "log.h"
#include "hlrmisc.h"
#include "bitmap.h"

BitmapObject bm_create (unsigned int width,unsigned int height,
                        unsigned int bpp) {
  /**
     Creates a Bitmap object
     @param[in] width,height - bitmap width and height in pixels
     @param[in] bpp - bits per pixel (should be 1, 4 or 8)
     @return a bitmap object;  the calling program should use bm_destroy
             when the bitmap is no more needed
  */
  BitmapObject this1;
  int i;

  this1 = (BitmapObject) hlr_malloc (sizeof (struct _bitmap_));
  if (this1 == NULL)
    die ("bm_create: could not create bitmapObject");
  this1->bitmap = (unsigned char *)hlr_malloc ((((width * bpp + 31) / 32) * 4) * height);
  if (this1->bitmap == NULL)
    die ("bm_create: could not create space for actual bitmap");
  this1->width = width;
  this1->height = height;
  this1->bitCount = bpp;
  this1->size = ((width * bpp + 31) / 32) * 4 * height;
  for (i=0;i<this1->size;i++)
    this1->bitmap[i] = 0;
  this1->clrUsed = 1 << bpp;
  this1->clrImportant = 0;
  return this1;
}

void bm_destroy_func (BitmapObject this1) {
  /**
     Destroys the bitmap object passed as argument;
     do not call in your programs -- use bm_destroy() instead
  */
  if (this1 == NULL)
    die ("bm_destroy_func: no bitmap object");
  hlr_free (this1->bitmap);
  hlr_free (this1);
}

void bm_getSize (BitmapObject this1,unsigned int *width,unsigned int *height,
                 unsigned int *bitPerPix) {
  /**
     Gets the size and depth of a Bitmap object
     @param[in] this1 - a bitmap object
     @param[in] width,height - can be NULL if not interested
     @param[in] bitPerPix - can be NULL if not interested
     @param[out] width,height - width and height of bitmap in pixels
     @param[out] bitPerPix - depth of bitmap in bit/pix
  */
  if (this1 == NULL)
    die ("bm_getSize: no bitmapObject");
  if (width != NULL)
    *width = this1->width;
  if (height != NULL)
    *height = this1->height;
  if (bitPerPix != NULL)
    *bitPerPix = this1->bitCount;
}

BitmapObject bm_changeBitPerPix (BitmapObject this1,int b) {
  /**
     Creates a new bitmap with the requested pixel depth and copies
     the content of the old bitmap into the new one. Deletes the old
     bitmap and returns the new one.<br>
     The number of bits per pixel can be 1 or 4 or 8.<br>
     Memory: The input bitmap is destroyed and new memory is allocated
             for the returned bitmap. The calling program should not
             try to destroy to old bitmap
     @param[in] this1 - a Bitmap object
     @param[in] b - new number of bits per pixel
     @param[out] this1 - a bitmap object woth the requested depth
  */
  BitmapObject new1;
  int i,k;

  if (this1 == NULL)
    die ("bm_changeBitPerPix: no bitmapObject");
  if (b != 1 && b != 4 && b != 8)
    die ("gif_changeBitPerPix: Bits per pixel can only be 1, 4 or 8");
  new1 = bm_create (this1->width,this1->height,b);
  if (new1 == NULL)
    die ("gif_changeBitPerPix: No memory for new bitmap");
  for (i=0;i<this1->clrUsed;i++) {
    new1->colTable[i].red = this1->colTable[i].red;
    new1->colTable[i].green = this1->colTable[i].green;
    new1->colTable[i].blue = this1->colTable[i].blue;
  }
  for (i=(int)this1->clrUsed;i< (1 << b);i++) {
    new1->colTable[i].red = 0;
    new1->colTable[i].green = 0;
    new1->colTable[i].blue = 0;
  }
  new1->clrUsed = (1 << b);
  new1->clrImportant = (1 << b);
  new1->bitCount = b;
  new1->size = ((this1->width * this1->bitCount + 31) / 32) * 4 * this1->height;
  for (i=0;i<this1->height;i++)
    for (k=0;k<this1->width;k++)
      bm_setPix (new1,i,k,bm_getPix (this1,i,k));
  bm_destroy (this1);
  return new1;
}

void bm_setColor (BitmapObject this1,int index,BitmapCol col) {
  /**
     Sets the rgb values of the color at the given index
     input: bmo: a bitmap object
            index: the color index
            col: the rgb values of the color
  */
  if (this1 == NULL)
    die ("bm_setColor: no bitmapObject");
  if (index < BITMAP_MAX_NUM_COLOR) {
    this1->colTable[index].red = col.red;
    this1->colTable[index].green = col.green;
    this1->colTable[index].blue = col.blue;
    if (index > this1->clrUsed)
      this1->clrUsed = index;
  }
}

int bm_getColors (BitmapObject this1,BitmapCol **col) {
  /**
     Get all currently defined colors
     @param[in] this1 - a Bitmap object
     @param[out] col - an array of colors
     @return how many colors are important
  */
  if (this1 == NULL)
    die ("bm_getColors: no bitmapObject");
  *col = this1->colTable;
  return (int)this1->clrImportant;
}

unsigned char bm_getPix (BitmapObject this1,unsigned int row,unsigned int col) {
  /**
     Get the value of a pixel
     @param[in] this1 - a Bitmap object
     @param[in] row,col - coordinates of the requested pixel
     @return the value of the pixel
  */
  int byte;
  unsigned char mask;
  unsigned char val;

  if (this1 == NULL)
    die ("bm_getPix: no bitmapObject");
  byte = row * (((this1->width * this1->bitCount + 31) / 32) * 4) +
                col * this1->bitCount / 8;
  val = *(this1->bitmap+byte);
  switch (this1->bitCount) {
    case 1:
      mask = 0x01;
      val = (val >> (7 - (col*this1->bitCount % 8))) & mask;
      break;
    case 4:
      mask = 0x0f;
      val = (val >> (4 - (col*this1->bitCount % 8))) & mask;
      break;
    case 8:
      mask = 0xff;
      val = val & mask;
      break;
  }
  return val;
}

void bm_setPix (BitmapObject this1,unsigned int row,unsigned int col,
                unsigned char val) {
  /**
     Set the value of a pixel
     @param[in] this1 - a Bitmap object
     @param[in] row,col - row and column of the pixel to be changed
     @param[in] val - the new value of the pixel
  */
  int byte;
  unsigned char mask;

  if (this1 == NULL)
    die ("bm_setPix: no bitmapObject");
  byte = row * (((this1->width * this1->bitCount + 31) / 32) * 4) +
                col * this1->bitCount / 8;
  switch (this1->bitCount) {
    case 1:
      mask = 0x01;
      *(this1->bitmap+byte) &= ~(mask << (7-(col*this1->bitCount % 8)));
      *(this1->bitmap+byte) |= ((mask & val) << (7-(col*this1->bitCount % 8)));
      break;
    case 4:
      mask = 0x0f;
      *(this1->bitmap+byte) &= ~(mask << (4-(col*this1->bitCount % 8)));
      *(this1->bitmap+byte) |= ((mask & val) << (4-(col*this1->bitCount % 8)));
      break;
    case 8:
      mask = 0xff;
      *(this1->bitmap+byte) = val;
      break;
  }
}

static int ff_oldVal,ff_newVal;
static unsigned int ff_rowMax;
static unsigned int ff_colMax;
static unsigned int ff_cnt; // number of pixels changed
static BitmapObject ff_bitmap;

static void floodFillRec (unsigned int row,unsigned int col) {
  ff_cnt++;
  bm_setPix (ff_bitmap,row,col,ff_newVal);
  if (row < ff_rowMax &&
      bm_getPix (ff_bitmap,row+1,col) == ff_oldVal)
    floodFillRec (row+1,col);
  if (row > 0 &&
      bm_getPix (ff_bitmap,row-1,col) == ff_oldVal)
    floodFillRec (row-1,col);
  if (col < ff_colMax &&
      bm_getPix (ff_bitmap,row,col+1) == ff_oldVal)
    floodFillRec (row,col+1);
  if (col > 0 &&
      bm_getPix (ff_bitmap,row,col-1) == ff_oldVal)
    floodFillRec (row,col-1);
}

int bm_floodFill (BitmapObject this1,
                  unsigned int row,unsigned int col,unsigned char newVal) {
  /**
     The area to be filled is defined by all pixels that have the
     same color as the pixel at position (row,col) and that can
     be reached by walking over adjacent pixels with that color only.
     For this area, set the color to 'newVal';
     If the pixel at position (row,col) already has the color 'newVal',
     nothing is done.
     The color flood will not spill outside the frame of the picture.<br>
     Postcondition: zero or more pixels changed
     @param[in] this1 - a Bitmap object
     @param[in] row, col - origin of filling
     @param[in] newVal - an index into the colormap
     @return number of pixels changed
  */
  if (this1 == NULL)
    die ("bm_floodFill: no bitmap");
  if (row >= this1->height || col >= this1->width)
    die ("bm_floodFill: row=%u, col=%u",row,col);
  ff_oldVal = bm_getPix (this1,row,col);
  if (ff_oldVal == newVal)
    return 0;
  ff_newVal = newVal;
  ff_rowMax = this1->height-1;
  ff_colMax = this1->width-1;
  ff_bitmap = this1;
  ff_cnt = 0;
  floodFillRec (row,col);
  return ff_cnt;
}

/* ------------- extension for colormap handling -------- */

BitmapCol bm_getMostAbundantColor (BitmapObject this1) {
  /**
     Determined probable background color
     @param[in] this1 - a Bitmap object
     @return must abundant color
  */
  int colcnts[BITMAP_MAX_NUM_COLOR];
  int i;
  int j = 0;
  int max = -1;
  for (i=0;i<BITMAP_MAX_NUM_COLOR;i++)
    colcnts[i] = 0;
  for (i=0;i<this1->height;i++)
    for (j=0;j<this1->width;j++)
      colcnts[bm_getPix(this1,i,j)]++;
  for (i=0;i<BITMAP_MAX_NUM_COLOR;i++)
    if (colcnts[i] > max) {
      max = colcnts[i];
      j = i;
    }
  return this1->colTable[j];
}

unsigned char bm_getColorIdx (BitmapObject this1,BitmapCol *color,int *found) {
  /**
     Determine the index of a color in the colortable
     @param[in] this1 - a Bitmap object
     @param[in] color - RGB color spec
     @param[in] found - a place to write an int to or NULL
     @param[out] *found - set to 1, if 'color' exists in the color table
                          of Bitmap, else 0; if 'found' is NULL, then
                          'color' must exist or the function die()s.
     @return index - can be used e.g. in bm_setPix() and bm_floodFill()
  */
  int i;
  int f = 0;
  for (i=0;i<this1->clrUsed;i++)
    if (!memcmp (this1->colTable + i,color,sizeof (BitmapCol))) {
      f = 1;
      break;
    }
  if (found)
    *found = f;
  else
    if (!f)
      die ("bm_getColIdx: cannot find color %d %d %d",
           color->red,color->green,color->blue);
  return (unsigned char)i;
}

/// structure to relate the color to the index in the Gif color table
typedef struct {
  BitmapCol color; //!< the color
  int index; //!< index in the GIF's color table, -1 for unknown
}IBitmapCol;

static BitmapCol centerColor;

static int centerDistance (BitmapCol *gcp) {
  return (abs (gcp->red - centerColor.red) + 1) *
         (abs (gcp->green - centerColor.green) + 1) *
         (abs (gcp->blue - centerColor.blue) + 1);
}

static int centerSort (IBitmapCol *c1,IBitmapCol *c2) {
  int d1 = centerDistance (&(c1->color));
  int d2 = centerDistance (&(c2->color));
  if (d1 < d2)
    return -1;
  if (d1 > d2)
    return 1;
  return 0;
}

Array bm_insertColors (BitmapObject this1,
                       Array requiredColors /* of BitmapCol */,
                       BitmapCol center) {
  /**
     Take the color table of the bitmap 'this1'
     and make sure that it contains the colors listed in 'requiredColors'.
     Determine the indices of these colors.
     Since the color table has a fixed size, it can happen that
     some existing colors have to be merged into one. In this
     case colors "close" to 'center' will be the victims.<br>
     Note: Not yet ready, but hopefully already useful ...
     still to do: accept other bit depths, use unused color slots
     @param[in] this1 - a Bitmap object
     @param[in] requiredColors - colors required
     @param[in] center - center of color merging
     @return Array of int - for each entry in 'requiredColors' the index in the
             GIF colortable; the indices of the returned array and
             'requiredColors' correspond to each other; the Array returned must
             be freed by its user
  */
  Array addColIndex = arrayCreate (arrayMax (requiredColors),IBitmapCol);
  int addColorsCnt; // number of colors to add to colormap
  Array bitmapColIndex; // of IBitmapCol;  color table of the GIF image, sorted
  unsigned int width;
  unsigned int height;
  unsigned int bitPerPix;
  BitmapCol *colormap;
  BitmapCol *gcp;
  BitmapCol *gcp2;
  IBitmapCol *igcp;
  int colorcnt; // number of colors that can fit into the colormap
  int i;
  int j;
  int k;
  Array index; // of int, return value
  int targetColIdx;
  int colIdx;
  int cnt;

  centerColor = center; // for use in centerSort()
  bm_getSize (this1,&width,&height,&bitPerPix);
  colorcnt = bm_getColors (this1,&colormap);
  if (colorcnt != 256 || bitPerPix != 8)
    die ("only 8 bit color table implemented");
  // copy into working table that can remember the index of a color in the colormap
  i = arrayMax (requiredColors);
  while (i--) {
    igcp = arrayp (addColIndex,i,IBitmapCol);
    igcp->index = -1; // not yet known, if or where in colormap
    igcp->color = arru (requiredColors,i,BitmapCol);
  }
  // check which of the requested colors already exist
  cnt = 0;
  i = arrayMax (addColIndex);
  while (i--) {
    igcp = arrp (addColIndex,i,IBitmapCol);
    gcp = &(igcp->color);
    for (j=0;j<colorcnt;j++) {
      gcp2 = &(colormap[j]);
      if (gcp->red == gcp2->red &&
          gcp->green == gcp2->green &&
          gcp->blue == gcp2->blue) {
        igcp->index = j;
        cnt++;
        break;
      }
    }
  }
  addColorsCnt = arrayMax (requiredColors) - cnt;
  if (addColorsCnt > colorcnt - 3)
    die ("too many new colors for this color table");
  if (addColorsCnt > 0) {
    /* before squezing the existing colors, here should be a check
       whether there are unused colors in the colormap */
    /* ... still to be implemented ... */

    /* squeeze existing colors;
       create of copy of the GIF's color table, with the addition
       of links into the GIF's color table; then sort the new table */
    bitmapColIndex = arrayCreate (colorcnt,IBitmapCol);
    for (i=0;i<colorcnt;i++) {
      igcp = arrayp (bitmapColIndex,i,IBitmapCol);
      igcp->index = i;
      igcp->color = colormap[i];
    }
    arraySort (bitmapColIndex,(ARRAYORDERF)centerSort);
    /* merge the first addColorsCnt+1 colors into one,
       by setting all pixels that use one of the colors
       bitmapColIndex[1..addColorsCnt] to bitmapColIndex[0] */
    for (i=1;i<=addColorsCnt;i++) {
      igcp = arrp (bitmapColIndex,i,IBitmapCol);
      gcp = &(igcp->color);
    }
    targetColIdx = arrp (bitmapColIndex,0,IBitmapCol)->index;
    cnt = 0;
    for (i=0;i<height;i++)
      for (j=0;j<width;j++) {
        colIdx = bm_getPix (this1,i,j);
        for (k=1;k<=addColorsCnt;k++)
          if (arrp (bitmapColIndex,k,IBitmapCol)->index == colIdx) {
            bm_setPix (this1,i,j,targetColIdx);
            cnt++;
          }
      }
    /* here holds: bitmapColIndex[1..addColorsCnt] point to unused
       slots in the GIF's colormap; 'unused' means there are no
       pixels using the colormap slots */
    // donate these unused slots to addColIndex[]
    j = 1; // index of entry in bitmapColIndex pointing to unused slot
    i = arrayMax (addColIndex);
    while (i--) {
      igcp = arrp (addColIndex,i,IBitmapCol);
      if (igcp->index == -1) {
        igcp->index = arrp (bitmapColIndex,j,IBitmapCol)->index;
        j++;
      }
    }
  } // if addColorsCnt > 0
  // copy the new (and old) color definitions into the GIF's colormap
  i = arrayMax (addColIndex);
  while (i--) {
    igcp = arrp (addColIndex,i,IBitmapCol);
    if (igcp->index == -1)
      die ("oops");
    bm_setColor (this1,igcp->index,igcp->color);
  }
  // fork out output vector
  index = arrayCreate (arrayMax (addColIndex),int);
  for (i=0;i<arrayMax (addColIndex);i++)
    array (index,i,int) = arrp (addColIndex,i,IBitmapCol)->index;
  if (arrayMax (index) != arrayMax (requiredColors))
    die ("oops");
  return index;
}

/* ------------- end of extensions for color map handling -------- */

void bm_recolorBox (BitmapObject this1,
                    int xmin,int ymin,int xmax,int ymax,
                    BitmapCol *old,BitmapCol *new1) {
  /**
     Assign all pixels within the rectangle with corners
     (xmin,ymin),(xmax,ymax) and have color 'old' the color 'new'
     @param[in] this1 - a Bitmap object
     @param[in] xmin,ymin,xmax,ymax - in pixels
     @param[in] old - color present in colormap
     @param[in] new1 - color present in colormap
     @return this1 - recolored
  */
  int i;
  int j;
  unsigned char oldColIdx = bm_getColorIdx (this1,old,NULL);
  unsigned char newColIdx = bm_getColorIdx (this1,new1,NULL);

  for (i=ymin;i<=ymax;i++)
    for (j=xmin;j<=xmax;j++)
      if (bm_getPix (this1,i,j) == oldColIdx)
        bm_setPix (this1,i,j,newColIdx);
}
