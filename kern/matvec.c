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
/** @file matvec.c
    @brief Purpose: basic routines for matrix and vector operations.
    Module prefix mv_
*/
#include "log.h"
#include "hlrmisc.h"
#include "matvec.h"
/*
  The following conventions apply to the functions
  in this module

  - function names: are a concatenation of
     * the basic data type: d double, f float, i integer
     * the name of the operation (mult,plus,...)
     * an abbreviation for each of the input arguments:
       M matrix, V vector, D diagonal matrix represented
       via a vector

  - the result of the function is found in the first variable
    that has to be allocated by the calling routine.

  - each of the non-scalar input arguments is followed by its
    dimensions.

  example: mv_dmultMV(double *y,double **A,int nA,int mA,double *x,int nx)
     mv_  -- module matvec
       d  -- function is working on double precision numbers
     mult  -- multiply a matrix with a vector
        M  -- first input argument is a matrix
        V  -- second input argument is a vector
     remark: the type and dimension of the result is implied by function
             type and the arguments
*/
float **mv_matrixF (int nr,int nc) {
  /**
     Allocates a matrix of float elements
     @param[in] nr - number of rows
     @param[in] nc - number of columns
     @return the matrix
  */
  float **m;
  int i;

  m = (float **)hlr_malloc (nr * sizeof (float *));
  m[0] = (float *)hlr_malloc (nr * nc * sizeof (float));
  for (i=1;i<nr;i++)
    m[i] = m[i-1] + nc;
  return m;
}

double **mv_matrixD (int nr,int nc) {
  /**
     Allocates a matrix of double elements
     @param[in] nr - number of rows
     @param[in] nc - number of columns
     @return the matrix
  */
  double **m;
  int i;

  m = (double **)hlr_malloc (nr * sizeof (double *));
  m[0] = (double *)hlr_malloc (nr * nc * sizeof (double));
  for (i=1;i<nr;i++)
    m[i] = m[i-1] + nc;
  return m;
}

int **mv_matrixI (int nr,int nc) {
  /**
     Allocates a matrix of int elements
     @param[in] nr - number of rows
     @param[in] nc - number of columns
     @return the matrix
  */
  int **m;
  int i;

  m = (int **)hlr_malloc (nr * sizeof (int *));
  m[0] = (int *)hlr_malloc (nr * nc * sizeof (int));
  for (i=1;i<nr;i++)
    m[i] = m[i-1] + nc;
  return m;
}

unsigned char **mv_matrixB (int nr,int nc) {
  /**
     Allocates a matrix of unsigned char elements
     @param[in] nr - number of rows
     @param[in] nc - number of columns
     @return the matrix
  */
  unsigned char **m;
  int i;

  m = (unsigned char **)hlr_malloc (nr * sizeof (unsigned char *));
  m[0] = (unsigned char *)hlr_malloc (nr * nc * sizeof (unsigned char));
  for (i=1;i<nr;i++)
    m[i] = m[i-1] + nc;
  return m;
}

float *mv_vectorF (int ne) {
  /**
     Allocates a vector of float elements
     @param[in] ne - number of elements
     @return the vector
  */
  float *v;

  v = (float *)hlr_malloc (ne * sizeof (float));
  return v;
}

double *mv_vectorD (int ne) {
  /**
     Allocates a vector of double elements
     @param[in] ne - number of elements
     @return the vector
  */
  double *v;

  v = (double *)hlr_malloc (ne * sizeof (double));
  return v;
}

int *mv_vectorI (int ne) {
  /**
     Allocates a vector of int elements
     @param[in] ne - number of elements
     @return the vector
  */
  int *v;

  v = (int *)hlr_malloc (ne * sizeof (int));
  return v;
}

unsigned char *mv_vectorB (int ne) {
  /**
     Allocates a vector of unsigned char elements
     @param[in] ne - number of elements
     @return the vector
  */
  unsigned char *v;

  v = (unsigned char *)hlr_malloc (ne * sizeof (unsigned char));
  return v;
}

void mv_freeMatrixF (float **m) {
  /**
     Frees a float matrix
     @param[in] m - the matrix
  */
  hlr_free (m[0]);
  hlr_free (m);
}

void mv_freeMatrixD (double **m) {
  /**
     Frees a double matrix
     @param[in] m - the matrix
  */
  hlr_free (m[0]);
  hlr_free (m);
}

void mv_freeMatrixI (int **m) {
  /**
     Frees an int matrix
     @param[in] m - the matrix
  */
  hlr_free (m[0]);
  hlr_free (m);
}

void mv_freeMatrixB (unsigned char **m) {
  /**
     Frees an unsigned char matrix
     @param[in] m - the matrix
  */
  hlr_free (m[0]);
  hlr_free (m);
}

void mv_freeVectorF (float *v) {
  /**
     Frees a float vector
     @param[in] v - the vector
  */
  hlr_free (v);
}

void mv_freeVectorD (double *v) {
  /**
     Frees a double vector
     @param[in] v - the vector
  */
  hlr_free (v);
}

void mv_freeVectorI (int *v) {
  /**
     Frees an int vector
     @param[in] v - the vector
  */
  hlr_free (v);
}

void mv_freeVectorB (unsigned char *v) {
  /**
     Frees an unsigned char vector
     @param[in] v - the vector
  */
  hlr_free (v);
}

void mv_dmultMM (double **C,double **A,int nA,int mA,double **B,int nB,int mB) {
  /**
     Multiply matrix A with B
     @param[in] A - first matrix
     @param[in] nA - number of rows of A
     @param[in] mA - number of columns of A
     @param[in] B - second matrix
     @param[in] nB - number of rows of B
     @param[in] mB - number of columns of B
     @param[out] C - matrix of dimension nA,mB
  */
  int h,i,j;
  double sum;

  if (nB != mA)
    die ("Can not multiply matrix of dimension (%iX%i) with one of dimension (%iX%i).",
         nA,mA,nB,mB);
  for (h=0;h<nA;h++) { // row of A
    for (i=0;i<mB;i++){ // column of B
      sum=0.;
      for (j=0;j<mA;j++) {
        sum += A[h][j]*B[j][i];
      }
      C[h][i] = sum;
    }
  }
}

void mv_fmultMM (float **C,float **A,int nA,int mA,float **B,int nB,int mB) {
  /**
     Multiply matrix A with B
     @param[in] A - first matrix
     @param[in] nA - number of rows of A
     @param[in] mA - number of columns of A
     @param[in] B - second matrix
     @param[in] nB - number of rows of B
     @param[in] mB - number of columns of B
     @param[out] C - matrix of dimension nA,mB
  */
  int h,i,j;
  float sum;

  if (nB != mA)
    die ("Can not multiply matrix of dimension (%iX%i) with one of dimension (%iX%i).",
         nA,mA,nB,mB);
  for (h=0;h<nA;h++) { // row of A
    for (i=0;i<mB;i++) { // column of B
      sum=0.f;
      for (j=0;j<mA;j++) {
        sum += A[h][j]*B[j][i];
      }
      C[h][i] = sum;
    }
  }
}

void mv_imultMM (int **C,int **A,int nA,int mA,int **B,int nB,int mB) {
  /**
     Multiply matrix A with B
     @param[in] A - first matrix
     @param[in] nA - number of rows of A
     @param[in] mA - number of columns of A
     @param[in] B - second matrix
     @param[in] nB - number of rows of B
     @param[in] mB - number of columns of B
     @param[out] C - matrix of dimension nA,mB
  */
  int h,i,j;
  int sum;

  if (nB != mA)
    die ("Can not multiply matrix of dimension (%iX%i) with one of dimension (%iX%i).",
         nA,mA,nB,mB);
  for (h=0;h<nA;h++) { // row of A
    for (i=0;i<mB;i++) { // column of B
      sum=0;
      for (j=0;j<mA;j++) {
        sum += A[h][j]*B[j][i];
      }
      C[h][i] = sum;
    }
  }
}

void mv_dmultDM (double **C,double *A,int nA,double **B,int nB,int mB) {
  /**
     Multiply diagonal matrix A with matrix B
     @param[in] A - vector that represents the diagonal matrix
     @param[in] nA - dimension of A
     @param[in] B - second matrix
     @param[in] nB - number of rows of B
     @param[in] mB - number of columns of B
     @param[out] C - matrix of dimension nA,mB
  */
  int h,i;

  if (nB != nA)
    die ("Can not multiply diagonal matrix of dimension (%iX%i) with one of dimension (%iX%i).",
         nA,nA,nB,mB);
  for (h=0;h<nA;h++) { // row of A
    for (i=0;i<mB;i++) { // column of B
      C[h][i] = A[h]*B[h][i];
    }
  }
}

void mv_imultDM (int **C,int *A,int nA,int **B,int nB,int mB) {
  /**
     Multiply diagonal matrix A with matrix B
     @param[in] A - vector that represents the diagonal matrix
     @param[in] nA - dimension of A
     @param[in] B - second matrix
     @param[in] nB - number of rows of B
     @param[in] mB - number of columns of B
     @param[out] C - matrix of dimension nA,mB
  */
  int h,i;

  if (nB != nA)
    die ("Can not multiply diagonal matrix of dimension (%iX%i) with one of dimension (%iX%i).",
         nA,nA,nB,mB);
  for (h=0;h<nA;h++) { // row of A
    for (i=0;i<mB;i++) { // column of B
      C[h][i] = A[h]*B[h][i];
    }
  }
}

void mv_fmultDM (float **C,float *A,int nA,float **B,int nB,int mB) {
  /**
     Multiply diagonal matrix A with matrix B
     @param[in] A - vector that represents the diagonal matrix
     @param[in] nA - dimension of A
     @param[in] B - second matrix
     @param[in] nB - number of rows of B
     @param[in] mB - number of columns of B
     @param[out] C - matrix of dimension nA,mB
  */
  int h,i;

  if (nB != nA)
    die ("Can not multiply diagonal matrix of dimension (%iX%i) with one of dimension (%iX%i).",
         nA,nA,nB,mB);
  for (h=0;h<nA;h++) { // row of A
    for (i=0;i<mB;i++) { // column of B
      C[h][i] = A[h]*B[h][i];
    }
  }
}
