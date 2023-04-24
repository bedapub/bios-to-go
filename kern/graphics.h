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
/** @file graphics.h
    @brief Drawing routines.
    Many of the routines come originally from the contribs of Daylight
    (www.daylight.com).
    Module prefix gr_
*/
#ifndef GRAPHICS_H
#define GRAPHICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bitmap.h"

/// structure for a point
typedef struct {
  float x; //!< x coordinate of the point
  float y; //!< y coordinate of the point
}GraphPoint;

/// structure for a rectangle
typedef struct {
  float xmin; //!< minimum x coordinate of the rectangle
  float ymin; //!< maximum x coordinate of the rectangle
  float xmax; //!< minimum y coordinate of the rectangle
  float ymax; //!< maximum y coordinate of the rectangle
}GraphRect;

/// structure for a label
typedef struct {
  char *text; //!< the text itself
  char *sizes; //!< sizes for each character
  char *super; //!< super/subscript for each character
  GraphPoint loc; //!< location where to draw the label
  float size; //!< basic size of the label
  float angle; //!< angle of the label
  char tag[10]; //!< tag indicating whether label is left/right adjusted or centered
}GraphLabel;

/// maximum edges i a polygon
#define MAX_POLY 100

/// structure for a polygon
typedef struct {
  float x[MAX_POLY]; //!< x coordinates of vertices of polygon
  float y[MAX_POLY]; //!< y coordinates of vertices of polygon
  int numVert; //!< how many vertices
}GraphPoly;

/// color black for SVG
#define SVG_BLACK    0
/// color white for SVG
#define SVG_WHITE    1
/// color red for SVG
#define SVG_RED      2
/// color green for SVG
#define SVG_GREEN    3
/// color blue for SVG
#define SVG_BLUE     4
/// color magenta for SVG
#define SVG_MAGENTA  5
/// color cyan for SVG
#define SVG_CYAN     6
/// color yellow for SVG
#define SVG_YELLOW   7
/// color grey for SVG
#define SVG_GREY     8
/// color dark gray for SVG
#define SVG_DARKGREY 9

extern void gr_initForBitmap (BitmapObject bmo);
extern void gr_initForSVG (FILE *stream,float width,float height);
extern void gr_deinit (void);

extern void gr_setColor (int ci);
extern void gr_setSvgColorRGB (float r,float g,float b);
extern void gr_setSvgColor (int col);

extern void gr_setLineWidth (int lw);
extern void gr_setLinePattern (int pattLen,char *patt);

extern void gr_setClipping (GraphRect *cl);
extern void gr_setBounds (GraphRect *rect);
extern void gr_fitBounds (GraphRect *rect);
extern GraphRect gr_getBounds (void);
extern void gr_moveto (GraphPoint *pt);
extern void gr_drawto (GraphPoint *pt);
extern void gr_drawBox (GraphRect *rect);
extern void gr_fillBox (GraphRect *rect);
extern void gr_drawOval (GraphRect *rect);
extern void gr_fillOval (GraphRect *rect);
extern void gr_fillPoly (GraphPoly *poly);
extern float gr_getAspect (void);
extern void gr_setOrientation (char o);

extern void gr_rgb2hsv (float rgb[3],float hsv[3]);
extern void gr_hsv2rgb (float hsv[3],float rgb[3]);

extern void gr_fitLabel (GraphLabel *lab,GraphRect *box);
extern void gr_drawLabel (GraphLabel *lab);
extern GraphRect gr_labelBounds (GraphLabel *lab);

extern void gr_al_init (double min,double max,int maxNumLabel);
extern int gr_al_moreLabels (void);
extern double gr_al_getLabel (void);

/// the GraphCoordTrans object
typedef struct _coordTransStruct_ {
  double minU; //!< minimum user coordinate
  double maxU; //!< maximum user coordinate
  int minP; //!< minimum pixel coordinate
  int maxP; //!< maximum pixel coordinate
}*GraphCoordTrans;

extern GraphCoordTrans gr_ct_create (double minU,double maxU,int minP,int maxP);
extern void gr_ct_destroy_func (GraphCoordTrans this1);
/// use this macro; do not use gr_ct_destroy_func()
#define gr_ct_destroy(this1) (gr_ct_destroy_func(this1),this1=NULL)
extern int gr_ct_toPix (GraphCoordTrans ct,double x);
extern double gr_ct_toUser (GraphCoordTrans ct,int x);

#ifdef __cplusplus
}
#endif

#endif
