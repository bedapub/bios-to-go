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
/** @file graphalgo.c
    @brief Module containing algorithms for graph handling.
    Module prefix gral_
*/
#include "hlrmisc.h"
#include "matvec.h"
#include "graphalgo.h"

/*
sub-module shortest path
interface synopsis (prefix gral_ omitted for readability):
  gral_spInit(number_of_rows_and_cols)
  gral_sp(i,j) = val
  val=sp(i,j)
  gral_spCompute()
  gral_spDeInit()
implementation:
  for maximum runtime efficieny this module
  is a singleton object. Be careful when nesting
  routines that use this module
*/

/// distance matrix[0..n-1,0..n-1]; GRAL_NC = no connection
static float **gralDistMat = NULL;
/// number of rows and columns in gralDistMat
static int gralDistN;

void gral_spInit (int n) {
  /**
     Create initialize distance matrix with 'no connection'.<br>
     Postcondition: gral_sp(i,j)=val, gral_spCompute() can be called
     @param[in] n - number of rows and columns
  */
  int i;
  int j;

  gralDistN = n;
  gralDistMat = mv_fallocM (n,n);
  for (i=0;i<n;i++)
    for (j=0;j<n;j++)
      gralDistMat[i][j] = GRAL_NC;
}

float gral_sp (int i,int j) {
  /**
     Return the requested matrix element
     @param[in] i,j - the coordinates
     @return the matrix element
  */
  return gralDistMat[i][j];
}

void gral_spSet (int i,int j,float v) {
  /**
     Set the requested matrix element
     @param[in] i,j - the coordinates
     @param[in] v - the value
  */
  gralDistMat[i][j] = v;
}

void gral_spDeInit (void) {
  /**
     Precondition: gral_spInit()
  */
  mv_ffreeM (gralDistMat);
  gralDistMat = NULL;
}

void gral_spCompute (void) {
  /**
     Compute shortest distances between all nodes within a graph
     using Floyd's algorithm. Runtime grows proportional to N^3,
     where N is the number of nodes.<br>
     Precondition: gral_spInit(), gral_sp(i,j)=val; graph contains no cycles
                   with non-positive sum of distances.<br>
     Postcondition: gral_sp(i,j) returns shortest distance between i and j
  */
  /*
  Floyd, R.W., "Algorithm 97: shortest path",
  Comm ACM 5:6, 345, 1962
  Volker Turau, Algorithmische Graphentheorie, Addison-Wesely, Bonn,
  Germany, 1996
  Robert Sedgewick, Algorithmes en langage C, Addison-Wesely, Europe,
  2001
  */
  int i;
  int j;
  int k;
  float t1,t2,t3,t4;
  for (i=0;i<gralDistN;i++) {
    for (j=0;j<gralDistN;j++) {
      t1 = gral_sp (j,i);
      if (t1 != GRAL_NC) {
        for (k=0;k<gralDistN;k++) {
          t2 = gral_sp (i,k);
          if (t2 != GRAL_NC) {
            t3 = gral_sp (j,k);
            t4 = t1+t2;
            if ( (t3 == GRAL_NC)  || (t4< t3))
              gralDistMat[j][k] = t4;
          }
        }
      }
    }
  }
}

int gral_spNGet (void) {
  /**
     Returns the number of rows and clumns in the distance matrix
     @return dimension of matrix
  */
  return gralDistN;
}

float gral_spMaxDistGet (int *imax,int *jmax) {
  /**
     How big is the largest distance, and between which two nodes is it?
     @param[in] imax,jmax - pointers to deposit row/column indices
     @return largest distance value in the distance matrix
     @param[out] *imax,*jmax - location of the highest value;
                 if the highest value occurs in several places,
                 the one with the lowest row number, and within that
                 row the one with the lowest column number is returned
  */
  int i;
  int j;
  float t1 = 0;
  for (i=0;i<gralDistN;i++) {
    for (j=0;j<gralDistN;j++) {
      if (gral_sp (i,j) > t1) {
        *imax = i;
        *jmax = j;
        t1 = gral_sp (i,j);
      }
    }
  }
  return t1;
}

// general functions for matrices

void gral_matDiagonalSet (float val) {
  /**
     Sets the diagonal values of the matrix to val.<br>
     Precondition: gral_spInit().<br>
     Postcondition: gral_sp(i,i) are set to val
     */
  int i;
  for (i=0;i<gralDistN;i++)
    gralDistMat[i][i] = val;
}

void gral_matUndirected (void) {
  /**
     Makes the matrix undirected.<br>
     Precondition: gral_spInit()<Br>
     Postcondition : matrix (gral_sp(i,j)) is modified
                     if needed, should be used before gral_spCompute()<br>
     Note: this does not necessarily make the matrix symmetric
     */
  int i,j;
  float t1;

  for (i=0;i<gralDistN;i++) {
    for (j=0;j<gralDistN;j++) {
      t1 = gralDistMat[i][j];
      if (t1 != GRAL_NC) {
        if (gralDistMat[j][i] == GRAL_NC)
          gralDistMat[j][i] = t1;
      }
    }
  }
}
