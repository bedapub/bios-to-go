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
/** @file graphics.c
    @brief Drawing routines.
    Many of the routines come originally from the contribs of Daylight
    (www.daylight.com).
    Module prefix gr_
*/
#include <ctype.h>
#include <math.h>
#include "log.h"
#include "hlrmisc.h"
#include "bitmap.h"
#include "graphics.h"

/// the absolute value
#define ABS(A) (((A) >= 0) ? (A) : -(A))
/// transform degrees into radians
#define RADIAN(A) A*3.14159/180.0

static void (*linesub_hook)(GraphPoint beg,GraphPoint end,int lw) = NULL;
static void (*drawbox_hook)(GraphRect *rect) = NULL;
static void (*fillbox_hook)(GraphRect *rect) = NULL;
static void (*drawoval_hook)(GraphRect *rect) = NULL;
static void (*filloval_hook)(GraphRect *rect) = NULL;
static void (*fillpoly_hook)(GraphPoly *poly) = NULL;
static void (*deinit_hook)(void) = NULL;

static int colorIndex = 0; // can be set from outside
static int lineWidth = 1; // can be set from outside
static char svgColorStr[7];
static GraphRect clipping;
static float lineAngle; // only used internally
static unsigned int XRange,YRange;
static int linePattLen;
static char linePattern[1000];
static int linePattPtr;
static char orientation;
static BitmapObject g_bmo;
static FILE *g_stream;

static void gr_init (void) {
  /**
     Called by the init functions like gr_initForBitmap() or gr_initForSVG().<br>
     Postcondition: orientation is 'P' (affects e.g. gr_moveto)
  */
  int i;

  for (i=0;i<sizeof (linePattern);i++)
    linePattern[i] = 1;
  linePattLen = sizeof (linePattern);
  linePattPtr = 0;
  gr_setOrientation ('P');
  clipping.xmin = 0;
  clipping.xmax = XRange-1;
  clipping.ymin = 0;
  clipping.ymax = YRange-1;
}

// bitmap_linesub and bitmap_setdot call each other
static void bitmap_linesub (GraphPoint beg,GraphPoint end,int lw);

static void bitmap_setdot (int x,int y,int lw) {
  /**
     draws into the bitmap if the linepattern requires a dot at the present
     position.
     if the line width is > 1, calls bitmap_linesub with a line which is
     90 degrees rotated relative to the actual line and of length
     'linewidth'.
     input: x,y : pixel position in the bitmap
            lw : line width in pixels
  */
  float a;
  GraphPoint b,e;

  if (orientation == 'P') {
    if (x < 0 || y < 0 || x >= XRange || y >= YRange)
      return;
  }
  else {
    if (x < 0 || y < 0 || x >= YRange || y >= XRange)
      return;
  }
  if (lw > 0) {
    if (linePattPtr >= linePattLen)
      linePattPtr = 0;
    if (linePattern[linePattPtr++] == 1) {
      if (orientation == 'P')
        bm_setPix (g_bmo,y,x,colorIndex);
      else
        bm_setPix (g_bmo,x,XRange-y,colorIndex);
      if (lw > 1) {
        a = lineAngle + 90.0;
        b.x = x + lw/2 * cos (a/180.0*3.14159);
        b.y = y + lw/2 * sin (a/180.0*3.14159);
        e.x = x - lw/2 * cos (a/180.0*3.14159);
        e.y = y - lw/2 * sin (a/180.0*3.14159);
        bitmap_linesub (b,e,0);
      }
    }
  }
  else {
    if (orientation == 'P')
      bm_setPix (g_bmo,y,x,colorIndex);
    else
      bm_setPix (g_bmo,x,XRange-y,colorIndex);
  }
}

static void bitmap_linesub (GraphPoint b,GraphPoint e,int lw) {
  /**
     implements the Bresenham algorithm to draw lines into a bitmap,
     is called by myline (and by bitmap_setdot if the linewidth is > 1),
     calls bitmap_setdot to actually draw into the bitmap.
     input: b,e begin and end of line segment (already clipped)
            lw: line width in pixels
  */
  int x,y,z,a1,b1,dx,dy,d,deltap,deltaq;

  dx = abs ((int)e.x-(int)b.x);
  dy = abs ((int)e.y-(int)b.y);
  if (dy <= dx) {
    x = (int)b.x;
    y = (int)b.y;
    z = (int)e.x;
    a1 = (b.x <= e.x) ? 1 : -1;
    b1 = (b.y <= e.y) ? 1 : -1;
    deltap = dy + dy;
    d = deltap - dx;
    deltaq = d - dx;
    bitmap_setdot (x,y,lw);
    while (x != z) {
      x += a1;
      if (d < 0)
        d += deltap;
      else {
        y += b1;
        d += deltaq;
      }
      bitmap_setdot (x,y,lw);
    }
  }
  else {
    y = (int)b.y;
    x = (int)b.x;
    z = (int)e.y;
    a1 = (b.y <= e.y) ? 1 : -1;
    b1 = (b.x <= e.x) ? 1 : -1;
    deltap = dx + dx;
    d = deltap - dy;
    deltaq = d - dy;
    bitmap_setdot (x,y,lw);
    while (y != z) {
      y += a1;
      if (d < 0)
        d += deltap;
      else {
        x += b1;
        d += deltaq;
      }
      bitmap_setdot (x,y,lw);
    }
  }
}

static void drawBox_bitmap (GraphRect *rect) {
  GraphPoint p;

  p.x = rect->xmin;
  p.y = rect->ymin;
  gr_moveto (&p);
  p.x = rect->xmax;
  gr_drawto (&p);
  p.y = rect->ymax;
  gr_drawto (&p);
  p.x = rect->xmin;
  gr_drawto (&p);
  p.y = rect->ymin;
  gr_drawto (&p);
}

static void fillBox_bitmap (GraphRect *rect) {
  GraphPoint p;
  int i;

  if (rect->ymin > rect->ymax)
    return;
  for (i=(int)rect->ymin;i<=(int)rect->ymax;i++) {
    p.x = rect->xmin;
    p.y = i;
    gr_moveto (&p);
    p.x = rect->xmax;
    gr_drawto (&p);
  }
}

static void drawOval_bitmap (GraphRect *rect) {
  float a,b;
  int x;
  GraphPoint p,c;

  a = 0.5 * (rect->xmax - rect->xmin);
  b = 0.5 * (rect->ymax - rect->ymin);
  c.x = rect->xmin + a;
  c.y = rect->ymin + b;
  p.x = rect->xmin;
  p.y = c.y;
  gr_moveto (&p);
  for (x=-(int)a;x<=(int)a;x++) {
    p.x = c.x + x;
    p.y = c.y + sqrt ((1.0 - ((x*x) / (a*a))) * b*b);
    gr_drawto (&p);
  }
  for (x=(int)(a-1);x>=(int)(-a);x--) {
    p.x = c.x + x;
    p.y = c.y - sqrt ((1.0 - ((x*x) / (a*a))) * b*b);
    gr_drawto (&p);
  }
}

static void fillOval_bitmap (GraphRect *rect) {
  float a,b;
  int x;
  GraphPoint p,c;
  float tmp;

  a = 0.5 * (rect->xmax - rect->xmin);
  b = 0.5 * (rect->ymax - rect->ymin);
  c.x = rect->xmin + a;
  c.y = rect->ymin + b;
  for (x=(int)(-a);x<=(int)a;x++) {
    p.x = c.x + x;
    tmp = sqrt ((1.0 - x*x / (a*a)) * b*b);
    p.y = c.y + tmp;
    gr_moveto (&p);
    p.y = c.y - tmp;
    gr_drawto (&p);
  }
}

/// used in fillPoly_bitmap
#define HATCH_SIZE 100

static void fillPoly_bitmap (GraphPoly *poly) {
  int min,max;
  int imin=0,imax=0;
  GraphPoint p;
  int hatchx[HATCH_SIZE];
  int i,d,m,dx,dy,h,i1,i2;
  int prev;

  if (poly->numVert > MAX_POLY)
    die ("gr_fillPoly: number of vertices cannot be larger than %d",MAX_POLY);
  poly->x[poly->numVert] = poly->x[0];
  poly->y[poly->numVert] = poly->y[0];
  min = 1000;
  max = -1000;
  for (i=0;i<poly->numVert;i++) {
    if (poly->y[i] > max) {
      max = (int)poly->y[i];
      imax = i;
    }
    if (poly->y[i] < min) {
      min = (int)poly->y[i];
      imin = i;
    }
  }
  for (d=(int)poly->y[imin];d<=(int)poly->y[imax];d++) {
    m = 0;
    for (i=0;i<poly->numVert;i++) {
      dy = (int)(poly->y[i+1]-poly->y[i]);
      if (dy == 0)
        continue;
      if (poly->y[i] == d) {
        prev = i-1;
        if (prev < 0)
          prev = poly->numVert-1;
        if ((poly->y[prev] > d && poly->y[i+1] > d) ||
            (poly->y[prev] < d && poly->y[i+1] < d))
          hatchx[m++] = (int)poly->x[i];
        hatchx[m++] = (int)poly->x[i];
      }
      else {
        if (dy < 0 && (d >= poly->y[i] || d <= poly->y[i+1]))
          continue;
        if (dy > 0 && (d <= poly->y[i] || d >= poly->y[i+1]))
          continue;
        dx = (int)(poly->x[i+1]-poly->x[i]);
        hatchx[m++] = (int)(poly->x[i] + dx * (d-poly->y[i]) / dy);
      }
      if (m >= HATCH_SIZE)
        die ("gr_fillPoly: hatchx too small");
    }
    if (m > 0) {
      if (m > 1) {
        for (i1=0;i1<m-1;i1++)
          for (i2=i1+1;i2<m;i2++)
            if (hatchx[i1] > hatchx[i2]) {
              h=hatchx[i1];
              hatchx[i1]=hatchx[i2];
              hatchx[i2]=h;
            }
        for (i=0;i<m;i+=2) {
          p.x = hatchx[i];
          p.y = d;
          gr_moveto (&p);
          p.x = hatchx[i+1];
          p.y = d;
          gr_drawto (&p);
        }
      }
    }
  }
}

void gr_initForBitmap (BitmapObject bmo) {
  /**
     Initializes the graphics module to draw into b bitmap
     @param[in] bmo - the bitmap object
  */
  if (bmo == NULL)
    die ("gr_initForBitmap: no bitmap object");
  bm_getSize (bmo,&XRange,&YRange,NULL);
  linesub_hook = bitmap_linesub;
  drawbox_hook = drawBox_bitmap;
  fillbox_hook = fillBox_bitmap;
  drawoval_hook = drawOval_bitmap;
  filloval_hook = fillOval_bitmap;
  fillpoly_hook = fillPoly_bitmap;
  g_bmo = bmo;
  gr_init ();
}

static void svg_linesub (GraphPoint b,GraphPoint e,int lw) {
  float x1,y1,x2,y2;

  x1 = b.x;
  y1 = b.y;
  x2 = e.x;
  y2 = e.y;
  if (orientation == 'L') {
    float tmp;

    tmp = x1;
    x1 = y1;
    y1 = tmp;
    tmp = x2;
    x2 = y2;
    y2 = tmp;
    x1 = XRange - x1;
    x2 = XRange - x2;
  }
  fprintf (g_stream,"<line x1=\"%g\" y1=\"%g\" x2=\"%g\" y2=\"%g\" fill=\"none\" stroke=\"#%s\" stroke-width=\"%d\" />\n",
           x1,YRange-y1,x2,YRange-y2,svgColorStr,lw);
}

static void drawBox_svg (GraphRect *rect) {
  float x1,y1,x2,y2;

  x1 = rect->xmin;
  y1 = rect->ymin;
  x2 = rect->xmax;
  y2 = rect->ymax;
  if (orientation == 'L') {
    float tmp;

    tmp = x1;
    x1 = y1;
    y1 = tmp;
    tmp = x2;
    x2 = y2;
    y2 = tmp;
    x1 = XRange - x1;
    x2 = XRange - x2;
  }
  fprintf (g_stream,"<rect x=\"%g\" y=\"%g\" width=\"%g\" height=\"%g\" fill=\"none\" stroke=\"#%s\" stroke-width=\"%d\" />\n",
           x1,YRange-y2,x2-x1,y2-y1,svgColorStr,lineWidth);
}

static void fillBox_svg (GraphRect *rect) {
  float x1,y1,x2,y2;

  x1 = rect->xmin;
  y1 = rect->ymin;
  x2 = rect->xmax;
  y2 = rect->ymax;
  if (orientation == 'L') {
    float tmp;

    tmp = x1;
    x1 = y1;
    y1 = tmp;
    tmp = x2;
    x2 = y2;
    y2 = tmp;
    x1 = XRange - x1;
    x2 = XRange - x2;
  }
  fprintf (g_stream,"<rect x=\"%g\" y=\"%g\" width=\"%g\" height=\"%g\" fill=\"#%s\" stroke=\"#%s\" stroke-width=\"%d\" />\n",
           x1,YRange-y2,x2-x1,y2-y1,
           svgColorStr,svgColorStr,lineWidth);
}

static void drawOval_svg (GraphRect *rect) {
  float a,b;
  GraphPoint c;
  float x1,y1,x2,y2;

  x1 = rect->xmin;
  y1 = rect->ymin;
  x2 = rect->xmax;
  y2 = rect->ymax;
  if (orientation == 'L') {
    float tmp;

    tmp = x1;
    x1 = y1;
    y1 = tmp;
    tmp = x2;
    x2 = y2;
    y2 = tmp;
    x1 = XRange - x1;
    x2 = XRange - x2;
  }
  a = 0.5 * (x2 - x1);
  b = 0.5 * (y2 - y1);
  c.x = x1 + a;
  c.y = y2 + b;
  fprintf (g_stream,"<ellipse cx=\"%g\" cy=\"%g\" rx=\"%g\" ry=\"%g\" fill=\"none\" stroke=\"#%s\" stroke-width=\"%d\" />\n",
           c.x,YRange-c.y,a,b,svgColorStr,lineWidth);
}

static void fillOval_svg (GraphRect *rect) {
  float a,b;
  GraphPoint c;
  float x1,y1,x2,y2;

  x1 = rect->xmin;
  y1 = rect->ymin;
  x2 = rect->xmax;
  y2 = rect->ymax;
  if (orientation == 'L') {
    float tmp;

    tmp = x1;
    x1 = y1;
    y1 = tmp;
    tmp = x2;
    x2 = y2;
    y2 = tmp;
    x1 = XRange - x1;
    x2 = XRange - x2;
  }
  a = 0.5 * (x2 - x1);
  b = 0.5 * (y2 - x1);
  c.x = x1 + a;
  c.y = y1 + b;
  fprintf (g_stream,"<ellipse cx=\"%g\" cy=\"%g\" rx=\"%g\" ry=\"%g\" fill=\"#%s\" stroke=\"#%s\" stroke-width=\"%d\" />\n",
           c.x,YRange-c.y,a,b,svgColorStr,svgColorStr,lineWidth);
}

static void fillPoly_svg (GraphPoly *poly) {
  int i;
  float x,y;

  fprintf (g_stream,"<polygon fill=\"#%s\" stroke=\"#%s\" stroke-width=\"%d\" points=\"",
           svgColorStr,svgColorStr,lineWidth);
  for (i=0;i<poly->numVert;i++) {
    x = poly->x[i];
    y = poly->y[i];
    if (orientation == 'L') {
      float tmp;

      tmp = x;
      x = y;
      y = tmp;
      x = XRange - x;
    }
    fprintf (g_stream,"%g,%g ",x,YRange-y);
  }
  fprintf (g_stream,"\" />\n");
}

static void deinit_svg (void) {
  fprintf (g_stream,"</svg>\n");
}

void gr_initForSVG (FILE *stream,float width,float height) {
  /**
     Initializes the graphics module to create SVG commands.
     @param[in] stream - the sream to write to
     @param[in] width,height - width and height of graphics
  */
  XRange = (unsigned int)width;
  YRange = (unsigned int)height;
  linesub_hook = svg_linesub;
  drawbox_hook = drawBox_svg;
  fillbox_hook = fillBox_svg;
  drawoval_hook = drawOval_svg;
  filloval_hook = fillOval_svg;
  fillpoly_hook = fillPoly_svg;
  deinit_hook = deinit_svg;
  g_stream = stream;
  gr_init ();
  strcpy (svgColorStr,"ffffff");
  fprintf (stream,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  // fprintf (stream,"<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1 Tiny//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11-tiny.dtd\">\n");
  fprintf (stream,"<svg version=\"1.1\" baseprofile=\"tiny\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" id=\"svg-root\" witdth=\"100%%\" height=\"100%%\" viewBox=\"0 0 %.0f %.0f\">\n",
           width,height);
}

void gr_deinit (void) {
  /**
     Deinitializes the graphics module
  */
  if (deinit_hook != NULL)
    deinit_hook ();
}

static GraphPoint curpt = { 0.0, 0.0 };
static GraphRect bounds = { 0.0, 0.0, 100.0, 100.0 };
static float rsizes[10] =
                   {1.0, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1};
static float rsuper[10] =
                   {0.0, -0.8, -0.6, -0.4, -0.2, 0.0, 0.2, 0.4, 0.6, 0.8};

void gr_setColor (int ci) {
  /**
     For bitmap graphics set the current color for drawing
     @param[in] ci - the color index
  */
  colorIndex = ci;
}

void gr_setSvgColorRGB (float r,float g,float b) {
  /**
     For SVG graphics set the current color for drawing in RGB mode
     @param[in] r,g,b - red, green and blue values (0.0 - 1.0).
     unsigned char type makes sure that the value between 0 and 255
  */
  snprintf (svgColorStr, 7, "%02x%02x%02x",(unsigned char)(r*255),(unsigned char)(g*255),(unsigned char)(b*255));
}

void gr_setSvgColor (int col) {
  /**
     For SVG graphics set the current color for drawing as a color value
     @param[in] col - one of the symbols like SVG_BLACK
  */
  if (col == SVG_BLACK)
    strcpy (svgColorStr,"000000");
  else if (col == SVG_WHITE)
    strcpy (svgColorStr,"ffffff");
  else if (col == SVG_RED)
    strcpy (svgColorStr,"ff0000");
  else if (col == SVG_GREEN)
    strcpy (svgColorStr,"00ff00");
  else if (col == SVG_BLUE)
    strcpy (svgColorStr,"0000ff");
  else if (col == SVG_MAGENTA)
    strcpy (svgColorStr,"ff00ff");
  else if (col == SVG_CYAN)
    strcpy (svgColorStr,"00ffff");
  else if (col == SVG_YELLOW)
    strcpy (svgColorStr,"ffff00");
  else if (col == SVG_GREY)
    strcpy (svgColorStr,"aaaaaa");
  else if (col == SVG_DARKGREY)
    strcpy (svgColorStr,"888888");
  else
    die ("gr_setSvgColor: unknown color %d",col);
}

void gr_setLineWidth (int lw) {
  /**
     Set the line width in pixels
     @param[in] lw - the line width in pixels
  */
  lineWidth = lw;
}

void gr_setLinePattern (int pattLen,char *patt) {
  /**
     Set the ine pattern
     @param[in] pattLen - the length of the pattern, e.g. 10
     @param[in] patt - the pattern, e.g. \1\0\1\0\1\0\1\0\1\0
  */
  int i;

  linePattLen = pattLen;
  for (i=0;i<pattLen;i++)
    linePattern[i] = patt[i];
  linePattPtr = 0;
}

void gr_setClipping (GraphRect *cl) {
  /**
     Set the clipping rectangle
     @param[in] cl - the clipping rectangle
  */
  clipping = *cl;
}

void gr_setBounds (GraphRect *rect) {
  /**
     Set the size of the drawing area
     @param[in] rect - the drawing rectangle
   */
  bounds.xmin = rect->xmin;
  bounds.ymin = rect->ymin;
  bounds.xmax = rect->xmax;
  bounds.ymax = rect->ymax;
}

void gr_fitBounds (GraphRect *rect) {
  /**
     Adjust drawing area to fit aspect ratio
     @param[in] rect - the drawing rectangle
     @param[out] rect - the drawing rectangle adjusted
   */
  float yrange,xrange,extra,aspect;

  bounds.xmin = rect->xmin;
  bounds.ymin = rect->ymin;
  bounds.xmax = rect->xmax;
  bounds.ymax = rect->ymax;
  yrange = bounds.ymax - bounds.ymin;
  xrange = bounds.xmax - bounds.xmin;
  aspect = yrange / xrange / gr_getAspect ();
  if (aspect > 1.0) {
    extra = (xrange * aspect - xrange) / 2.0;
    bounds.xmin -= extra;
    bounds.xmax += extra;
  }
  else {
    extra = (yrange / aspect - yrange) / 2.0;
    bounds.ymin -= extra;
    bounds.ymax += extra;
  }
}

GraphRect gr_getBounds (void) {
  /**
     Get the current size of the draiwng area
     @return the rectangle of the drawing area
  */
  return bounds;
}

void gr_moveto (GraphPoint *pt) {
  /**
     Move the current point to a new location
     @param[in] pt - the new location
  */
  curpt.x = pt->x;
  curpt.y = pt->y;
}

/// begin left
#define BL 1
/// end left
#define EL 2
/// begin right
#define BR 4
/// end right
#define ER 8
/// begin top
#define BT 16
/// end top
#define ET 32
/// begin bottom
#define BB 64
/// end bottom
#define EB 128

static void myline (GraphPoint b,GraphPoint e,GraphRect c) {
  /**
     draws a line from b to e but only the part which is within the
     clipping rectangle c
  */
  int exch,done;
  GraphPoint t;
  float k;
  char code;

  b.x = ceil (b.x);
  b.y = ceil (b.y);
  e.x = ceil (e.x);
  e.y = ceil (e.y);

  if (e.x-b.x > 0.0)
    lineAngle = atan ((e.y-b.y)/(e.x-b.x)) * 180.0 / 3.14159;
  else if (e.x-b.x < 0.0)
    lineAngle = 180.0 + atan ((e.y-b.y)/(e.x-b.x)) * 180.0 / 3.14159;
  else {
    if (e.y-b.y > 0.0)
      lineAngle = 90.0;
    else
      lineAngle = 270.0;
  }
  if (lineWidth < 1)
    lineWidth = 1;
  done = 0;
  exch = 0;
  while (!done) {
    if (e.x>=c.xmin && e.x<=c.xmax && e.y>=c.ymin && e.y<=c.ymax) {
      if (b.x>=c.xmin && b.x<=c.xmax && b.y>=c.ymin && b.y<=c.ymax) {
        if (exch) {
          t = b;
          b = e;
          e = t;
        }
        (*linesub_hook) (b,e,lineWidth);
        done = 1;
      }
      else {
        t = b;
        b = e;
        e = t;
        exch = !exch;
      }
    }
    if (!done) {
      code = 0;
      if (b.x<c.xmin) code |= BL;
      if (b.x>c.xmax) code |= BR;
      if (e.x<c.xmin) code |= EL;
      if (e.x>c.xmax) code |= ER;
      if (b.y<c.ymin) code |= BB;
      if (b.y>c.ymax) code |= BT;
      if (e.y<c.ymin) code |= EB;
      if (e.y>c.ymax) code |= ET;
      if ((code & BL && code & EL) ||
          (code & BR && code & ER) ||
          (code & BT && code & ET) ||
          (code & BB && code & EB))
        done = 1;
      if (!done) {
        if (code & EL || code & ER) {
          k = (e.y-b.y)/(e.x-b.x);
          if (code & EL) {
            e.x = c.xmin;
            e.y = k*(c.xmin-b.x)+b.y;
          }
          else {
            e.x = c.xmax;
            e.y = k*(c.xmax-b.x)+b.y;
          }
        }
        else if (code & EB || code & ET) {
          k = (e.x-b.x)/(e.y-b.y);
          if (code & ET) {
            e.x = k*(c.ymax-b.y)+b.x;
            e.y = c.ymax;
          }
          else {
            e.x = k*(c.ymin-b.y)+b.x;
            e.y = c.ymin;
          }
        }
      }
    }
  }
}

void gr_drawto (GraphPoint *pt) {
  /**
     Draw to a new location
     @param[in] pt - the new location
  */
  myline (curpt,*pt,clipping);
  curpt.x = pt->x;
  curpt.y = pt->y;
}

void gr_drawBox (GraphRect *rect) {
  /**
     Draw a rectangle
     @param[in] rect - the rectangle
  */
  drawbox_hook (rect);
}

void gr_fillBox (GraphRect *rect) {
  /**
     Fill a rectangle
     @param[in] rect - the rectangle
  */
  fillbox_hook (rect);
}

void gr_drawOval (GraphRect *rect) {
  /**
     Draw an oval
     @param[in] rect - the rectangle bounding the oval
  */
  drawoval_hook (rect);
}

void gr_fillOval (GraphRect *rect) {
  /**
     Fill an oval
     @param[in] rect - the rectangle bounding the oval
  */
  filloval_hook (rect);
}

void gr_fillPoly (GraphPoly *poly) {
  /**
     Fill a polygon
     @param[in] poly - the polygon
  */
  fillpoly_hook (poly);
}

float gr_getAspect (void) {
  /**
     Get the aspect ratio
     @return the aspect ratio
  */
  return 1.0;
}

void gr_setOrientation (char o) {
  /**
     Set the orientation of the graphics:
     orienation is 'P' portrait or 'L' landscape.
     In portrait x is horizontal, y vertical, 0,0 lower left.
     In landscape y is horizontal, x vertical, 0.0 lower left.
     @param[in] o - the orientation: P or L
  */
  orientation = o;
  if (o == 'P') {
    clipping.xmin = 0;
    clipping.xmax = XRange-1;
    clipping.ymin = 0;
    clipping.ymax = YRange-1;
  }
  else {
    clipping.xmin = 0;
    clipping.xmax = YRange-1;
    clipping.ymin = 0;
    clipping.ymax = XRange-1;
  }
}

void gr_rgb2hsv (float rgb[3],float hsv[3]) {
  /**
     Translate color from RGB into HSV system.
     r,g,b in [0,1];
     h in [0,360] s,v in [0,1]
     @param[in] rgb - the RGB values
     @param[out] hsv - the HSV values
  */
  float max,min,d;

  if (rgb[1] > rgb[0])
    max = rgb[1];
  else
    max = rgb[0];
  if (max < rgb[2])
    max = rgb[2];
  if (rgb[1] < rgb[0])
    min = rgb[1];
  else
    min = rgb[0];
  if (min > rgb[2])
    min = rgb[2];
  hsv[2] = max;
  if (max != 0.0)
    hsv[1] = (max-min) / max;
  else
    hsv[1] = 0.0;
  if (hsv[1] == 0.0) {
    hsv[0] = 0.0;
    warn ("gr_rgb2hsv: h undefined");
  }
  else {
    d = max - min;
    if (rgb[0] == max)
      hsv[0] = (rgb[1] - rgb[2]) / d;
    else if (rgb[1] == max)
      hsv[0] = 2 + (rgb[2]-rgb[0])/d;
    else if (rgb[2] == max)
       hsv[0] = 4 + (rgb[0]-rgb[1])/d;
    hsv[0] *= 60.0;
    if (hsv[0] < 0.0)
      hsv[0] += 360.0;
  }
}

void gr_hsv2rgb (float hsv[3],float rgb[3]) {
  /**
     Translate color from HSV into RGB system.
     h in [0,360] s,v in [0,1];
     r,g,b in [0,1]
     @param[in] hsv - the HSV values
     @param[out] rgb - the RGB values
  */
  int i;
  float f,p,q,t;

  if (hsv[1] == 0.0) { /* black and white center line */
    if (hsv[0] < 0.0 || hsv[0] > 360.0) { /* achromatic color, no hue */
      rgb[0] = rgb[1] = rgb[2] = hsv[2];
    }
    else {
      rgb[0] = rgb[1] = rgb[2] = 0.0;
      warn ("gr_hsv2rgb: s is 0, h > 0");
    }
  }
  else { // chromatic color
    if (hsv[0] == 360.0)
      hsv[0] = 0.0;
    hsv[0] /= 60.0;
    i = (int)(floor (hsv[0]));
    f = hsv[0] - i;
    p = hsv[2] * (1 - hsv[1]);
    q = hsv[2] * (1 - hsv[1] * f);
    t = hsv[2] * (1 - hsv[1] * (1 - f));
    switch (i) {
      case 0 : rgb[0] = hsv[2];
               rgb[1] = t;
               rgb[2] = p;
               break;
      case 1 : rgb[0] = q;
               rgb[1] = hsv[2];
               rgb[2] = p;
               break;
      case 2 : rgb[0] = p;
               rgb[1] = hsv[2];
               rgb[2] = t;
               break;
      case 3 : rgb[0] = p;
               rgb[1] = q;
               rgb[2] = hsv[2];
               break;
      case 4 : rgb[0] = t;
               rgb[1] = p;
               rgb[2] = hsv[2];
               break;
      case 5 : rgb[0] = hsv[2];
               rgb[1] = p;
               rgb[2] = q;
               break;
    }
  }
}

//------- submodule dealing with drawing of characters

/// number of chars in font
#define DM_ALPHACOUNT 128
/// maximum length of char description
#define DM_ALPHALENGTH 80

static int fontbuilt; // Font table built yet?
static char *font[DM_ALPHACOUNT]; // string char descriptions
static float wid[DM_ALPHACOUNT]; // char widths
static float up[DM_ALPHACOUNT]; // char ascents
static float down[DM_ALPHACOUNT]; // char descents

static int charvalue (char c) {
  /**
     return coded character location value
  */
  if (isdigit (c))
    return '0' - c; // Negative values for non-zero digits.
  if (isupper (c)) // Alphabet positions for others.
    return c - 'A';
  return c - 'a';
}

static void font_init (void) {
  /**
     Each character is represented as an (even length) string of characters.
     Each pair of characters in the string represents a vector end-point
     coordinate (in units of .05) based on its ASCII value.
     Upper case characters mean "move", lower case characters mean "draw".
     Odd characters represent height, digits being negative.
  */
  int i;

/* original characters from Daylight ???
  font[' ']="AaHa";
  font['A']="AacghqmgoaCgmg";
  font['B']="AaiakblcmelgkhiikjlkmmlokpiqaqBqbabiii";
  font['C']="Mclbjaeacbbcaeambocpeqjqlpmo";
  font['D']="AaiakblcmemmlokpiqaqBqba";
  font['E']="KqaqaakaAihi";
  font['F']="KqaqaaAihi";
  font['G']="MclbjaeacbbcaeambocpeqjqlpmoMamgHgog";
  font['H']="AaaqKakqAiki";
  font['I']="AagaAqgqDadq";
  font['J']="AgaebccbeagbhcieiqDqnq";
  font['K']="AaaqAgkqCika";
  font['L']="Kaaaaq";
  font['M']="Aaaqhgoqoa";
  font['N']="Aaaqmamq";
  font['O']="Aebccbeaiakblcmemmlokpiqeqcpboamae";
  font['P']="Aaaqiqkplommmlljkiihah";
  font['Q']="AebccbeaiakblcmemmlokpiqeqcpboamaeHfma";
  font['R']="AaaqiqkplommmlljkiihahIhma";
  font['S']="Adcbeaiakblcmelgkhcjbkambocpeqiqkpmn";
  font['T']="HahqAqoq";
  font['U']="Aqaebccbeaiakblcmemq";
  font['V']="Aqgamq";
  font['W']="Aqeahikaoq";
  font['X']="AqkaAakq";
  font['Y']="AqfikqFafi";
  font['Z']="Aqkqaaka";
  font['a']="BjdkikjjkhkblaKbiadabbadbfdgigke";
  font['b']="BccbeagaibjckekgjiijgkekcjbiAqbqbaaa";
  font['c']="Jjhkekcjbiagaebccbeahajb";
  font['d']="KcgaeacbbcaeagbicjekhkjjkhLakakqlq";
  font['e']="Kcibgaeacbbcaeagbicjekhkjjkhkgag";
  font['f']="BadaCacndpfqhpinAhfh";
  font['g']="KcibgaeacbbcaeahbjdkhkjjkhLkkkk3j5h6e6c5b4";
  font['h']="AqbqbaaaBhcjekhkjjkhka";
  font['i']="AaeaCacjbjClcmbmblcl";
  font['j']="GmhmhlglgmGjhjh3g5e6d6b5a3";
  font['k']="AababqaqBdhkIaeg";
  font['l']="AaeaCacqbq";
  font['m']="AkbkbaBhcjekfkhjihiaihjjlkmkojphpa";
  font['n']="AkbkbaaaBhcjekhkjjkhka";
  font['o']="Aebccbeagaibjckekgjiijgkekcjbiagae";
  font['p']="BccbeagaibjckekgjiijgkekcjbiAkbkb6a6";
  font['q']="KcgaeacbbcaeagbicjekhkjjkhLkkkk6l6";
  font['r']="AkbkbaBhcjekhkji";
  font['s']="Abcagaibjdiebgahbjdkhkjj";
  font['t']="AkikEoebfahaib";
  font['u']="AkaebccbeagaibjckeKkkala";
  font['v']="Akfakk";
  font['w']="Akeahgkaok";
  font['x']="AkkaAakk";
  font['y']="AkaebccbeagaibjckeKkk3j5h6e6c5b4";
  font['z']="Akkkaaka";
  font['0']="DqbpanadbbdahajbkdknjphqdqBbjp";
  font['1']="AndqdaAaga";
  font['2']="Kaaaacbehijjklknjphqdqbpan";
  font['3']="AnbpdqhqjpknkljjhieiHijhkfkdjbhadabbad";
  font['4']="Kfafhqha";
  font['5']="Kqaqaicjhjjikgkdjbhacaab";
  font['6']="Jphqdqbpanadbbdahajbkdkgjihjdjbiah";
  font['7']="Aqkqda";
  font['8']="DqbpanalbjdihijjklknjphqdqDibhafadbbdahajbkdkfjhhi";
  font['9']="Bbdahajbkdknjphqdqbpanakbidhhhjikk";
  font['!']="CbdbdccccbCedeeqbqce";
  font['#']="AfmfAkmkDcdnJcjn";
  font['^']="Akeqik";
  font['&']="Kaibbmbncpeqfqgogmfkbiagaebccbeafahbke";
  font['$']="AdcchcjdkfjhhidibjalbndoioknFafq";
  font['\'']="Epdpdqeqeocm";
  font['`']="Cpdpdqcqcoem";
  font['"']="CpbpbqcqcobnFpepeqfqfoen";
  font['.']="Dacacbdbda";
  font[',']="Dacacbdbdad1c2";
  font[':']="DccccddddcCjdjdkckcj";
  font[';']="DccccddddcdbcaCjdjdkckcj";
  font['~']="Albncoeofnfmgliljmko";
  font['_']="Aaka";
  font['-']="Aiii";
  font['+']="AiiiEeem";
  font['>']="Aakiaq";
  font['<']="Kaaikq";
  font['(']="Eacbbcaeambocpeq";
  font[')']="Aacbdceeemdocpaq";
  font[']']="Aafafqaq";
  font['[']="Faaaaqfq";
  font['{']="Eacbbdbfchaicjblbncpeq";
  font['}']="Aacbdddfcheicjdldncpaq";
  font['?']="AlambocpeqgqipjoknkljjhighfffdFbfagagbfb";
  font['=']="AmimAhih";
  font['%']="AanqIblbmcmflgighfhcibCkfkglgofpcpboblck";
  font['/']="Aamq";
  font['\\']="Aqma";
  font['|']="Aaaq";
  font['@']="Khjfheffehejflhmjlkkkeldmdneogomnompkqeqcpboamaebccbeaka";
  font['*']="AjljCejoCoje";
*/
  font[' '] = "AaHa";
  font['!'] = "DccbdaebdcDdcjdkejddEbcbDcda";
  font['"'] = "BhbkEhek";
  font['#'] = "AadkGkdaGdadAhgh";
  font['$'] = "AdccdcdaDcecgdgeefcfagahcididkDieigh";
  font['%'] = "AagkBjaibhcibjFbecfdgcfbAiciBjbhEcgcFbfd";
  font['&'] = "Gabidkeiaeaccafbgd";
  font['\''] = "Dhdk";
  font['('] = "Dacbbdbhcjdk";
  font[')'] = "Bacbdddhcjbk";
  font['*'] = "GdahDjdbAdgh";
  font['+'] = "DbdjAfgf";
  font[','] = "C1ebdccbdaCbebDadc";
  font['-'] = "Afgf";
  font['.'] = "DaebdccbdaCbebDadc";
  font['/'] = "Aagk";
  font['0'] = "Fhfdeacabdbhckekfh";
  font['1'] = "CidkdaBafa";
  font['2'] = "Aickekgiggffcebdaaga";
  font['3'] = "Akgkcfdgfggegceacaac";
  font['4'] = "EkaegeFafi";
  font['5'] = "Accaeagcgeegbgbkgk";
  font['6'] = "Aecgeggegceacaacaickekgi";
  font['7'] = "CadfgkakCfef";
  font['8'] = "Cfadaccaeagcgdefcfahaickekgighef";
  font['9'] = "Accaeagcgiekckaiagceeegg";
  font[':'] = "DbecddccdbDheidjcidhCcecDbddCieiDhdj";
  font[';'] = "C1ebdccbdaDheidjcidhCbebDadcCieiDhdj";
  font['<'] = "Gbafgj";
  font['='] = "GdadAhgh";
  font['>'] = "Abgfaj";
  font['?'] = "DaebdccbdaDedfefghgiekckaiCbebDadc";
  font['@'] = "Fcdbcbacahcjdjfhfeeddedgdgcgceddde";
  font['A'] = "AadkgaCfef";
  font['B'] = "AaakekgighefAfefgdgceaaa";
  font['C'] = "Gceacaacaickekgi";
  font['D'] = "Aaakekgigceaaa";
  font['E'] = "GaaaakfkAfef";
  font['F'] = "AaakgkAfef";
  font['G'] = "Dfgfgceacaacaickekgi";
  font['H'] = "AaakAfgfGagk";
  font['I'] = "BafaDadkBkfk";
  font['J'] = "AcabbadaebekCkgk";
  font['K'] = "AkaaGacgAefk";
  font['L'] = "Akaaga";
  font['M'] = "Aaakdfgkga";
  font['N'] = "Aaakgagk";
  font['O'] = "Aickekgigceacaacai";
  font['P'] = "Aaakekgighefaf";
  font['Q'] = "Caacaickekgigceacacbdcga";
  font['R'] = "AaakekgighefafDfga";
  font['S'] = "Abbafagbgdffbfagaibkfkgi";
  font['T'] = "DadkAkgk";
  font['U'] = "Akaccaeagcgk";
  font['V'] = "Akdagk";
  font['W'] = "Akaadfgagk";
  font['X'] = "AkgaAagk";
  font['Y'] = "AkdfdaDfgk";
  font['Z'] = "Akgkaaga";
  font['['] = "Fababkfk";
  font['\\'] = "Akga";
  font[']'] = "Bafafkbk";
  font['^'] = "Afdkgf";
  font['_'] = "A1g1";
  font['`'] = "Bkeh";
  font['a'] = "GeegcgaeaccaeagcGagg";
  font['b'] = "AkaaAccaeagcgeegcgae";
  font['c'] = "Gceacaacaecgegff";
  font['d'] = "GeegcgaeaccaeagcGagj";
  font['e'] = "Aegeegcgaeaccaeagc";
  font['f'] = "BgfgDadjekgk";
  font['g'] = "Ggg2e4c4a2Geegcgaeaccaeagc";
  font['h'] = "AaakAecgeggega";
  font['i'] = "DadgDicjdkejdiEjcjDidk";
  font['j'] = "A4c4e2egEidjekfjeiFjdjEkej";
  font['k'] = "AkaaGaceAdfg";
  font['l'] = "BafaDadkbk";
  font['m'] = "AgaaAebgcgdedaDeegfggega";
  font['n'] = "AgaaAecgeggega";
  font['o'] = "Egcgaeaccaeagcgeeg";
  font['p'] = "Aga4Accaeagcgeegcgae";
  font['q'] = "GeegcgaeaccaeagcG4gg";
  font['r'] = "AgaaAecgegge";
  font['s'] = "Abbafagbgcfdbdaeafbgfggf";
  font['t'] = "BgfgGaeadbdj";
  font['u'] = "AgaccaeagcGgga";
  font['v'] = "Agdagg";
  font['w'] = "Agbadffagg";
  font['x'] = "AggaAagg";
  font['y'] = "A2c4e4g2ggAgaccaeagc";
  font['z'] = "Agggaaga";
  font['{'] = "Fadbcebfafbfcgdjfk";
  font['|'] = "Dadk";
  font['}'] = "Badbeeffgfffegdjbk";
  font['~'] = "Aibkckeifigk";

  for (i=0;i<DM_ALPHACOUNT;i++) {
    char *s;
    int  x,y,v;

    wid[i] = up[i] = down[i] = 0;
    s = font[i];
    if (! s) continue;
    while (*s) {
      x = s[0];
      y = s[1];
      if (charvalue(x) > wid[i])
        wid[i] = charvalue(x);
      v = charvalue(y);
      if (v > up[i])
        up[i] = v;
      if (v < down[i])
        down[i] = v;
      s += 2;
    }
  }
  fontbuilt++;
}

static GraphRect vectorchar (unsigned char c, char *vstr) {
  /**
     return bounding box & description of char c
  */
  GraphRect box;

  if (!fontbuilt)
    font_init ();
  if (c < ' ' || c >= 127) {
    if (c > 165)
      c = ' ';
    else if (c == 128)
      c = 'C';
    else if (c == 129)
      c = 'u';
    else if (c == 130 || c == 136 || c == 137 || c == 138)
      c = 'e';
    else if (c == 131 || c == 132 || c == 133 || c == 134 || c == 145 || c == 160)
      c = 'a';
    else if (c == 135 || c == 155)
      c = 'c';
    else if (c == 139 || c == 140 || c == 141 || c == 161)
      c = 'i';
    else if (c == 142 || c == 143 || c == 146)
      c = 'A';
    else if (c == 144)
      c = 'E';
    else if (c == 147 || c == 148 || c == 149 || c == 162)
      c = 'o';
    else if (c == 150 || c == 151 || c == 163)
      c = 'u';
    else if (c == 152)
      c = 'y';
    else if (c == 153)
      c = 'O';
    else if (c == 154)
      c = 'U';
    else if (c == 156)
      c = 'L';
    else if (c == 157)
      c = 'Y';
    else if (c == 158)
      c = 'P';
    else if (c == 159)
      c = 'F';
    else if (c == 164)
      c = 'n';
    else if (c == 165)
      c = 'N';
    else
      c = ' ';
  }
  strcpy (vstr,font[c]);
  box.xmin = 0.0;
  box.xmax = 0.05 * wid[c];
  box.ymax = 0.05 * up[c];
  box.ymin = 0.05 * down[c];
  return box;
}

static GraphPoint shiftpoint (GraphPoint point,GraphPoint offset,float angle) {
  /**
     shift point by given amount (rotate, translate, scale)
     This rotates offset by angle (in degrees), corrects for the current
     aspect ratio of the display, adds it to point, and returns the result
   */
   GraphPoint p;
   float theta, sint, cost;

   theta = RADIAN (angle);
   if (fabs (theta) < 0.02) {
      sint = theta;
      cost = 1.0;
   }
   else {
      sint = sin (theta);
      cost = cos (theta);
   }
   p.x = point.x + (offset.x * cost + offset.y * sint) * gr_getAspect ();
   p.y = point.y + (offset.y * cost - offset.x * sint);
   return p;
}

static void cmove (GraphPoint loc,float size,float angle,char cx,char cy) {
  /**
     move in character coordinates
  */
  GraphPoint p;

  p.x  = size * charvalue (cx);
  p.y  = size * charvalue (cy);
  p = shiftpoint (loc,p,angle);
  gr_moveto (&p);
}

static void cdraw (GraphPoint loc,float size,float angle,int cx,int cy) {
  /**
     draw in character coordinates
  */
  GraphPoint p;

  p.x = size * charvalue (cx);
  p.y = size * charvalue (cy);
  p = shiftpoint (loc,p,angle);
  gr_drawto (&p);
}

static GraphPoint drawchar (GraphPoint loc,float size,float angle,char c) {
  /**
     draw a character, return sizes used
  */
  GraphPoint point;
  GraphRect  box;
  char vstr[DM_ALPHALENGTH],*vp=vstr;

  // Get the description of how to "implement" c
  box = vectorchar (c,vstr);
  // Now implement it
  while (*vp) {
    if (isupper (vp[0]))
      cmove (loc,0.05*size,-angle,tolower(vp[0]),vp[1]);
    else
      cdraw (loc,0.05*size,-angle,vp[0],vp[1]);
    vp += 2;
  }
  point.x = size * (box.xmax + 0.2);
  point.y = 0.0;
  // Figure out how much space this all took, and report it to caller
  return shiftpoint (loc,point,-angle);
}

static GraphPoint rotatepoint (GraphPoint point,float theta) {
  /**
     rotate point about origin though angle th
  */
   GraphPoint rotated;
   float sint, cost;

   if (fabs (theta) < 0.02) {
      sint = theta;
      cost = 1.0;
   }
   else {
      sint = sin (theta);
      cost = cos (theta);
   }
   rotated.x = point.x * cost + point.y * sint;
   rotated.y = point.y * cost - point.x * sint;
   return rotated;
}

// --------- public functions dealing with labels

void gr_fitLabel (GraphLabel *lab,GraphRect *box) {
  /**
     Draw label to fill the box
     @param[in] lab - the label
     @param[in] box - the box
  */
  GraphLabel label;
  GraphRect bb1;
  float height,width,room;

  // Measure required width and height
  width = box->xmax - box->xmin;
  height = box->ymax - box->ymin;
  // Measure label when fit vertically
  label.text = lab->text;
  label.sizes = lab->sizes;
  label.super = lab->super;
  label.loc.x = box->xmin + 0.5 * width;
  label.loc.y = box->ymin + 0.5 * height;
  label.size = 0.8 * height;
  label.angle = 0.0;
  strcpy (label.tag,"MC");
  bb1 = gr_labelBounds (&label);
  // Shrink to fit horizontally if needed. (ASPECT?)
  room = 0.9 * width; // /gr_getAspect(); ???
  if (bb1.xmax > room)
    label.size *= room / bb1.xmax;
  // if (bb1->xmax > 0.9 * width) label.size *= 0.9 * width / bb1->xmax;
  // Draw the label
  gr_drawLabel (&label);
}

void gr_drawLabel (GraphLabel *lab) {
  /**
     Draw label with given attributes.
     For each character in the label there can be a sizes character
     which can be '0' to '9': '0' does not modify the basic size,
     increasing values diminish the size for the given character.
     For each character in the label there can be a super character
     which can be '0' to '9': '0' writes the character at the baseline,
     the subscript is increasing from '4' to '1', '5' again is at
     the baseline and the superscript is increasing from '6' to '9'.
     The first character of the tag indicates the vertical alignment:
     B: bottom, T: top, U: higher than T, M: middle,
     C: a little bit higher than M
     @param[in] lab - the label
  */
  GraphRect box;
  GraphPoint p,rp,sp,loc,shift;
  float siznow,offset;
  char *text,*sizes,*super;

  shift.x = 0.0;
  shift.y = 0.0;
  loc = lab->loc;
  // if tag is not "LL", we must make some adjustments
  if (lab->tag[0] != 'L' || lab->tag[1] != 'L') {
    box = gr_labelBounds(lab);
    if (lab->tag[0] == 'C')
      // shift.y = -0.5 * DL_MIN (box.ymax, 0.8 * lab->size);
      shift.y = -0.4 * lab->size;
    else if (lab->tag[0] == 'M')
      shift.y = -0.5 * (box.ymax + box.ymin);
    else if (lab->tag[0] == 'U')
      // shift.y = - DL_MIN(box.ymax, 0.8 * lab->size);
      shift.y = -0.8 * lab->size;
    else if (lab->tag[0] == 'T')
      shift.y = - box.ymax;
    else if (lab->tag[0] == 'B')
      shift.y = - box.ymin;
    else
      shift.y = 0.0;
    if (lab->tag[1] == 'C')
      shift.x = -0.5 * box.xmax;
    else if (lab->tag[1] == 'R')
      shift.x = - box.xmax;
    else
      shift.x = 0.0;
  }
  p.x = shift.x;
  p.y = shift.y;
  // compute the "shifted" point
  loc = shiftpoint (loc,p,-lab->angle);
  offset = 0.0;
  siznow = lab->size;
  p.x = 0.0;
  text = lab->text;
  sizes = lab->sizes;
  super = lab->super;
  while (*text) {
    if (*sizes) {
      if (isdigit (*sizes))
        siznow = lab->size * rsizes[*sizes - '0'];
      sizes++;
    }
    if (*super) {
      if (isdigit (*super))
        offset = lab->size * rsuper[*super - '0'];
      super++;
    }
    p.y = offset;
    rp  = rotatepoint (p,RADIAN (-lab->angle));
    loc = shiftpoint (loc,p,-lab->angle);
    /* vchar lets us know our "limits". Unadjust by the rotation amount,
       so we can readjust by the rotation amount next time through.
       (Could this be done easier?) */
    sp = drawchar (loc, siznow, lab->angle, *text);
    loc.x = sp.x - rp.x * gr_getAspect ();
    loc.y = sp.y - rp.y;
    text++;
  }
}

GraphRect gr_labelBounds (GraphLabel *lab) {
  /**
     Return bounding box of given label
     Return size in a GraphRect, where the left edge of string is at the origin
          *---*-----------+
          *   *  ***  ****|
          ***** *   * *   *
          *   * *   * *   *
     -0-  *   *  ***  ****|
          |           *   |
          +-----------*---+
          0
     @param[in] lab - the label
     @return the bounding box of the label
  */
  GraphRect box;
  char vstr[DM_ALPHALENGTH]; // Char description string goes here.
  float width,above,below; // Width of string, distance above, below
  float siznow,offset;
  char *text,*sizes,*super; // local copies of string pointers

  width = above = below = 0; // Zero the box dimensions.
  offset= 0; // Set up default offset,
  siznow= lab->size; // and the default character size.
  text  = lab->text;
  sizes = lab->sizes;
  super = lab->super;
  while (*text) {
    if (*sizes) {
      if (isdigit (*sizes))
        siznow = lab->size * rsizes[*sizes - '0'];
      sizes++;
    }
    if (*super) {
      if (isdigit (*super))
        offset = lab->size * rsuper[*super - '0'];
       super++;
    }
    box    = vectorchar(*text, vstr);
    width += siznow * (box.xmax + 0.2);
    if (offset + siznow * box.ymax > above)
      above = offset + siznow * box.ymax;
    if (offset + siznow * box.ymin < below)
      below = offset + siznow * box.ymin;
    text++;
  }
  width -= siznow * 0.2; // Subtract off .2 of last char
  // Package up answer, and return it
  box.xmin = 0;
  box.xmax = width;
  box.ymax = above;
  box.ymin = below;
  return box;
}

//------------- axis labeling submodule

static double al_interval;
static double al_gMax;
static double al_nextLabel,al_currLabel;

void gr_al_init (double min,double max,int maxNumLabel) {
  /**
     Create useful labels along an axis.<br>
     Postcondition: gr_al_moreLabels() can be called
     @param[in] min,max - user coordinates of the range for which to generate
                          labels
     @param[in] maxNumLabel - the maximum number of labels to generate
  */
  double d;
  int f,f1;
  double i;
  double factor;

  al_gMax  = max;
  d = max - min;
  if (d <= 0.0) {
      al_nextLabel = al_gMax + 1.0; /* there will be no valid labels */
    return;
  }
  factor = 1.0;
  while (d < 1.0) {
    d *= 10.0;
    factor *= 10.0;
  }
  f = (int) (log (d) / log (10.0));
  i = al_interval = pow (10.0,(double) (f-1));
  f1 = 2;
  while ((int)(d/al_interval) > maxNumLabel) {
    al_interval = f1 * i;
    if (f1 == 2)
      f1 = 5;
    else if (f1 == 5)
      f1 = 10;
    else if (f1 == 10)
      f1 = 20;
    else if (f1 == 20)
      f1 = 50;
    else if (f1 == 50)
      f1 = 100;
    else
      break;
  }
  al_interval /= factor;
  al_nextLabel = al_interval;
  if (al_nextLabel > min) {
    while (al_nextLabel-al_interval >= min)
      al_nextLabel -= al_interval;
  }
  else if (al_nextLabel < min) {
    while (al_nextLabel+al_interval < min)
      al_nextLabel += al_interval;
    al_nextLabel += al_interval;
  }
}

int gr_al_moreLabels (void) {
  /**
     Check whether one more label was generated
     @return 1 if there is one more label, 0 if not
  */
  if (al_nextLabel <= al_gMax) {
    al_currLabel = al_nextLabel;
    al_nextLabel += al_interval;
    return 1;
  }
  return 0;
}

double gr_al_getLabel (void) {
  /**
     Returns value of the label.<br>
     Precondition: can be called if gr_al_moreLabels() returned 1
     @return the value of the label
  */
  return al_currLabel;
}

//----------- coordinate transformation submodule

GraphCoordTrans gr_ct_create (double minU,double maxU,int minP,int maxP) {
  /**
     Creates a coordinate transformer
     @param[in] minU,maxU - user coordinates
     @param[in] minP,maxP - corresponding pixel coordinates
     @return a GraphCoordTrans object
  */
  GraphCoordTrans this1;

  this1 = (GraphCoordTrans)hlr_malloc (sizeof (struct _coordTransStruct_));
  this1->minU = minU;
  this1->maxU = maxU;
  this1->minP = minP;
  this1->maxP = maxP;
  return this1;
}

void gr_ct_destroy_func (GraphCoordTrans this1) {
  /**
     Destroys a GraphCoordTrans object.<br>
     Note: do not call this but call the corresponding macro
     @param[in] this1 - a valid GraphCoordTrans object
  */
  if (this1 == NULL)
    die ("gr_ct_destroy_func: no GraphCoordTrans");
  hlr_free (this1);
}

int gr_ct_toPix (GraphCoordTrans ct,double x) {
  /**
     Returns pixel coordinate for the user coordinate x
     @param[in] ct - a valid GraphCoordTrans object
     @param[in] x - a user coordinate
     @return the pixel coordinate
  */
  if (!ct)
    die ("gr_ct_toPix: no GraphCoordTrans");
  return (int)(ct->minP + (x - ct->minU) * (ct->maxP - ct->minP) / (ct->maxU - ct->minU));
}

double gr_ct_toUser (GraphCoordTrans ct,int x) {
  /**
     Returns user coordinate for the pixel coordinate
     @param[in] ct - a valid GraphCoordTrans object
     @param[in] x - a pixel coordinate
     @return the user coordinate
  */
  if (!ct)
    die ("gr_ct_toUser: no GraphCoordTrans");
  return ct->minU + (x - ct->minP) * (ct->maxU - ct->minU) / (ct->maxP - ct->minP);
}
