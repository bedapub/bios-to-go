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
/** @file matvec.h
    @brief Purpose: basic routines for matrix and vector operations.
    Module prefix mv_
*/
#ifndef MAXVEC_H
#define MAXVEC_H

#ifdef __cplusplus
extern "C" {
#endif

extern float **mv_matrixF (int nr,int nc);
/// synonym
#define mv_fallocM(n,m) mv_matrixF(n,m)
extern double **mv_matrixD (int nr,int nc);
/// synonym
#define mv_dallocM(n,m) mv_matrixD(n,m)
extern int **mv_matrixI (int nr,int nc);
/// synonym
#define mv_iallocM(n,m) mv_matrixI(n,m)
extern unsigned char **mv_matrixB (int nr,int nc);
/// synonym
#define mv_ballocM(n,m) mv_matrixB(n,m)

extern float *mv_vectorF (int ne);
/// synonym
#define mv_fallocV(n) mv_vectorF(n)
extern double *mv_vectorD (int ne);
/// synonym
#define mv_dallocV(n) mv_vectorD(n)
extern int *mv_vectorI (int ne);
/// synonym
#define mv_iallocV(n) mv_vectorI(n)
extern unsigned char *mv_vectorB (int ne);
/// synonym
#define mv_ballocV(n) mv_vectorB(n)

extern void mv_freeMatrixF (float **m);
/// synonym
#define mv_ffreeM(M) mv_freeMatrixF(M)
extern void mv_freeMatrixD (double **m);
/// synonym
#define mv_dfreeM(M) mv_freeMatrixD(M)
extern void mv_freeMatrixI (int **m);
/// synonym
#define mv_ifreeM(M) mv_freeMatrixI(M)
extern void mv_freeMatrixB (unsigned char **m);
/// synonym
#define mv_bfreeM(M) mv_freeMatrixB(M)

extern void mv_freeVectorF (float *v);
/// synonym
#define mv_ffreeV(V) mv_freeVectorF(V)
extern void mv_freeVectorD (double *v);
/// synonym
#define mv_dfreeV(V) mv_freeVectorD(V)
extern void mv_freeVectorI (int *v);
/// synonym
#define mv_ifreeV(V) mv_freeVectorI(V)
extern void mv_freeVectorB (unsigned char *v);
/// synonym
#define mv_bfreeV(V) mv_freeVectorB(V)

void mv_dmultMM (double **C,double **A,int nA,int mA,double **B,int nB,int mB);
void mv_fmultMM (float **C,float **A,int nA,int mA,float **B,int nB,int mB);
void mv_imultMM (int **C,int **A,int nA,int mA,int **B,int nB,int mB);
void mv_dmultDM (double **C,double *A,int nA,double **B,int nB,int mB);
void mv_fmultDM (float **C,float *A,int nA,float **B,int nB,int mB);
void mv_imultDM (int **C,int *A,int nA,int **B,int nB,int mB);

#ifdef __cplusplus
}
#endif

#endif
