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
/** @file algutil.c
    @brief Makes local or global sequence alignments.
    Uses code from EMBOSS.
    Module prefix algutil_
*/
#include <limits.h>
#include <float.h>
#include <math.h>
#include "format.h"
#include "log.h"
#include "sequtil.h"
#include "algutil.h"

static char *gSeq1=NULL;
static char *gSeq2=NULL;
static int gLen1,gLen2;
static int gIsNuc = -1;
static int gMode;
static float gGapO;
static float gGapE;
static float gEndGapO;
static float gEndGapE;
static int gDoEndWeight;
static Stringa gAlg1=NULL,gAlg2=NULL;

static float (*weight_symb_hook)(char c1,char c2);
static float (*weight_pos_hook)(int p1,int p2);

static float getScoreNW (float *ix,float *iy,float *m,
                         int *start1,int *start2) {
  // from EMBOSS embAlignGetScoreNWMatrix
  int i,j,cursor;
  float score = INT_MIN;
  *start1 = gLen1-1;
  *start2 = gLen2-1;

  if (gDoEndWeight) {
    /* when using end gap penalties the score of the optimal global
       alignment is stored in the final cell of the path matrix */
    cursor = gLen1 * gLen2 - 1;
    if (m[cursor] > ix[cursor] && m[cursor] > iy[cursor])
      score = m[cursor];
    else if (ix[cursor] > iy[cursor])
      score = ix[cursor];
    else
      score = iy[cursor];
  }
  else {
    for (i=0;i<gLen2;i++) {
      cursor = (gLen1 - 1) * gLen2 + i;
      if (m[cursor] > score) {
        *start2 = i;
        score = m[cursor];
      }
      if (ix[cursor] > score) {
        score = ix[cursor];
        *start2 = i;
      }
      if (iy[cursor] > score) {
        score = iy[cursor];
        *start2 = i;
      }
    }
    for (j=0;j<gLen1;j++) {
      cursor = j * gLen2 + gLen2 - 1;
      if (m[cursor] > score) {
        *start1 = j;
        *start2 = gLen2-1;
        score = m[cursor];
      }
      if (ix[cursor] > score) {
        score = ix[cursor];
        *start1 = j;
        *start2 = gLen2-1;
      }
      if (iy[cursor] > score) {
        score = iy[cursor];
        *start1 = j;
        *start2 = gLen2-1;
      }
    }
  }
  return score;
}

/// direction diagonal
#define DIAG 0
/// direction left
#define LEFT 1
/// direction down
#define DOWN 2
/// used to evaluate the path
#define E_FPEQ(a,b,e) (((b - e) < a) && (a < (b + e)))
/// used to evaluate the path
#define U_FEPS 1.192e-6F // 1.0F + E_FEPS != 1.0F

static float pathCalcWithEndGapPenalties (int *start1,int *start2,
                                          int *compass) {
  // from EMBOSS embAlignPathCalcWithEndGapPenalties
  int xpos;
  int ypos;
  int seq2pos;
  float match;
  float ixp;
  float iyp;
  float mp;
  int cursor;
  int cursorp;
  float testog;
  float testeg;
  float score;
  static float *m = NULL;
  static float *ix = NULL;
  static float *iy = NULL;
  static int prevDim = 0;

  if (gLen1*gLen2 > prevDim) {
    prevDim = gLen1*gLen2;
    if (m != NULL)
      hlr_free (m);
    m = (float *)hlr_calloc (prevDim,sizeof (float));
    if (ix != NULL)
      hlr_free (ix);
    ix = (float *)hlr_calloc (prevDim,sizeof (float));
    if (iy != NULL)
      hlr_free (iy);
    iy = (float *)hlr_calloc (prevDim,sizeof (float));
  }
  if (!gDoEndWeight) {
    gEndGapO = 0.0;
    gEndGapE = 0.0;
    gDoEndWeight = 1;
  }
  if (weight_pos_hook != NULL)
    match = (*weight_pos_hook)(0,0);
  else
    match = (*weight_symb_hook)(gSeq1[0],gSeq2[0]);
  ix[0] = -gEndGapO-gGapO;
  iy[0] = -gEndGapO-gGapO;
  m[0] = match;
  cursor = 0;
  // first initialise the first column
  for (ypos=1;ypos<gLen1;ypos++) {
    if (weight_pos_hook != NULL)
      match = (*weight_pos_hook)(ypos,0);
    else
      match = (*weight_symb_hook)(gSeq1[ypos],gSeq2[0]);
    cursor = ypos * gLen2;
    cursorp = (ypos-1) * gLen2;
    testog = m[cursorp] - gGapO;
    testeg = iy[cursorp] - gGapE;
    if(testog >= testeg)
      iy[cursor] = testog;
    else
      iy[cursor] = testeg;
    m[cursor] = match - (gEndGapO + (ypos - 1) * gEndGapE);
    ix[cursor] = -gEndGapO - ypos * gEndGapE - gGapO;
  }
  ix[cursor] -= gEndGapO;
  ix[cursor] += gGapO;
  cursor=0;
  // now initialise the first row
  for (xpos=1;xpos<gLen2;xpos++) {
    if (weight_pos_hook != NULL)
      match = (*weight_pos_hook)(0,xpos);
    else
      match = (*weight_symb_hook)(gSeq1[0],gSeq2[xpos]);
    cursor = xpos;
    cursorp = xpos -1;
    testog = m[cursorp] - gGapO;
    testeg = ix[cursorp] - gGapE;
    if(testog >= testeg)
      ix[cursor] = testog;
    else
      ix[cursor] = testeg;
    m[cursor] = match - (gEndGapO + (xpos - 1) * gEndGapE);
    iy[cursor] = -gEndGapO - xpos * gEndGapE - gGapO;
  }
  iy[cursor] -= gEndGapO;
  iy[cursor] += gGapO;
  xpos = 1;
  // now construct match, ix, and iy matrices
  seq2pos = 0;
  while (xpos != gLen2) {
    ypos = 1;
    seq2pos++;
    // coordinates of the cells being processed
    cursorp = xpos-1;
    cursor = xpos++;
    while (ypos < gLen1) {
      // get match for current xpos/ypos
      if (weight_pos_hook != NULL)
        match = (*weight_pos_hook)(ypos++,seq2pos);
      else
        match = (*weight_symb_hook)(gSeq1[ypos++],gSeq2[seq2pos]);
      cursor += gLen2;
      // match matrix calculations
      mp = m[cursorp];
      ixp = ix[cursorp];
      iyp = iy[cursorp];
      if(mp > ixp && mp > iyp)
        m[cursor] = mp+match;
      else if(ixp > iyp)
        m[cursor] = ixp+match;
      else
        m[cursor] = iyp+match;
      // iy matrix calculations
      if(xpos==gLen2) {
        testog = m[++cursorp] - gEndGapO;
        testeg = iy[cursorp] - gEndGapE;
      }
      else {
        testog = m[++cursorp];
        if (testog < ix[cursorp])
          testog = ix[cursorp];
        testog -= gGapO;
        testeg = iy[cursorp] - gGapE;
      }
      if(testog > testeg)
        iy[cursor] = testog;
      else
        iy[cursor] = testeg;
      cursorp += gLen2;
      // ix matrix calculations
      if (ypos == gLen1) {
        testog = m[--cursorp] - gEndGapO;
        testeg = ix[cursorp] - gEndGapE;
      }
      else {
        testog = m[--cursorp];
        if (testog<iy[cursorp])
          testog = iy[cursorp];
        testog -= gGapO;
        testeg = ix[cursorp] - gGapE;
      }
      if(testog > testeg )
        ix[cursor] = testog;
      else
        ix[cursor] = testeg;
    }
  }
  score = getScoreNW (ix,iy,m,start1,start2);
  xpos = *start2;
  ypos = *start1;
  /* In the following loop the three matrices (m, ix, iy) are traced back
     and path/alignment decision/selection is made.
     0 means match: go up and left in the matrix
     1 means: go left in the matrix, i.e. gap in the first sequence(seq a)
     2 means: go up in the matrix, i.e. gap in the second sequence(seq b)
  */
  cursorp=0;
  cursor=1;
  while (xpos>=0 && ypos>=0) {
    cursor = ypos*gLen2+xpos;
    mp = m[cursor];
    if (cursorp == LEFT &&
        E_FPEQ ((ypos==0 || (ypos==gLen1-1) ? gEndGapE : gGapE),
                (ix[cursor] - ix[cursor+1]),U_FEPS)) {
      compass[cursor] = LEFT;
      xpos--;
    }
    else if (cursorp == DOWN &&
             E_FPEQ ((xpos==0 || (xpos==gLen2-1) ? gEndGapE : gGapE),
                     (iy[cursor] - iy[cursor+gLen2]),U_FEPS)) {
      compass[cursor] = DOWN;
      ypos--;
    }
    else if(mp >= ix[cursor] && mp>= iy[cursor]) {
      if(cursorp == LEFT && E_FPEQ (mp,ix[cursor],U_FEPS)) {
        compass[cursor] = LEFT;
        xpos--;
      }
      else if(cursorp == DOWN && E_FPEQ (mp,iy[cursor],U_FEPS)) {
        compass[cursor] = DOWN;
        ypos--;
      }
      else {
        compass[cursor] = 0;
        ypos--;
        xpos--;
      }
    }
    else if (ix[cursor] >= iy[cursor] && xpos>-1) {
      compass[cursor] = LEFT;
      xpos--;
    }
    else if (ypos>-1) {
      compass[cursor] = DOWN;
      ypos--;
    }
    else
      die ("something is seriously wrong in the traceback algorithm");
    cursorp = compass[cursor];
  }
  return score;
}

static float pathCalcSW (float *path,int *compass) {
  // from EMBOSS embAlignPathCalcSW
  float ret;
  long xpos;
  long ypos;
  long i;
  long j;
  double match;
  double mscore;
  double result;
  double fnew;
  static double *maxa = NULL;
  double bx;

  ret= -FLT_MAX;
  // create stores for the maximum values in a row or column
  if (maxa != NULL)
    hlr_free (maxa);
  maxa = (double *)hlr_calloc (gLen1,sizeof (double));
  // first initialise the first column and row
  for (i=0;i<gLen1;i++) {
    if (weight_pos_hook != NULL)
      result = (*weight_pos_hook)(i,0);
    else
      result = (*weight_symb_hook)(gSeq1[i],gSeq2[0]);
    fnew = i==0 ? 0.0 :
      path[(i-1)*gLen2] - (compass[(i-1)*gLen2] == DOWN ?
                           gGapE : gGapO);
    if (result > fnew && result > 0) {
      path[i*gLen2] = (float)result;
      compass[i*gLen2] = 0;
    }
    else if (fnew>0) {
      path[i*gLen2] = (float)fnew;
      compass[i*gLen2] = DOWN;
    }
    else {
      path[i*gLen2] = 0.0;
      compass[i*gLen2] = 0;
    }
    maxa[i] = i==0 ? path[i*gLen2]-gGapO :
      path[i*gLen2] - (compass[(i-1)*gLen2]==DOWN ? gGapE : gGapO);
  }
  for (j=0;j<gLen2;j++) {
    if (weight_pos_hook != NULL)
      result = (*weight_pos_hook)(0,j);
    else
      result = (*weight_symb_hook)(gSeq1[0],gSeq2[j]);
    fnew = j==0 ? 0. :
      path[j-1] -(compass[j-1]==LEFT ? gGapE : gGapO);
    if (result > fnew && result > 0) {
      path[j] = (float) result;
      compass[j] = 0;
    }
    else if (fnew > 0) {
      path[j] = (float) fnew;
      compass[j] = LEFT;
    }
    else {
      path[j] = 0.;
      compass[j] = 0;
    }
  }
  // xpos and ypos are the diagonal steps so start at 1
  xpos = 1;
  while (xpos != gLen2) {
    ypos  = 1;
    bx = path[xpos] - gGapO - gGapE;
    while (ypos < gLen1) {
      // get match for current xpos/ypos
      if (weight_pos_hook != NULL)
        match = (*weight_pos_hook)(ypos,xpos);
      else
        match = (*weight_symb_hook)(gSeq1[ypos],gSeq2[xpos]);
      // get diag score
      mscore = path[(ypos-1)*gLen2+xpos-1] + match;
      // set compass to diagonal value 0
      compass[ypos*gLen2+xpos] = 0;
      path[ypos*gLen2+xpos] = (float)mscore;
      // Now parade back along X axis
      maxa[ypos] -= gGapE;
      fnew=path[(ypos)*gLen2+xpos-1];
      fnew-=gGapO;
      if(fnew > maxa[ypos])
        maxa[ypos] = fnew;
      if (maxa[ypos] > mscore) {
        mscore = maxa[ypos];
        path[ypos*gLen2+xpos] = (float)mscore;
        compass[ypos*gLen2+xpos] = LEFT; // Score comes from left
      }
      // and then bimble down Y axis
      bx -= gGapE;
      fnew = path[(ypos-1)*gLen2+xpos];
      fnew-=gGapO;
      if(fnew > bx)
        bx = fnew;
      if(bx > mscore) {
        mscore = bx;
        path[ypos*gLen2+xpos] = (float)mscore;
        compass[ypos*gLen2+xpos] = DOWN; // Score comes from bottom
      }
      if (mscore > ret)
        ret = (float)mscore;
      result = path[ypos*gLen2+xpos];
      if (result < 0.0)
        path[ypos*gLen2+xpos] = 0.0;
      ypos++;
    }
    ++xpos;
  }
  return ret;
}

static void rev (char *s) {
  char *p,*q;
  char c;

  p = s;
  q = s + strlen (s) -1;
  while (p < q) {
    c = *p;
    *p = *q;
    *q = c;
    p++;
    q--;
  }
}

static void walkSWMatrix (float *path,int *compass,
                          int *start1,int *start2) {
  // from EMBOSS embAlignWalkSWMatrix
  long i;
  long j;
  long k;
  long gapcnt;
  double pmax;
  double score;
  double bimble;
  long ix;
  long iy;
  long xpos = 0;
  long ypos = 0;
  int ic;
  double errbounds;

  stringCreateClear (gAlg1,100);
  stringCreateClear (gAlg2,100);
  // errbounds = gapextend;
  errbounds = (double)0.01;
  // get maximum path score and save position
  pmax = -FLT_MAX;
  k = (long)gLen1 * (long)gLen2 - 1;
  for (i=gLen1-1;i>=0;i--) {
    for (j=gLen2-1;j>=0;j--) {
      if ((path[k--] > pmax) || E_FPEQ (path[k+1],pmax,U_FEPS)) {
        pmax = path[k+1];
        xpos = j;
        ypos = i;
      }
    }
  }
  while (xpos >= 0 && ypos >= 0) {
    if (!compass[ypos*gLen2+xpos]) { // diagonal
      stringCatChar (gAlg1,gSeq1[ypos--]);
      stringCatChar (gAlg2,gSeq2[xpos--]);
      if (ypos >= 0 && xpos >=0 && path[ypos*gLen2+xpos] <= 0.0)
        break;
      continue;
    }
    else if (compass[ypos*gLen2+xpos] == LEFT) { // Left, gap(s) in vertical
      score = path[ypos*gLen2+xpos];
      gapcnt = 0;
      ix = xpos-1;
      while(1) {
        bimble = path[ypos*gLen2+ix] - gGapO - (gapcnt*gGapE);
        if (!ix || fabs ((double)score-(double)bimble) < errbounds)
          break;
        ix--;
        gapcnt++;
      }
      if (bimble <= 0.0)
        break;
      for (ic=0;ic<=gapcnt;ic++) {
        stringCatChar (gAlg1,'.');
        stringCatChar (gAlg2,gSeq2[xpos--]);
      }
      continue;
    }
    else if (compass[ypos*gLen2+xpos]==DOWN) { // Down, gap(s) in horizontal
      score = path[ypos*gLen2+xpos];
      gapcnt = 0;
      iy = ypos-1;
      while (1) {
        bimble = path[iy*gLen2+xpos] - gGapO - (gapcnt*gGapE);
        if (!iy || fabs ((double)score - (double)bimble) < errbounds)
          break;
        iy--;
        if (iy < 0)
          die ("SW: Error walking down");
        gapcnt++;
      }
      if (bimble <= 0.0)
        break;
      for (ic=0;ic<=gapcnt;ic++) {
        stringCatChar (gAlg1,gSeq1[ypos--]);
        stringCatChar (gAlg2,'.');
      }
      continue;
    }
    else
      die ("Walk Error in SW");
  }
  *start1 = (int)(ypos + 1); // Potential lossy cast
  *start2 = (int)(xpos + 1); // Potential lossy cast
  rev (string (gAlg1));
  rev (string (gAlg2));
}

static void walkNWMatrixUsingCompass (int *start1,int *start2,int *compass) {
  // from EMBOSS embAlignWalkNWMatrixUsingCompass
  int xpos = *start2;
  int ypos = *start1;
  int i;
  int j;
  unsigned int cursor;

  stringCreateClear (gAlg1,100);
  stringCreateClear (gAlg2,100);
  for (i=gLen2-1;i>xpos;i--) {
    stringCatChar (gAlg1,'.');
    stringCatChar (gAlg2,gSeq2[i]);
  }
  for (j=gLen1-1;j>ypos;j--) {
    stringCatChar (gAlg1,gSeq1[j]);
    stringCatChar (gAlg2,'.');
  }
  while (xpos >= 0 && ypos >= 0) {
    cursor = ypos * gLen2 + xpos;
    if (!compass[cursor]) { // diagonal
      stringCatChar (gAlg1,gSeq1[ypos--]);
      stringCatChar (gAlg2,gSeq2[xpos--]);
      continue;
    }
    else if(compass[cursor] == LEFT) { // Left, gap(s) in vertical
      stringCatChar (gAlg1,'.');
      stringCatChar (gAlg2,gSeq2[xpos--]);
      continue;
    }
    else if(compass[cursor] == DOWN) { // Down, gap(s) in horizontal
      stringCatChar (gAlg1,gSeq1[ypos--]);
      stringCatChar (gAlg2,'.');
      continue;
    }
    else
      die ("Walk Error in NW");
  }
  for (;xpos>=0;xpos--) {
   stringCatChar (gAlg1,'.');
   stringCatChar (gAlg2,gSeq2[xpos]);
  }
  for (;ypos>=0;ypos--) {
    stringCatChar (gAlg1,gSeq1[ypos]);
    stringCatChar (gAlg2,'.');
  }
  *start2 = xpos+1;
  *start1 = ypos+1;
  rev (string (gAlg1));
  rev (string (gAlg2));
}

static float weightNuc (char c1,char c2) {
  if (c1 == c2)
    return 5.0;
  if (c1 == 'n' || c2 == 'n')
    return -2.0;
  return -4.0;
}

static float weightPro (char c1,char c2) {
  return blosum62 (c1,c2);
}

void algutil_register_weight_symb (float (*f)(char c1,char c2)) {
  /**
     Registers a function to be called when the weight of a match
     between nucs or amino acids needs to be known.<br>
     Can be called after algutil_setSeqs to override default matrix
     @param[in] f - the function (see weightNuc or weightPro, the default
                    functions
  */
  weight_symb_hook = f;
}

void algutil_register_weight_pos (float (*f)(int p1,int p2)) {
  /**
     Registers a function to be called when the weight of a match
     between two positions needs to be known.<br>
     Can be called after algutil_setSeqs to register position specific weighting
     @param[in] f - the function (no default functions)
  */
  weight_pos_hook = f;
}

void algutil_setSeqs (char *name1,char *seq1,char *name2,char *seq2,int isNuc) {
  /**
     Sets the 2 input sequnces and their type
     @param[in] name1,name2 - the sequence names
     @param[in] seq1,seq2 - the sequences
     @param[in] isNuc - 1 if nucelotide, 0 if protein
  */
  strReplace (&gSeq1,seq1);
  strReplace (&gSeq2,seq2);
  gLen1 = strlen (seq1);
  gLen2 = strlen (seq2);
  gIsNuc = isNuc;
  // register the following default functions
  if (gIsNuc) {
    algutil_register_weight_symb (weightNuc);
    tolowerStr (gSeq1);
    tolowerStr (gSeq2);
  }
  else {
    algutil_register_weight_symb (weightPro);
    toupperStr (gSeq1);
    toupperStr (gSeq2);
  }
  weight_pos_hook = NULL;
}

float algutil_run (int mode,float go,float ge,
                   int doEndWeight,float ego,float ege) {
  /**
     Run the alignment
     @param[in] mode - either ALGUTIL_GLOBAL or ALGUTIL_LOCAL
     @param[in] go,ge - gap opening and extension penalties
     @param[in] doEndWeight - whether to penalize end weights
     @param[in] ego,ege - end opening and extending penalties, only used if
                          doEndWeight is 1
     @return the alignment score
  */
  int start1,start2;
  static int *compass = NULL;
  static int prevDim = 0;
  float score = 0.0;

  if (gIsNuc == -1)
    die ("algutil_run: call first algutil_setSeqs with boolean isNuc");
  gMode = mode;
  gGapO = go;
  gGapE = ge;
  gDoEndWeight = doEndWeight;
  gEndGapO = ego;
  gEndGapE = ege;
  if (prevDim < gLen1*gLen2) {
    prevDim = gLen1*gLen2;
    if (compass != NULL)
      hlr_free (compass);
    compass = (int *)hlr_calloc (prevDim,sizeof (int));
  }
  if (gMode == ALGUTIL_GLOBAL) {
    score = pathCalcWithEndGapPenalties (&start1,&start2,
                                         compass);
    walkNWMatrixUsingCompass (&start1,&start2,
                              compass);
  }
  else {
    static int prevDimPath = 0;
    static float *path = NULL;

    if (prevDimPath < gLen1*gLen2) {
      prevDimPath = gLen1*gLen2;
      if (path != NULL)
        hlr_free (path);
      path = (float *)hlr_calloc (prevDimPath,sizeof (float));
    }
    score = pathCalcSW (path,compass);
    walkSWMatrix (path,compass,
                  &start1,&start2);
  }
  return score;
}

void algutil_getAlg (char **alg1,char **alg2) {
  /**
     Retrieves the aligned sequences
     @param[in] alg1,alg2 - pointers to the sequences, NULL if not interested
     @param[out] alg1,alg2 - the aligned sequences
  */
  if (alg1 != NULL)
    *alg1 = string (gAlg1);
  if (alg2 != NULL)
    *alg2 = string (gAlg2);
}
