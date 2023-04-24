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
/** @file statistics.c
    @brief Module containing statistical algorithms from various sources.
    Module prefix stat_
*/
/* Implementation of methods described in

   Elementare Tests zur Beurteilung von Messdaten
   Soforthilfe fuer statistische Tests mit wenigen Messdaten
   von
   Dr. R. Kaiser
   Institut fuer Chromatographie Bad Duerkheim
   und
   Prof. Dr. G. Gottschalk
   Osram-Studiengesellschaft, Muenchen
   Bibliographisches Institut Mannheim/Wien/Zuerich
   B.I.-Wissenschaftsverlag 1972

   and

   Applied multivariate techniques
   Subhash Sharma
   John Wiley & Sons 1996

   and

   Robust statistics
   Frank R. Hampel
   Elvezio M. Ronchetti
   Peter J. Rousseuw
   Werner A. Stahel
   John Wiley & Sons 1986

   and

   Lindgren
   Statistical Theory
   Chapman&Hall 1993
*/

#include <unistd.h>
#include <math.h>
#include "log.h"
#include "array.h"
#include "hlrmisc.h"
#include "matvec.h"
#include "combi.h"
#include "recipes.h"
#include "statistics.h"

static void sortAscending (double x[],int num) {
  if (num > 1)
    qsort (x,num,sizeof (double),
           (int (*)(const void *,const void *))arrayDoublecmp);
}

void stat_pca (int nObs,int nVar,double **data,
               double *eigVal,double **eigVec) {
  /**
     Makes principal component analysis
     @param[in] nObs - number of observations
     @param[in] nVar - number of variables
     @param[in] data - array with nObs rows and nVar columns containing data
                       (will not be destroyed)
     @param[out] eigVal - eigenvalues (memory to be managed by calling routine)
                          (nVar values)
     @param[out] eigVec - eigenvectors (memory to be managed by calling routine)
                          (nVar*nVar values)
  */
  int i,k,n;
  double **dt;
  double *e;
  double totVar,sumEigval;

  // this table is transposed compared to data
  dt = mv_matrixD (nVar,nObs);
  for (i=0;i<nVar;i++)
    for (k=0;k<nObs;k++)
      dt[i][k] = data[k][i];

  totVar = 0.0;
  for (i=0;i<nVar;i++)
    totVar += stat_meanCorrect (dt[i],nObs);

  for (i=0;i<nVar;i++)
    for (k=0;k<nVar;k++)
      eigVec[i][k] = 0.0;
  for (n=0;n<nObs;n++)
    for (i=0;i<nVar;i++)
      for (k=0;k<nVar;k++)
        eigVec[i][k] += dt[i][n] * dt[k][n];
  for (i=0;i<nVar;i++)
    for (k=0;k<nVar;k++)
      eigVec[i][k] /= nObs;
  e = mv_vectorD (nVar);
  rcp_tred2 (eigVec,nVar,eigVal,e);
  rcp_tqli (eigVal,e,nVar,eigVec);
  sumEigval = 0.0;
  for (i=0;i<nVar;i++)
    sumEigval += eigVal[i];
  for (i=0;i<nVar;i++)
    eigVal[i] *= totVar/sumEigval;
  mv_freeMatrixD (dt);
  mv_freeVectorD (e);
  rcp_eigsrt (eigVal,eigVec,nVar);
}

double stat_meanCorrect (double x[],int num) {
  /**
     Calculates mean and variance of the input values and subtracts
     the mean from them.
     @param[in] x - the values
     @param[in] num - how many
     @return the variance
  */
  double mean,var;
  int i;

  if (num <= 0)
    die ("stat_meanCorrect: Invalid number of observations: %d",num);
  mean = stat_mean (x,num);
  var = stat_variance (x,num);
  for (i=0;i<num;i++)
    x[i] = x[i] - mean;
  return var;
}

void stat_normalize (double x[],int num) {
  /**
     Normalizes input values to a mean of zero and a stddev of one.
     @param[in] x - the values
     @param[in] num - how many
  */
  double mean,stddev;
  int i;

  if (num <= 0)
    die ("stat_normalize: Invalid number of observations: %d",num);
  mean = stat_mean (x,num);
  stddev = stat_stddev (x,num);
  for (i=0;i<num;i++)
    x[i] = (x[i] - mean)/stddev;
}

double stat_mean (double x[],int num) {
  /**
     Calculates the mean
     @param[in] x - the values
     @param[in] num - how many
     @return the mean
  */
  double sum;
  int i;

  if (num <= 0)
    die ("stat_mean: Invalid number of observations: %d",num);
  sum = 0.0;
  for (i=0;i<num;i++)
    sum += x[i];
  return sum/num;
}

double stat_median (double x[],int num) {
  /** From numerial recipes but adapted for index starting at 0
      on return, the list x is sorted in ascending order
      @param[in] x - the values
      @param[in] num - how many
      @return the median
  */
  int n2;

  if (num <= 0)
    die ("stat_median: Invalid number of observations: %d",num);
  sortAscending (x,num);
  n2 = num/2;
  if (2 * n2 == num)
    return 0.5 * (x[n2-1] + x[n2]);
  else
    return x[n2];
}

static double mq_sub (double x[],int n,double m) {
  if (m == 0.0)
    return x[n];
  else if (m == 0.25)
    return 0.75 * x[n] + 0.25 * x[n+1];
  else if (m == 0.5)
    return 0.5 * x[n] + 0.5 * x[n+1];
  else if (m == 0.75)
    return 0.25 * x[n] + 0.75 * x[n+1];
  else
    die ("double mq_sub: illegal fraction %f",m);
  return 0.0; // to keep compiler happy
}

double stat_median_quart (double x[],int num,double *q25,double *q75) {
  /**
     Calculate median and both quartiles
     @param[in] x - the values
     @param[in] num - how many
     @return the median
     @param[out] q25 - the 25% quartile
     @param[out] q75 - the 75% quartile
  */
  int n;
  double m;

  if (num <= 0)
    die ("stat_median_quart: Invalid number of observations: %d",num);
  sortAscending (x,num);
  if (q25) {
    n = (num-1)/4;
    m = (double)(num-1)/4.0 - n;
    *q25 = mq_sub (x,n,m);
  }
  if (q75) {
    n = 3*(num-1)/4;
    m = 3*(num-1)/4.0 - n;
    *q75 = mq_sub (x,n,m);
  }
  n = (num-1)/2;
  m = (double)(num-1)/2.0 - n;
  return mq_sub (x,n,m);
}

double stat_median_abs_dev (double x[], int num) {
  /**
     Calcuates the median of the deviation from the median
     @param[in] x - the values
     @param[in] num - how many
     @return the median
  */
  int i;
  double median;
  double res = 0;
  double *dev;

  dev = (double *)malloc (num * sizeof (double));
  median = stat_median (x,num);
  for (i=0;i<num;i++)
    dev[i] = fabs (x[i] - median);
  res = stat_median (dev, num);
  hlr_free (dev);
  return res;
}

double stat_variance (double x[],int num) {
  /**
     Calulate the variance of some numbers
     @param[in] x - the values
     @param[in] num - how many
     @return the varince
  */
  double sum,sum2;
  int i;

  if (num <= 0)
    die ("stat_variance: Invalid number of observations: %d",num);
  if (num == 1)
    return 0.0;
  sum = 0.0;
  sum2 = 0.0;
  for (i=0;i<num;i++) {
    sum += x[i];
    sum2 += x[i] * x[i];
  }
  return (sum2 - sum*sum/num) / (num-1);
}

double stat_stddev (double x[],int num) {
  /**
     Calculates the standard deviation of some numbers
     @param[in] x - the values
     @param[in] num - how many
     @return the standard deviation
  */
  if (num <= 0)
    die ("stat_stddev: Invalid number of observations: %d",num);
  if (num == 1)
    return 0.0;
  return sqrt (stat_variance (x,num));
}

double stat_skew (double x[],int num) {
  /**
     Calculates the skew (valid only for small number of values)
     @param[in] x - the values
     @param[in] num - how many
     @return the skew
  */
  double m,s,sum;
  int i;

  if (num < 3)
    die ("stat_skew: Invalid number of observations: %d",num);
  m = stat_mean (x,num);
  s = stat_stddev (x,num);
  if (s == 0.0)
    return STAT_INVALID;
  sum = 0.0;
  for (i=0;i<num;i++)
    sum += pow ((x[i]-m),3.0);
  return num*sum/((num-1)*(num-2)*pow(s,3.0));
}

double stat_kurt (double x[],int num) {
  /**
     Calculates he kurtosis (valid only for small number of values)
     @param[in] x - the values
     @param[in] num - how many
     @return the kurtosis
   */
  double m,s,sum;
  int i;

  if (num < 4)
    die ("stat_kurt: Invalid number of observations: %d",num);
  m = stat_mean (x,num);
  s = stat_stddev (x,num);
  if (s == 0.0)
    return STAT_INVALID;
  sum = 0.0;
  for (i=0;i<num;i++)
    sum += pow ((x[i]-m),4.0);
  return num*(num+1)*sum/((num-1)*(num-2)*(num-3)*pow(s,4.0)) - 3*(num-1)*(num-1)/((num-2)*(num-3));
}

double stat_percentile (double x[],int num,double p) {
  /**
     Determines the p percentile of the observations in x[]
     @param[in] x - the values
     @param[in] num - how many
     @param[in] p - which percentile
     @return the percentile value
 */
  double z;

  if (num <= 0)
    die ("stat_percentile: Invalid number of observations: %d",num);
  if (p<=0.0 || p>=100.0)
    die ("stat_percentile: Invalid percentage: %f",p);
  sortAscending (x,num);
  z=(p/100.0)*(1.0*num-1);
  if (z==floor(z))
    return x[(int) z];
  else
    return 0.5 * (x[(int) floor(z)] + x[(int) ceil(z)]);
}

double stat_ma68 (double x[],int num) {
  /**
     Calculates the ma68, a robust replacement for the standard deviation
     @param[in] x - the values
     @param[in] num - how many
     @return the standard deviation
  */

  double *med_dev;
  double m;
  int i;

  med_dev = mv_vectorD (num);
  m = stat_median (x,num);
  for (i=0;i<num;i++)
    med_dev[i]=fabs (x[i]-m);
  m = stat_percentile (med_dev,num,68.0);
  mv_freeVectorD (med_dev);
  return m;
}

double stat_span (double x[],int num) {
  /**
     Calculates the span of some values
     @param[in] x - the values
     @param[in] num - how many
     @return the span
  */
  double min,max;
  int i;

  if (num <= 0)
    die ("stat_span: Invalid number of observations: %d",num);
  min = max = x[0];
  for (i=1;i<num;i++) {
    min = (min > x[i] ? x[i] : min);
    max = (max < x[i] ? x[i] : max);
  }
  return (max - min);
}

double stat_chiSquare (int n11,int n12,int n21,int n22) {
  /**
     Calculates the positive Chi^2-Value of a matrix<br>
     n11 n12<br>
     n21 n22<br>
     @param[in] n11,n12,n21,n22 - the matrix elements
     @return the Chi^2 value
  */
  double chi;

  chi=(double)n11-((double)(n11+n12)*(n11+n21)/(n11+n12+n21+n22));
  if (chi < 0)
    chi*=-1.0;
  return chi;
}

double stat_fisherScore (double x[],int numx,double y[],int numy) {
  /**
     Calculates the Fisher Score, a classical measure for the degree of
     separation between two classes
     @param[in] x - the first series of values
     @param[in] numx - how many
     @param[in] y - the second series of values
     @param[in] numy - how many
     @return the Fisherscore
  */
  double d;

  d = stat_mean (x,numx) - stat_mean (y,numy);
  return (d*d) / (stat_variance (x,numx) + stat_variance (y,numy));
}

double stat_robustFisherScore (double x[],int numx,double y[],int numy) {
  /**
     Calculates the robust Fisher Score, a modification of the classical
     Fisher Score that is less affected by single outliers
     @param[in] x - the first series of values
     @param[in] numx - how many
     @param[in] y - the second series of values
     @param[in] numy - how many
     @return the robust Fisher score
  */
  double d;

  d = stat_median (x,numx) - stat_median (y,numy);
  return (d*d) / (pow (stat_ma68 (x,numx),2) + pow (stat_ma68 (y,numy),2));
}

double stat_signalToNoiseRatio (double x[],int numx,double y[],int numy) {
  /**
     Class separation measure used for example in
     Golub et al. Science 286 (1999), 531-537;<br>
     Prefers within class variances compared to the Fisher criterion
     @param[in] x - the first series of values
     @param[in] numx - how many
     @param[in] y - the second series of values
     @param[in] numy - how many
     @return the S/N value
  */
  return fabs (stat_mean (x,numx) - stat_mean (y,numy)) /
         (stat_stddev (x,numx) + stat_stddev (y,numy));
}

/* Anova and non-parametric tests =================================== */

static Array gConds = NULL; // of StatCond
static int isRanked;

void stat_setConds (Array conds) {
  /**
     Sets the conditions and their values for following functions
     @param[in] conds - Array of type StatCond
  */
  StatCond *currCond;
  int i;

  gConds = conds;
  for (i=0;i<arrayMax (gConds);i++) {
    currCond = arrp (gConds,i,StatCond);
    if (!currCond->samples)
      currCond->samples = arrayCreate (2,double);
  }
  isRanked = 0;
}

/* Anova ------------------------------------------------------------ */

double stat_getF (void) {
  /**
     Calculate F value.<br>
     precondition: stat_setConds has been called
  */
  int i,k;
  static Array means = NULL; // of double
  double mean;
  int num;
  double ssAmong,ssWithin;
  StatCond *currCond;

  if (gConds == NULL)
    die ("stat_getF: no conditions");
  if (means == NULL)
    means = arrayCreate (arrayMax (gConds),double);
  arrayClear (means);
  mean = 0.0;
  num = 0;
  for (i=0;i<arrayMax (gConds);i++) {
    currCond = arrp (gConds,i,StatCond);
    array (means,i,double) = 0.0;
    for (k=0;k<arrayMax (currCond->samples);k++)
      array (means,i,double) += arru (currCond->samples,k,double);
    mean += array (means,i,double);
    num += arrayMax (currCond->samples);
    if (arrayMax (currCond->samples) > 0)
      array (means,i,double) /= arrayMax (currCond->samples);
  }
  if (num == 0)
    return STAT_INVALID;
  mean /= num;
  ssAmong = 0.0;
  for (i=0;i<arrayMax (gConds);i++) {
    currCond = arrp (gConds,i,StatCond);
    ssAmong += arrayMax (currCond->samples) * (arru (means,i,double)-mean) *
               (arru (means,i,double)-mean);
  }
  ssAmong /= (arrayMax (gConds) - 1);
  ssWithin = 0.0;
  for (i=0;i<arrayMax (gConds);i++) {
    currCond = arrp (gConds,i,StatCond);
    for (k=0;k<arrayMax (currCond->samples);k++)
      ssWithin += (arru (currCond->samples,k,double) -
                   arru (means,i,double)) * (arru (currCond->samples,k,double) -
                   arru (means,i,double));
  }
  if (num == arrayMax (gConds))
    return STAT_INVALID;
  ssWithin /= (num-arrayMax (gConds));
  if (ssWithin == 0.0)
    return STAT_INVALID;
  return ssAmong/ssWithin;
}

/* non-parametric tests --------------------------------------------- */

/// structure to hold conditions and their values to assign ranks
typedef struct {
  int cond; //!< condition
  double val; //!< value
  double rank; //!< rank assigned
}Rank;

static Array gRanks = NULL; // of Rank

static int orderRanks (Rank *r1,Rank *r2) {
  if (r1->val > r2->val)
    return 1;
  if (r2->val > r1->val)
    return -1;
  return 0;
}

static void doRanking (void) {
  /**
     Used internally by stat_getH, stat_getU, stat_getPvalU.<br>
     Precondition: stat_setConds has been called
  */
  int i,k,l;
  Rank *currRank;
  int rank;
  StatCond *currCond;

  if (gConds == NULL)
    die ("doRanking: no conditions");
  if (isRanked)
    return;
  if (gRanks == NULL)
    gRanks = arrayCreate (10,Rank);
  arrayClear (gRanks);
  for (i=0;i<arrayMax (gConds);i++) {
    currCond = arrp (gConds,i,StatCond);
    for (k=0;k<arrayMax (currCond->samples);k++) {
      currRank = arrayp (gRanks,arrayMax (gRanks),Rank);
      currRank->cond = i;
      currRank->val = arru (currCond->samples,k,double);
    }
  }
  arraySort (gRanks,(ARRAYORDERF)orderRanks);
  i=0;
  while (i<arrayMax (gRanks)) {
    rank = i+1;
    for (k=i+1;k<arrayMax (gRanks);k++) {
      if (arrp (gRanks,i,Rank)->val != arrp (gRanks,k,Rank)->val)
        break;
      rank += k+1;
    }
    for (l=i;l<k;l++)
      arrp (gRanks,l,Rank)->rank = (double)rank/(k-i);
    i = k;
  }
  isRanked = 1;
}

double stat_getH (void) {
  /**
     Kruskal-Wallis test.<br>
     Precondition: stat_setConds has been called
  */
  int num;
  double sum1,sum2;
  double h;
  int i,k;
  StatCond *currCond;
  Rank *currRank;
  double t,r1;
  int n;

  if (gConds == NULL)
    die ("stat_getH: no conditions");
  doRanking ();
  num = 0;
  sum2 = 0.0;
  for (i=0;i<arrayMax (gConds);i++) {
    sum1 = 0.0;
    currCond = arrp (gConds,i,StatCond);
    num += arrayMax (currCond->samples);
    for (k=0;k<arrayMax (gRanks);k++) {
      currRank = arrp (gRanks,k,Rank);
      if (currRank->cond == i)
        sum1 += currRank->rank;
    }
    if (arrayMax (currCond->samples) != 0)
      sum2 += (sum1 * sum1 / arrayMax (currCond->samples));
  }
  if (num == 0)
    return STAT_INVALID;
  h = 12.0 / (num * (num+1)) * sum2 - 3.0 * (num+1);

  t = 0.0;
  r1 = -1.0;
  n = 0;
  for (k=0;k<arrayMax (gRanks);k++) {
    currRank = arrp (gRanks,k,Rank);
    if (currRank->rank != r1) {
      if (n > 1) {
        t += (pow (n,3)-n);
      }
      n = 1;
      r1 = currRank->rank;
    }
    else
      n++;
  }
  if (t > 0.0)
    h /= (1 - t/(pow (num,3)-num));
  return h;
}

/// maximum number of iterations to use in stat_getPvalU()
#define THRESHOLD_ITER_PVAL_U 10000

double stat_getPvalU (void) {
  /**
     Get the p-value of the U test.<br>
     From Lindgren, Statistical Theory, Chapman&Hall 1993<br>
     Precondition: stat_setConds has been called
  */
  double sumRanks0,sumRanks1,sumRanks,sr;
  Rank *currRank;
  int num0,num1,num,n,n1;
  int i,k;

  if (gConds == NULL)
    die ("stat_getPvalU: no conditions");
  doRanking ();
  sumRanks0 = sumRanks1 = 0.0;
  num0 = num1 = 0;
  for (k=0;k<arrayMax (gRanks);k++) {
    currRank = arrp (gRanks,k,Rank);
    if (currRank->cond == 0) {
      sumRanks0 += currRank->rank;
      num0++;
    }
    else if (currRank->cond == 1) {
      sumRanks1 += currRank->rank;
      num1++;
    }
  }
  if (sumRanks0/num0 < sumRanks1/num1) {
    sr = sumRanks0;
    num = num0;
  }
  else {
    sr = sumRanks1;
    num = num1;
  }
  if (cmb_NtiefK (arrayMax (gRanks),num) < THRESHOLD_ITER_PVAL_U) {
    int *p;

    cmb_combInit (arrayMax (gRanks),num);
    n = n1 = 0;
    while (cmb_combNext (&p)) {
      sumRanks = 0;
      for (i=0;i<num;i++)
        sumRanks += p[i]+1;
      if (sumRanks <= sr)
        n1++;
      n++;
    }
  }
  else {
    int *p;
    int p1,p2;
    int seed = -1;
    int r;

    p = (int *)hlr_calloc (num,sizeof (int));
    n = n1 = 0;
    for (i=0;i<THRESHOLD_ITER_PVAL_U;i++) {
      p1 = 0;
      while (p1 < num) {
        r = arrayMax (gRanks) * (int)rcp_ran3 (&seed);
        for (p2=0;p2<num;p2++)
          if (p[p2] == r)
            break;
        if (p2 == num) {
          p[p1] = r;
          p1++;
        }
      }
      sumRanks = 0;
      for (p1=0;p1<num;p1++)
        sumRanks += p[p1]+1;
      if (sumRanks <= sr)
        n1++;
      n++;
    }
    hlr_free (p);
  }
  return 2.0*n1/n; // 2.0 for two-sided test
}

double stat_getU (void) {
  /**
     U-test by Wilcoxon, Mann and Whitney.<br>
     Fraom: Lothar Sachs, Statistische Auswertungsmethoden<br>
     Precondition: stat_setConds has been called
  */
  double u[2];
  int i,k;
  Rank *currRank;

  if (gConds == NULL)
    die ("stat_getU: no conditions");
  doRanking ();
  for (i=0;i<2;i++) {
    u[i] = arrayMax (arrp (gConds,0,StatCond)->samples) *
      arrayMax (arrp (gConds,1,StatCond)->samples);
    u[i] += arrayMax (arrp (gConds,i,StatCond)->samples) *
      (arrayMax (arrp (gConds,i,StatCond)->samples) + 1) / 2;
    for (k=0;k<arrayMax (gRanks);k++) {
      currRank = arrp (gRanks,k,Rank);
      if (currRank->cond == i)
        u[i] -= currRank->rank;
    }
  }
  if (u[0]+u[1] != arrayMax (arrp (gConds,0,StatCond)->samples) *
      arrayMax (arrp (gConds,1,StatCond)->samples))
    die ("stat_getU: error in calculation");
  if (u[0] < u[1])
    return u[0];
  return u[1];
}

static void doRankingDiff (void) {
  /**
     Used internally by stat_getPvalSignedRank.<br>
     Precondition: stat_setConds has been called
  */
  int i,k,l;
  Rank *currRank;
  int rank;
  StatCond *cond1,*cond2;

  if (gConds == NULL)
    die ("doRankingDiff: no conditions");
  if (arrayMax (gConds) != 2)
    die ("doRankingDiff: need exactly 2 conditions");
  cond1 = arrp (gConds,0,StatCond);
  cond2 = arrp (gConds,1,StatCond);
  if (arrayMax (cond1->samples) != arrayMax (cond2->samples))
    die ("doRankingDiff: both conditions need the same number of samples");
  if (isRanked)
    return;
  if (gRanks == NULL)
    gRanks = arrayCreate (10,Rank);
  arrayClear (gRanks);
  for (i=0;i<arrayMax (cond1->samples);i++) {
    if (arru (cond1->samples,i,double) == arru (cond2->samples,i,double))
      continue;
    currRank = arrayp (gRanks,arrayMax (gRanks),Rank);
    if (arru (cond1->samples,i,double) > arru (cond2->samples,i,double)) {
      currRank->cond = 0;
      currRank->val = arru (cond1->samples,i,double) -
                      arru (cond2->samples,i,double);
    }
    else {
      currRank->cond = 1;
      currRank->val = arru (cond2->samples,i,double) -
                      arru (cond1->samples,i,double);
    }
  }
  arraySort (gRanks,(ARRAYORDERF)orderRanks);

  i=0;
  while (i<arrayMax (gRanks)) {
    rank = i+1;
    for (k=i+1;k<arrayMax (gRanks);k++) {
      if (arrp (gRanks,i,Rank)->val != arrp (gRanks,k,Rank)->val)
        break;
      rank += k+1;
    }
    for (l=i;l<k;l++)
      arrp (gRanks,l,Rank)->rank = (double)rank/(k-i);
    i = k;
  }
  isRanked = 1;
}

/// maximum number of ranks to use in stat_getPvalSignedRank()
#define THRESHOLD_RANKS_PVAL_SR 15

double stat_getPvalSignedRank (void) {
  /**
     Signed rank test: non-parametric equivalent of paired t-test,
     assumes symmetric distribution about the median.<br>
     From Lindgren<br>
     In Statistica: corresponds to Wilcoxon matched pairs test<br>
     p-values correspond to those in SAS for <15 replicates<br>
     Precondition: stat_setConds has been called
  */
  double sum[2];
  int i,k;
  Rank *currRank;
  double sumRanks;
  int condNum;
  int numSmaller;
  int numTested;

  if (gConds == NULL)
    die ("stat_getPvalSignedRank: no conditions");
  doRankingDiff ();
  if (arrayMax (gRanks) > 63)
    die ("stat_getPvalSignedRank: number of pairs should be smaller than 64");
  // because of overflow of long
  sum[0] = sum[1] = 0.0;
  for (i=0;i<arrayMax (gRanks);i++) {
    currRank = arrp (gRanks,i,Rank);
    sum[currRank->cond] += currRank->rank;
  }
  if (sum[0] < sum[1])
    condNum = 0;
  else
    condNum = 1;
  numSmaller = 0;
  if (arrayMax (gRanks) < THRESHOLD_RANKS_PVAL_SR) {
    numTested = (int)pow (2,arrayMax (gRanks));
    for (i=0;i<numTested;i++) {
      sumRanks = 0.0;
      for (k=0;k<arrayMax (gRanks);k++) {
        if (((i & (1<<k)) > 0) == condNum)
          sumRanks += arrp (gRanks,k,Rank)->rank;
      }
      if (sumRanks <= sum[condNum])
        numSmaller++;
    }
  }
  else {
    int seed = -1;
    unsigned long long r;

    numTested = (int)pow (2,THRESHOLD_RANKS_PVAL_SR);
    for (i=0;i<numTested;i++) {
      sumRanks = 0.0;
      r = (unsigned long long)rcp_ran3 (&seed) * (unsigned long long)pow (2,arrayMax (gRanks));
      for (k=0;k<arrayMax (gRanks);k++) {
        if (((r & (1<<k)) > 0) == condNum)
          sumRanks += arrp (gRanks,k,Rank)->rank;
      }
      if (sumRanks <= sum[condNum])
        numSmaller++;
    }
  }
  return 2.0*numSmaller/numTested; // 2.0 for two-sided test
}

/* Outlier tests ============================================ */

/// for outlier tests
typedef struct {
  int f; //!< degrees of freedom
  double p[3]; //!< up to three threshold values
}Rtab;

/* DIXON outlier test ----------------------------------------*/

/// table with values to calculate Dixon outlier test
static Rtab rTabD[] = {
{      3, {0, 0.99, 0}},
{      4, {0, 0.89, 0}},
{      5, {0, 0.76, 0}},
{      6, {0, 0.70, 0}},
{      7, {0, 0.64, 0}},
{      8, {0, 0.59, 0}},
{      9, {0, 0.56, 0}},
{     10, {0, 0.53, 0}},
{     11, {0, 0.50, 0}},
{     12, {0, 0.48, 0}},
{     13, {0, 0.47, 0}},
{     14, {0, 0.45, 0}},
{     15, {0, 0.44, 0}},
{     20, {0, 0.39, 0}},
{     25, {0, 0.36, 0}},
{     30, {0, 0.34, 0}}};

int stat_dixon (double x[],int num,int sig) {
  /**
     Tests highest and lowest values for outliers.<br>
     !!! argument sig is not used, only present for compatibility
          with other outlier tests !!!
     @param[in] x - vector of values
     @param[in] num - number of values
     @param[out] x - possibly with the lower outlier removed
     @param[in] sig - significance (STAT_SIG_95, etc.)
     @return number of remaining values
  */
  double q1,qn;
  int f;
  int i;
  double span;

  if (num < 3)
    die ("dixon: Invalid number of observations: %d",num);
  span = stat_span (x,num);
  if (span == 0.0)
    return num;
  sortAscending (x,num);
  q1 = fabs(x[1] - x[0]) / span;
  qn = fabs(x[num-1] - x[num-2]) / span;
  for (f=0;f<NUMELE (rTabD);f++)
    if (rTabD[f].f >= num)
      break;
  if (f == NUMELE (rTabD))
    die ("dixon: Too many data points for this test (max: 30).");
  if (q1 >= rTabD[f].p[1]) {
    if (qn >= rTabD[f].p[1]) {
      for (i=1;i<num;i++)
        x[i-1] = x[i];
      return num-2;
    }
    else {
      for (i=1;i<num;i++)
        x[i-1] = x[i];
      return num-1;
    }
  }
  else {
    if (qn >= rTabD[f].p[1]) {
      return num-1;
    }
    else {
      return num;
    }
  }
}

/* GRUBBS outlier test ---------------------------------------*/

static Rtab rTabG[] = {
{      3, {1.15, 1.16, 0}},
{      4, {1.46, 1.49, 0}},
{      5, {1.67, 1.75, 0}},
{      6, {1.82, 1.94, 0}},
{      7, {1.94, 2.10, 0}},
{      8, {2.03, 2.22, 0}},
{      9, {2.11, 2.32, 0}},
{     10, {2.18, 2.41, 0}},
{     12, {2.29, 2.55, 0}},
{     15, {2.41, 2.71, 0}},
{     20, {2.56, 2.88, 0}},
{     30, {2.75, 3.10, 0}},
{     40, {2.87, 3.24, 0}},
{     50, {2.96, 3.34, 0}}};

int stat_grubbs (double x[],int num,int sig) {
  /**
     Tests for outliers at a given level of significance
     @param[in] x - vector of values
     @param[in] num - number of values
     @param[in] sig - STAT_SIG_95 or STAT_SIG_99 or STAT_SIG_99_9
     @param[out] x - possibly with outliers removed
     @return number of remaining values
  */
  double t;
  int f;
  int i,j;
  int n;
  double sd;
  double mean;

  if (num < 3)
    die ("grubbs: Invalid number of observations: %d",num);
  for (f=0;f<NUMELE (rTabG);f++)
    if (rTabG[f].f >= num)
      break;
  if (f == NUMELE (rTabG))
    die ("grubbs: Too many data points for this test (max: 50)");
  sd = stat_stddev (x,num);
  mean = stat_mean (x,num);
  n = num;
  i = 0;
  while (i < n) {
    t = fabs(x[i] - mean)/sd;
    if (t >= rTabG[f].p[sig]) {
      n--;
      for (j=i;j<n;j++)
        x[j] = x[j+1];
    }
    else
      i++;
  }
  return n;
}

/* nalimov outlier test --------------------------------------*/

static Rtab rTab[] = {
{      1, {1.409, 1.414, 1.414}},
{      2, {1.645, 1.715, 1.730}},
{      3, {1.757, 1.918, 1.982}},
{      4, {1.814, 2.051, 2.178}},
{      5, {1.848, 2.142, 2.329}},
{      6, {1.870, 2.208, 2.447}},
{      7, {1.885, 2.256, 2.540}},
{      8, {1.895, 2.294, 2.616}},
{      9, {1.903, 2.324, 2.678}},
{     10, {1.910, 2.348, 2.730}},
{     11, {1.916, 2.368, 2.774}},
{     12, {1.920, 2.385, 2.812}},
{     13, {1.923, 2.399, 2.845}},
{     14, {1.926, 2.412, 2.874}},
{     15, {1.928, 2.423, 2.899}},
{     16, {1.931, 2.432, 2.921}},
{     17, {1.933, 2.440, 2.941}},
{     18, {1.935, 2.447, 2.959}},
{     19, {1.936, 2.454, 2.975}},
{     20, {1.937, 2.460, 2.990}},
{     25, {1.942, 2.483, 3.047}},
{     30, {1.945, 2.498, 3.085}},
{     35, {1.948, 2.509, 3.113}},
{     40, {1.949, 2.518, 3.134}},
{     45, {1.950, 2.524, 3.152}},
{     50, {1.951, 2.529, 3.166}},
{    100, {1.956, 2.553, 3.227}},
{    200, {1.958, 2.564, 3.265}},
{    300, {1.958, 2.566, 3.271}},
{    400, {1.959, 2.568, 3.275}},
{    500, {1.959, 2.570, 3.279}},
{    600, {1.959, 2.571, 3.281}},
{    700, {1.959, 2.572, 3.283}},
{    800, {1.959, 2.573, 3.285}},
{1000000, {1.960, 2.576, 3.291}}};

static int nalimov (double x[],int num,int xi,int sig,int remove) {
  /**
     Tests for outlier at a given level of significance
     @param[in] x : vector of values
     @param[in] num : number of values
     @param[in] xi : index of value to check whether it is an outlier
     @param[in] sig : STAT_SIG_95 or STAT_SIG_99 or STAT_SIG_99_9
     @param[in] remove: 1: remove outlier from vector, 0: not not remove outlier
     @param[out] x - possibly with one outlier removed
     @return 1 if outlier was detected, 0 if not
  */
  double r;
  int f;
  int i;
  double sd;

  sd = stat_stddev (x,num);
  if (sd == 0.0)
    return 0;
  r = fabs (x[xi] - stat_mean (x,num)) / sd * sqrt ((double)num / (num-1));
  f = num-2;
  for (i=0;i<NUMELE (rTab);i++)
    if (rTab[i].f >= f)
      break;
  if (i == NUMELE (rTab))
    die ("nalimov: Invalid index of rTab: %d",i);
  if (r < rTab[i].p[sig])
    return 0;
  if (remove) {
    for (i=xi+1;i<num;i++)
      x[i-1] = x[i];
  }
  return 1;
}

int stat_nalimov (double x[],int num,int sig) {
  /**
     Removes outliers at a given level of significance in iterative way
     @param[in] x - vector of values
     @param[in] num - number of values
     @param[in] sig - STAT_SIG_95 or STAT_SIG_99 or STAT_SIG_99_9
     @param[out] x - possibly with removed outliers
     @return remaining number of values
  */
  int done;
  int n,i;

  if (num < 3)
    die ("stat_nalimov: Invalid number of observations: %d, at least 3 required",num);
  done = 0;
  n = num;
  while (!done && n > 2) {
    for (i=0;i<n;i++)
      if (nalimov (x,n,i,sig,1)) /* one value deleted */
        break;
    if (i == n)
      done = 1;
    else
      n--;
  }
  return n;
}

int stat_nalimov1 (double x[],int num,int sig) {
  /**
     Removes outliers at a given level of significance;
     tests each value in turn in non-iterative fashion
     @param[in] x - vector of values
     @param[in] num - number of values
     @param[in] sig - STAT_SIG_95 or STAT_SIG_99 or STAT_SIG_99_9
     @param[out] x - possibly with removed outliers
     @return remaining number of values
  */
  static Array inds = NULL; // of int
  int i,k,i1;

  if (num < 3)
    die ("stat_nalimov1: Invalid number of observations: %d, at least 3 required",
         num);
  if (inds == NULL)
    inds = arrayCreate (10,int);
  arrayClear (inds);
  for (i=0;i<num;i++)
    if (nalimov (x,num,i,sig,0))
      array (inds,arrayMax (inds),int) = i;
  i1 = 0;
  for (i=0;i<num;i++) {
    for (k=0;k<arrayMax (inds);k++)
      if (i == arru (inds,k,int))
        break;
    if (k == arrayMax (inds))
      x[i1++] = x[i];
  }
  return i1;
}

/* X84 outlier test ------------------------------------------*/

int stat_x84 (double x[],int num,int sig) {
  /**
     Outlier test based on median and median absolute deviates.<br>
     From: Hampel, Ronchetti, Rousseuw, Stahel: Robust Statistics<br>
     !!! argument sig is not used, only present for compatibility
         with other outlier tests !!!
     @param[in] x - vector of values
     @param[in] num - number of values
     @param[in] sig - STAT_SIG_95 or STAT_SIG_99 or STAT_SIG_99_9
     @param[out] x - possibly with the lower outlier removed
     @return number of remaining values
  */
  double median,mad;
  double t;
  int n;
  int i,j;

  if (num < 3)
    die ("x84: Invalid number of observations: %d",num);

  median = stat_median (x,num);
  mad = stat_median_abs_dev (x,num);

  n = num;
  i = 0;
  while (i < n) {
    t = fabs (x[i] - median);
    t = t / mad;
    if (t >= 5.2) {
      n--;
      for (j=i;j<n;j++)
        x[j] = x[j+1];
    }
    else
      i++;
  }
  return n;
}

/* test program -------------------------------------------------*/

/*
  main () {
    double data[] = {30.41,30.05,30.49,29.22,30.40,30.42};
    double x1[10];
    int i,k1,k2,n;

    // check fTable
    for (i=0;i<3;i++)
      for (k1=1;k1<26;k1++)
        for (k2=1;k2<18;k2++) {
          if (fTab[i][k1][k2] > fTab[i][k1-1][k2])
            printf ("%d %d %d > %d %d %d\n",i,k1,k2,i,k1-1,k2);
          if (fTab[i][k1][k2] > fTab[i][k1][k2-1])
            printf ("%d %d %d > %d %d %d\n",i,k1,k2,i,k1,k2-1);
        }
    for (i=0;i<3;i++)
      x1[i] = data[i];
    printf ("Mean= %lf (expected 30.317)\n",stat_mean (x1,3));

    for (i=0;i<3;i++)
      x1[i] = data[i];
    printf ("Stddev= %lf (expected 0.234)\n",stat_stddev (x1,3));

    for (i=0;i<4;i++)
      x1[i] = data[i];
    printf ("Values after nalimov (STAT_SIG_95): ");
    n = stat_nalimov (x1,4,STAT_SIG_95);
    for (i=0;i<n;i++)
      printf ("%lf ",x1[i]);
    printf ("\n");

    for (i=0;i<4;i++)
      x1[i] = data[i];
    printf ("Values after nalimov (STAT_SIG_99): ");
    n = stat_nalimov (x1,4,STAT_SIG_99);
    for (i=0;i<n;i++)
      printf ("%lf ",x1[i]);
    printf ("\n");

    for (i=0;i<4;i++)
      x1[i] = data[i];
    printf ("Values after nalimov (STAT_SIG_99_9): ");
    n = stat_nalimov (x1,4,STAT_SIG_99_9);
    for (i=0;i<n;i++)
      printf ("%lf ",x1[i]);
    printf ("\n");

    for (i=0;i<6;i++)
      x1[i] = data[i];
    printf ("Values after nalimov (STAT_SIG_95): ");
    n = stat_nalimov (x1,6,STAT_SIG_95);
    for (i=0;i<n;i++)
      printf ("%lf ",x1[i]);
    printf ("\n");

    for (i=0;i<6;i++)
      x1[i] = data[i];
    printf ("Values after nalimov (STAT_SIG_99): ");
    n = stat_nalimov (x1,6,STAT_SIG_99);
    for (i=0;i<n;i++)
      printf ("%lf ",x1[i]);
    printf ("\n");

    for (i=0;i<6;i++)
      x1[i] = data[i];
    printf ("Values after nalimov (STAT_SIG_99_9): ");
    n = stat_nalimov (x1,6,STAT_SIG_99_9);
    for (i=0;i<n;i++)
      printf ("%lf ",x1[i]);
    printf ("\n");
  }
*/

/// Iterations to use in stat_randomGaussInit()
#define RG_ITERS 24
static double rg_mean = 0.0;
static double rg_stddev = 1.0;
static double rg_scalor = 0.0; // initialize with value making a missing randomGaussInit() obvious
static int rg_seed = 12345; // random seed

void stat_randomGaussInit (double mean,double stddev,int doRandomize) {
  /**
     Initialize the generator for normal distributed random values
     N (mean,stddev**2).<br>
     Postcondition: stat_randomGauss() can be called
     @param[in] mean - mean value for the distribution
     @param[in] stddev - standard deviation for the distribution
     @param[in] doRandomize - 0 to always generate the same series of
                              random numbers; 1 to generate a different
                              series in each run
  */
  rg_mean = mean;
  rg_stddev = stddev;
  rg_scalor = sqrt (12.0 / RG_ITERS);
  if (doRandomize)
    rg_seed = getpid ();
}

double stat_randomGauss (void) {
  /**
     Routine for generating normal-distributed random numbers.<br>
     Mathematical basis:<br>
     X = sqrt (12/n) * ((U1+U2+...+Un) - n/2)
     where Ui, i=1..n are random variables distributed uniformly in (0,1)
     is N(0,1) distributed.
*/
  double s = 0.0;
  int i = RG_ITERS;

  if (rg_scalor==0.0)
    die ("stat_randomGauss: random number generator not initialized");
  while (i--)
    s += rcp_ran3 (&rg_seed);
  return (rg_scalor * (s - (double)RG_ITERS / 2.0)) * rg_stddev + rg_mean;
}

void stat_bootstrap (double *val,int count,int *seed,int rep,
                     double *mean,double *median,double quantile[]) {
  /**
     NOTE: we calculate quantiles from 1 percent to 99 percent in integer steps
     - so quantile [100] has to allocated by the caller.
     "rep" must be a multiple of 100 and must not be smaller than 100.
     In the web interface, this is ensured by a select menu. In other cases,
     use more sophisticated routines.
  */
  int i,j;
  int index;
  double av;
  Array bsMeans; // of double

  *mean = 0;
  bsMeans = arrayCreate (rep,double);
  for (i=0;i<rep;i++) {
    av = 0;
    for (j=0;j<count;j++) {
      index = (int)(rcp_ran3 (seed) * (count-1));
      av += val[index];
    }
    av /= count;
    array (bsMeans,arrayMax (bsMeans),double) = av;
    *mean += av;
  }
  arraySort (bsMeans,(ARRAYORDERF)&arrayDoublecmp);
  *mean /= rep;
  index = -1;
  for (i=1;i<100;i++) {
    index += rep/100;
    quantile[i] = arru (bsMeans,index,double);
  }
  index = rep/2 - 1;
  *median  = arru (bsMeans,index,double);
  arrayDestroy (bsMeans);
}

/* routines for quantile-quantile normalization */

/// an element with its original index
typedef struct {
  int index; //!< index
  double value; //!< value
}IndexedValue;

static int sort_indexedValues (IndexedValue *a,IndexedValue *b) {
  /**
     Private; needed for sorting indexed values by value
  */
  return (a->value > b->value) ? 1 : -1;
}

void stat_qq (double **x,int n,int dim,double *values) {
  /**
     Applies quantile-quantile normalizaton on a set of n vectors with dim
     components; the distribution of values is then identical for all resulting
     vectors;<br>
     Memory for array "values" has to be allocated and managed by caller;
     dimension of values is values[dim]
     @param[in] x - data to be normalized; x has dimension x[n][dim]
     @param[in] n = number of rows of x
     @param[in] dim - number of columns of x
     @param[out] values - if NULL, this parameter is ignored;
                          otherwise, the sorted list of component values as
                          obtained after normalization is returned for later
                          use, e.g. as input parameter for stat_qq_fixed()
  */
  Array *tmp;
  double tmpmean;
  int i,j;

  // allocate memory for temporary data
  tmp = (Array *)hlr_calloc (n,sizeof(Array));
  for (i=0;i<n;i++)
    tmp[i] = arrayCreate (dim,IndexedValue);
  // initialize temporary space
  for (i=0;i<n;i++)
    for (j=0;j<dim;j++) {
      array (tmp[i],j,IndexedValue).value = x[i][j];
      arru (tmp[i],j,IndexedValue).index = j;
    }
  // first step of QQ: sort all vectors
  for (i=0;i<n;i++)
    arraySort (tmp[i],(ARRAYORDERF)&sort_indexedValues);
  // next, calculate means over all rows and replace original data by QQ normalized values
  for (j=0;j<dim;j++) {
    tmpmean = 0.0;
    for (i=0;i<n;i++)
      tmpmean += arru (tmp[i],j,IndexedValue).value;
    tmpmean /= n;
    if (values != NULL)
      values[j] = tmpmean;
    for (i=0;i<n;i++)
      x[i][arru (tmp[i],j,IndexedValue).index] = tmpmean;
  }
  // finally, free allocated memory
  for (i=0;i<n;i++)
    arrayDestroy (tmp[i]);
  hlr_free (tmp);
}

void stat_qq_fixed  (double **x,int n,int dim,double *values) {
  /**
     Enforces a fixed distribution on a set of n vectors with dim components;
     the values used to fill.<br>
     Memory for array "values" has to be allocated and managed by caller;
     dimension of values is values[dim]
     @param[in] x - data to be normalized; x has dimension x[n][dim]
     @param[in] n - number of rows of x
     @param[in] dim - number of columns of x
     @param[out] values - values to replace the components of the orginal
                          vectors; these must be sorted in ascending order;
                          the output from stat_qq() can be directly used as
                          input at this point
  */
  Array tmp;
  int i,j;

  tmp=arrayCreate (dim,IndexedValue);
  for (i=0;i<n;i++) { // loop over all vectors
    // initialize temporary space
    for (j=0;j<dim;j++) {
      array (tmp,j,IndexedValue).value = x[i][j];
      arru (tmp,j,IndexedValue).index = j;
    }
    // sort vector
    arraySort (tmp,(ARRAYORDERF)&sort_indexedValues);
    // then, replace values in original vectors by user specified ones
    for (j=0;j<dim;j++)
      x[i][arru (tmp,j,IndexedValue).index] = values[j];
  }
  arrayDestroy (tmp);
}

/* Gini Inequality index */

double stat_gini (double x[],int num) {
  /**
     Calculate Gini index.<br>
     Implementation follows R code of package ineq (function Gini(x) )<br>
     Postcondition: x is sorted
     @param[in] x - numbers
     @param[in] num - how many
     @return Gini index
  */
  double sum = 0.0;
  double gini = 0.0;
  int i;

  sortAscending (x,num);
  for (i=0;i<num;i++) {
    gini += (double)(i+1)*x[i];
    sum += x[i];
  }
  gini = 2.0*(gini/((double)num*sum));
  return gini - 1.0 - (1.0/(double)num);
}

double stat_phi (double x) {
  /**
     Phi function. This function computes the value of the normal c.d.f. at
     point x.<br>
     Ref.: Abramowitz, M. and I.A. Stegun (1964). Handbook of Mathematical
     Functions with Formulas, Graphs and Mathematical Tables.
  */
  double y;

  if (x > 0.0) {
    if (x > 45.0) {
      return 1.0;
    }
    else {
      y = x;
      return 1.0-0.5/pow((1.0+(0.0498673470+(0.0211410061+(0.0032776263+(0.0000380036+(0.0000488906+0.0000053830*y)*y)*y)*y)*y)*y),16);
    }
  }
  else {
    if (x < -45.0) {
      return 0.0;
    }
    else {
      y =- x;
      return 0.5/pow((1.0+(0.049867347+(0.0211410061+(0.0032776263+(0.0000380036+(0.0000488906+0.0000053830*y)*y)*y)*y)*y)*y),16);
    }
  }
}

double stat_phiQuantile (double beta) {
  /**
     Phi quantile function. This function computes the quantile of order beta
     from the normal distribution with k degrees of freedom.<br>
     Ref.: Abramowitz, M. and I.A. Stegun (1964). Handbook of Mathematical
     Functions with Formulas, Graphs and Mathematical Tables.
  */
  double delta;
  double high;
  double low;
  double mid;
  double w;

  if (beta > 0.5)
    delta = beta;
  else
    delta = 1.0-beta;
  w = sqrt (-log ((1.0-delta)*(1.0-delta)));
  low = w-(2.515517+(0.802853+0.010328*w)*w)/(1.0+(1.432788+(0.189269+0.001308*w)*w)*w);
  high = low + 4.5e-4;
  low = low - 4.5e-4;
  do {
    mid = (low+high) / 2.0;
    if (stat_phi (mid) < delta)
      low=mid;
    else
      high=mid;
  } while (fabs (mid-(low+high)/2.0) > 1.e-15);
  if (beta >= 0.5)
    return mid;
  else
    return -mid;
}

double stat_studentTQuantile (double beta,int k) {
  /**
     Student t quantile function. This function computes the quantile of order
     beta from the Student t distribution with k degrees of freedom.<br>
     Ref.: Abramowitz, M. and I.A. Stegun (1964). Handbook of Mathematical
     Functions with Formulas, Graphs and Mathematical Tables.
   */
  double dfk;
  double delta;
  double low;
  double high;
  double mid;
  double test=0.0;
  double w;
  double x;
  double y;

  if (beta == 0.5)
    return 0.0;
  else if (k >= 7) {
    x = stat_phiQuantile (beta);
    y = x*x;
    dfk = 1.0*k;
    return  x*(1.0+((-945.0+(-3600.0+(2880.0+23040.0*dfk)*dfk)*dfk)
                   +((-1920.0+(4080.0+(15360.0+23040.0*dfk)*dfk)*dfk)
                     +((1482.0+(4560.0+4800.0*dfk)*dfk)+((776.0+720.0*dfk)+79.0*y)*y)*y)*y)
               /(dfk*dfk*dfk*dfk*92160.0));
  }
  else if (k == 1)
    return tan ((beta-0.5)*M_PI);
  else if (k == 2)
    return (2.0*beta-1)/sqrt(2.0*beta*(1.0-beta));
  else {
    if (beta >= 0.5)
      delta = beta;
    else
      delta = 1.0-beta;
    low = stat_phiQuantile (delta);
    high = (2.0*delta-1.0) / sqrt (2.0*delta*(1.0-delta));
    do {
      mid = (low+high)/2.0;
      w = mid*mid;
      if (k == 3)
        test = sqrt (3.0)*tan (M_PI*(delta-0.5)-(mid*sqrt (3.0))/(3.0+w));
      else if (k == 4)
        test = (2*delta-1.0)*pow (sqrt (4.0+w),3.0)/(6.0+w);
      else if (k == 5) {
        test = sqrt (5.0)
          *tan (M_PI*(delta-0.5) -
                (mid*sqrt (5.0))/
                (3.0*(5.0+w)*(5.0+w))*(25.0+3.0*w));
      }
      else if (k == 6)
        test = (2.0*delta-1.0)*pow (sqrt (6.0+w),5.0)/(w*w+15.0*w+67.5);
      if (test > mid)
        low=mid;
      else
        high=mid;
    } while (fabs (mid-(low+high)/2.)>1.e-15);
    if (beta >= 0.5)
      return mid;
    else
      return -mid;
  }
}

/* Implementation of Fisher's exact test of independence for 2x2 tables.
   It is used to detect group differences using frequency data. Members of the
   2 independent groups must be in one of two mutually exclusive categories.
   This code is based on Javascript code by Oyvind Langsrud
   (http://www.matforsk.no/ola/fisher.htm)
*/

static int **gVal;
static int sn11;
static int sn1_;
static int sn_1;
static int sn;
static double sprob;

static double lnfact (int n) {
  if (n<=1)
    return 0;
  return rcp_gammln (n+1);
}

static double lnbico (int n,int k) {
  return (lnfact (n)-lnfact (k)-lnfact (n-k));
}

static double hyper_323 (int n11,int n1_,int n_1,int n) {
  return exp (lnbico (n1_,n11)+lnbico (n-n1_,n_1-n11)-lnbico (n,n_1));
}

static double hyper0 (int n11i,int n1_i,int n_1i,int ni) {

  if (!(n1_i | n_1i | ni)) {
    if(!(n11i % 10 == 0)) {
      if (n11i==sn11+1) {
        sprob = sprob*((double)(sn1_-sn11)/(n11i))*((double)(sn_1-sn11)/(n11i+sn-sn1_-sn_1));
        sn11 = n11i;
        return sprob;
      }
      if (n11i==sn11-1) {
        sprob = sprob*((double)(sn11)/(sn1_-n11i))*((double)(sn11+sn-sn1_-sn_1)/(sn_1-n11i));
        sn11 = n11i;
        return sprob;
      }
    }
    sn11 = n11i;
  }
  else {
    sn11 = n11i;
    sn1_ = n1_i;
    sn_1 = n_1i;
    sn = ni;
  }
  sprob = hyper_323 (sn11,sn1_,sn_1,sn);
  return sprob;
}

static double hyper (int n11) {
  return hyper0 (n11,0,0,0);
}

double fisher_exact_test (int x11,int x12,int x21,int x22,
                          double *sless,double *slarg,double *twotail) {
  /**
     Fisher's exact test of independence for 2x2 tables
     @param[in] x11,x12,x21,x22 - the 4 numbers of the 2 mutually exclusive
                                  categories (2x2 table)
     @param[out] sless,slarg,twotail - the left-sided, the right-sided and the
                                       two-tailed pvalue
     @return value of Fisher exact tet
  */
  int n = 0;
  int n1_ = 0;
  int n_1 = 0;
  int i,j;
  int min = 0;
  int max = 0;
  double sleft = 0;
  double sright = 0;
  double p = 0;
  double prob = 0;

  gVal = mv_matrixI (2,2);
  gVal[0][0] = x11;
  gVal[0][1] = x12;
  gVal[1][0] = x21;
  gVal[1][1] = x22;
  for (i=0;i<2;i++){
    n1_ = n1_ + gVal[0][i]; // row total
    n_1 = n_1 + gVal[i][0]; // column total
    for (j=0;j<2;j++)
      n = n+gVal[i][j];
  }
  max = n1_;
  if (n_1 < max)
    max = n_1;
  min = n1_ + n_1 - n;
  if (min < 0)
    min=0;
  if (min == max) {
    *sless = 1;
    *slarg = 1;
    *twotail = 1;
    mv_freeMatrixI (gVal);
    return 1;
  }
  prob=hyper0 (gVal[0][0],n1_,n_1,n);
  sleft=0;
  p=hyper (min);
  for (i=min+1;p<0.99999999*prob;i++) {
    sleft = sleft+p;
    p=hyper (i);
  }
  i--;
  if (p < 1.00000001*prob)
    sleft = sleft+p;
  else
    i--;
  sright = 0;
  p=hyper (max);
  for (j=max-1;p<0.99999999*prob;j--){
    sright = sright+p;
    p=hyper (j);
  }
  j++;
  if (p < 1.00000001*prob)
    sright = sright+p;
  else
    j++;
  if (abs (i-gVal[0][0]) < abs (j-gVal[0][0])) {
    *sless = sleft;
    *slarg = 1.0-sleft+prob;
  }
  else {
    *sless = 1.0-sright+prob;
    *slarg = sright;
  }
  *twotail = sleft+sright;
  if (*twotail > 1.0)
    *twotail = 1;
  mv_freeMatrixI (gVal);
  return prob;
}
