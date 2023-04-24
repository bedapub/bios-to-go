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
/** @file statistics.h
    @brief Module containing statistical algorithms from various sources.
    Module prefix stat_
*/
#ifndef STATISTICS_H
#define STATISTICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <float.h>
#include "array.h"

extern void stat_pca (int nObs,int nVar,double **data,
                      double *eigVal,double **eigVec);
extern double stat_meanCorrect (double x[],int num);
extern void stat_normalize (double x[],int num);
extern double stat_mean (double x[],int num);
extern double stat_median (double x[],int num);
extern double stat_median_quart (double x[],int num,double *q25,double *q75);
extern double stat_median_abs_dev (double x[],int num);
extern double stat_variance (double x[],int num);
extern double stat_stddev (double x[],int num);
extern double stat_skew (double x[],int num);
extern double stat_kurt (double x[],int num);
extern double stat_percentile (double x[],int num,double p);
extern double stat_ma68 (double x[],int num);
extern double stat_span (double x[],int num);
extern double stat_chiSquare (int n11,int n12,int n21,int n22);
extern double stat_fisherScore (double x[],int numx,double y[],int numy);
extern double stat_robustFisherScore (double x[],int numx,double y[],int numy);
extern double stat_signalToNoiseRatio (double x[],int numx,double y[],int numy);

/// structure for values of a condition
typedef struct {
  char *name; //!< name of condition
  Array samples; //!< double values of condition
}StatCond;

extern void stat_setConds (Array conds /* of StatCond */);
extern double stat_getF (void);
extern double stat_getH (void);
extern double stat_getPvalU (void);
extern double stat_getU (void);
extern double stat_getPvalSignedRank (void);

/// significance 95%
#define STAT_SIG_95   0
/// significance 99%
#define STAT_SIG_99   1
/// significance 99.9%
#define STAT_SIG_99_9 2

/// indicator for invalid value
#define STAT_INVALID FLT_MAX // to fit into double or float

extern int stat_dixon (double x[],int num,int sig);
extern int stat_grubbs (double x[],int num,int sig);
extern int stat_nalimov (double x[],int num,int sig);
extern int stat_nalimov1 (double x[],int num,int sig);
extern int stat_x84 (double x[],int num,int sig);

extern void stat_randomGaussInit (double mean,double stddev,int doRandomize);
extern double stat_randomGauss (void);

extern void stat_bootstrap (double *val,int count,int *seed,int rep,
                            double *mean,double *median,double quantile[]);

extern void stat_qq (double **x,int n,int dim,double *values);
extern void stat_qq_fixed (double **x,int n,int dim,double *values);

extern double stat_gini (double x[],int num);

extern double stat_phi (double x);
extern double stat_phiQuantile (double beta);
extern double stat_studentTQuantile (double beta,int k);

extern double fisher_exact_test (int x11,int x12,int x21,int x22,
                                 double *sless,double *slarg,double *twotail);

#ifdef __cplusplus
}
#endif

#endif
