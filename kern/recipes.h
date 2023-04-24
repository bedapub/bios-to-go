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
/** @file recipes.h
    @brief Routines from Numerical Recipes,
    The art of scientific computing,
    Press, W.H., Flannery, B.P., Teukolsky, S.A., and Vetterling, W.T.,
    Cambridge University Press 1986.
    Algorithms have been adapted to real C (arrays start at 0).
    Module prefix rcp_
*/
#ifndef RECIPES_H
#define RECIPES_H

#ifdef __cplusplus
extern "C" {
#endif

extern double rcp_ran3 (int *idum);
extern int rcp_irbit1 (unsigned long *iseed);
extern void rcp_tqli (double d[],double e[],int n,double **z);
extern void rcp_tred2 (double **a,int n,double d[],double e[]);
extern double rcp_betai (double a,double b,double x);
extern double rcp_betacf (double a,double b,double x);
extern void rcp_gser (double *gamser,double a,double x,double *gln);
extern void rcp_gcf (double *gammcf,double a,double x,double *gln);
extern double rcp_gammq (double a,double x);
extern double rcp_gammln (double xx);
extern void rcp_ttest (double x1[],int num1,double x2[],int num2,
                       double *t,double *prob);
extern void rcp_ttest_m (double ave1,double ave2,double sd1,double sd2,
                         int num1,int num2,double *t,double *prob);
extern void rcp_ttest_welch (double x1[],int num1,double x2[],int num2,
                             double *t,double *prob);
extern void rcp_ttest_welch_m (double ave1,double ave2,double sd1,double sd2,
                               int num1,int num2,double *t,double *prob);
extern void rcp_ttestPaired (double x1[],double x2[],int num,
                             double *t,double *prob);
extern void rcp_tutest (double x1[],int num1,double x2[],int num2,
                        double *t,double *prob);
extern void rcp_ftest (double x1[],int num1,double x2[],int num2,
                       double *f,double *prob);
extern void rcp_avevar (double x[],int num,double *ave,double *var);
extern void rcp_moment (double x[],int num,
                        double *ave,double *adev,double *sdev,
                        double *var,double *skew,double *curt);
extern void rcp_eigsrt (double *d,double **v,int n);
extern void rcp_pearsn (double *x,double *y,int n,double *r,
                        double *prob,double *z);
extern void rcp_sort2 (int n,double arr[],double brr[]);
extern void rcp_crank (int n,double w[],double *s);
extern double rcp_erfcc (double x);
extern void rcp_spear (double *x,double *y,int n,double *d,double *zd,
                       double *probd,double *rs,double *probrs);
extern void rcp_contTab (int **val,int num1,int num2,double *chisq,double *prob,
                         double *uygx,double *uxgy,double *oddsRatio);
extern double rcp_probks (double alam);
extern void rcp_ks (double x1[],int n1,double x2[],int n2,
                    double *ks,double *prob);
extern void rcp_svdcmp (float **a,int m,int n,float *w,float **v);
extern void rcp_mrqMin (float x[],float y[],float sig[],int ndata,float a[],
                        int ia[],int ma,float **covar,float **alpha,
                        float *chiSq,
                        void (*funcs)(float,float[],float *,float[],int),
                        float *aLambda);
extern void rcp_mrqCof (float x[],float y[],float sig[],int nData,float a[],
                        int ia[],int ma,float **alpha,float beta[],float *chiSq,
                        void (*funcs)(float,float[],float *,float[],int));
extern void rcp_covSrt (float **covar,int ma,int ia[],int mfit);
extern void rcp_gaussJ (float **a,int n,float **b,int m);

#ifdef __cplusplus
}
#endif

#endif
