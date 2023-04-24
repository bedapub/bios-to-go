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
/** @file recipes.c
    @brief Routines from Numerical Recipes,
    The art of scientific computing,
    Press, W.H., Flannery, B.P., Teukolsky, S.A., and Vetterling, W.T.,
    Cambridge University Press 1986.
    Algorithms have been adapted to real C (arrays start at 0).
    Module prefix rcp_
*/
#include <math.h>
#include "log.h"
#include "array.h"
#include "hlrmisc.h"
#include "matvec.h"
#include "recipes.h"

///used in rcp_ran3
#define MBIG 1000000000
///used in rcp_ran3
#define MSEED 161803398
///used in rcp_ran3
#define MZ 0
///used in rcp_ran3
#define FAC (1.0/MBIG)

double rcp_ran3 (int *idum) {
  /**
     Produces random numbers
     @param[in] idum - set to negative value to (re)initialize
                       the sequence of random numbers
     @return uniform random deviate between 0.0 and 1.0
  */
  static int inext,inextp;
  static long ma[56];
  static int iff=0;
  long mj,mk;
  int i,ii,k;

  if (*idum < 0 || iff == 0) {
    iff=1;
    mj=MSEED-(*idum < 0 ? -*idum : *idum);
    mj %= MBIG;
    ma[55]=mj;
    mk=1;
    for (i=1;i<=54;i++) {
      ii=(21*i) % 55;
      ma[ii]=mk;
      mk=mj-mk;
      if (mk < MZ)
        mk += MBIG;
      mj=ma[ii];
    }
    for (k=1;k<=4;k++)
      for (i=1;i<=55;i++) {
        ma[i] -= ma[1+(i+30) % 55];
        if (ma[i] < MZ)
          ma[i] += MBIG;
      }
    inext=0;
    inextp=31;
    *idum=1;
  }
  if (++inext == 56)
    inext=1;
  if (++inextp == 56)
    inextp=1;
  mj=ma[inext]-ma[inextp];
  if (mj < MZ)
    mj += MBIG;
  ma[inext]=mj;
  return mj*FAC;
}

#undef MBIG
#undef MSEED
#undef MZ
#undef FAC

/*-------------------------------------------------------------*/
///used in rcp_irbit1
#define IB1 1
///used in rcp_irbit1
#define IB2 2
///used in rcp_irbit1
#define IB5 16
///used in rcp_irbit1
#define IB18 131072

int rcp_irbit1 (unsigned long *iseed) {
  /**
     Returns as an int a random bit, based on the 18-low-significance
     bits in iseed (which is modified for the next call)
     @param[in] iseed - the seed
     @return random bit
  */
  unsigned long newbit;

  newbit = (*iseed & IB18) >> 17 ^
           (*iseed & IB5) >> 4 ^
           (*iseed & IB2) >> 1 ^
           (*iseed & IB1);
  *iseed=(*iseed << 1) | newbit;
  return (int) newbit;
}

#undef IB1
#undef IB2
#undef IB5
#undef IB18

/*-------------------------------------------------------------*/
/// used in rcp_tqli
#define SIGN(a,b) ((b)<0 ? -fabs(a) : fabs(a))

void rcp_tqli (double d[],double e[],int n,double **z) {
  /**
     Eigensolution of a symmetric tridiagonal matrix
     @param[in] d - diagonal elements of tridiagonal matrix
     @param[in] e - subdiagonal elments of the tridiagonal matrix,
                    e[0] arbitrary
     @param[in] n - dimension of vectors/matrices
     @param[out] d - eigenvalues
     @param[out] e - destroyed
     @param[in] z - if eigenvectors of tridiagonal matrix are desired, z is
                    input as the identity matrix. If the eigenvectors of a
                    matrix that has been reduced by tred2 are required, then
                    z is input as the matrix output by tred2. In either case,
                    the kth column of z returns the normalized eigenvector
                    corresponding to d[k]
  */
  int m,l,iter,i,k;
  double s,r,p,g,f,dd,c,b;

  for (i=1;i<n;i++) e[i-1]=e[i];
  e[n-1]=0.0;
  for (l=0;l<n;l++) {
    iter=0;
    do {
      for (m=l;m<n-1;m++) {
        dd=fabs(d[m])+fabs(d[m+1]);
        if (fabs(e[m])+dd == dd)
          break;
      }
      if (m != l) {
        if (iter++ == 30) {
          romsg ("Too many iterations in TQLI");
          return;
        }
        g=(d[l+1]-d[l])/(2.0*e[l]);
        r=sqrt((g*g)+1.0);
        g=d[m]-d[l]+e[l]/(g+SIGN(r,g));
        s=c=1.0;
        p=0.0;
        for (i=m-1;i>=l;i--) {
          f=s*e[i];
          b=c*e[i];
          if (fabs(f) >= fabs(g)) {
            c=g/f;
            r=sqrt((c*c)+1.0);
            e[i+1]=f*r;
            c *= (s=1.0/r);
          }
          else {
            s=f/g;
            r=sqrt((s*s)+1.0);
            e[i+1]=g*r;
            s *= (c=1.0/r);
          }
          g=d[i+1]-p;
          r=(d[i]-g)*s+2.0*c*b;
          p=s*r;
          d[i+1]=g+p;
          g=c*r-b;
          for (k=0;k<n;k++) {
            f=z[k][i+1];
            z[k][i+1]=s*z[k][i]+c*f;
            z[k][i]=c*z[k][i]-s*f;
          }
        }
        d[l]=d[l]-p;
        e[l]=g;
        e[m]=0.0;
      }
    } while (m != l);
  }
}

/*-------------------------------------------------------------*/

void rcp_tred2 (double **a,int n,double d[],double e[]) {
  /**
     Householder reduction of a real, symmetric matrix
     @param[in] a - the matrix
     @param[in] n - the dimension
     @param[out] a - orthogonal matrix Q effecting the transformation
     @param[out] d - diagonal elements of the tridiagonal matrix
     @param[out] e - off-diagonal elements (e[0] = 0)
  */
  int l,k,j,i;
  double scale,hh,h,g,f;

  for (i=n-1;i>=1;i--) {
    l=i-1;
    h=scale=0.0;
    if (l > 0) {
      for (k=0;k<=l;k++)
        scale += fabs(a[i][k]);
      if (scale == 0.0)
        e[i]=a[i][l];
      else {
        for (k=0;k<=l;k++) {
          a[i][k] /= scale;
          h += a[i][k]*a[i][k];
        }
        f=a[i][l];
        g = f>0 ? -sqrt(h) : sqrt(h);
        e[i]=scale*g;
        h -= f*g;
        a[i][l]=f-g;
        f=0.0;
        for (j=0;j<=l;j++) {
          a[j][i]=a[i][j]/h;
          g=0.0;
          for (k=0;k<=j;k++)
            g += a[j][k]*a[i][k];
          for (k=j+1;k<=l;k++)
            g += a[k][j]*a[i][k];
          e[j]=g/h;
          f += e[j]*a[i][j];
        }
        hh=f/(h+h);
        for (j=0;j<=l;j++) {
          f=a[i][j];
          e[j]=g=e[j]-hh*f;
          for (k=0;k<=j;k++)
            a[j][k] -= (f*e[k]+g*a[i][k]);
        }
      }
    }
    else
      e[i]=a[i][l];
    d[i]=h;
  }
  d[0]=0.0;
  e[0]=0.0;
  for (i=0;i<n;i++) {
    l=i-1;
    if (d[i]) {
      for (j=0;j<=l;j++) {
        g=0.0;
        for (k=0;k<=l;k++)
          g += a[i][k]*a[k][j];
        for (k=0;k<=l;k++)
          a[k][j] -= g*a[k][i];
      }
    }
    d[i]=a[i][i];
    a[i][i]=1.0;
    for (j=0;j<=l;j++) a[j][i]=a[i][j]=0.0;
  }
}

/*-------------------------------------------------------------*/

double rcp_betai (double a,double b,double x) {
  /**
     Incomplete beta function Ix(a,b)
  */
  double bt;

  if (x < 0.0 || x > 1.0)
    die ("rcp_betai: x out of range: %f",x);
  if (x == 0.0 || x == 1.0)
    bt = 0.0;
  else
    bt = exp (rcp_gammln (a+b) - rcp_gammln (a) - rcp_gammln (b) +
              a*log (x) + b*log (1.0-x));
  if (x < (a+1.0)/(a+b+2.0))
    return bt*rcp_betacf (a,b,x)/a;
  else
    return 1.0-bt*rcp_betacf (b,a,1.0-x)/b;
}

double rcp_betacf (double a,double b,double x) {
  /**
     Continued fraction used by BETAI
  */
  int itmax = 100;
  double eps = 3.0e-7;
  double fpmin = 1.0e-30;
  double aa,c,d,del,h,qab,qam,qap;
  int m,m2;

  qab = a+b;
  qap = a+1.0;
  qam = a-1.0;
  c = 1.0;
  d = 1.0-qab*x/qap;
  if (fabs (d) < fpmin)
    d = fpmin;
  d = 1.0/d;
  h = d;
  for (m=1;m<=itmax;m++) {
    m2 = 2*m;
    aa = m*(b-m)*x/((qam+m2)*(a+m2));
    d = 1.0 + aa*d;
    if (fabs (d) < fpmin)
      d = fpmin;
    c = 1.0 + aa/c;
    if (fabs (c) < fpmin)
      c = fpmin;
    d = 1.0/d;
    h *= d*c;
    aa = -(a+m)*(qab+m)*x/((a+m2)*(qap+m2));
    d = 1.0 + aa*d;
    if (fabs (d) < fpmin)
      d = fpmin;
    c = 1.0 + aa/c;
    if (fabs (c) < fpmin)
      c = fpmin;
    d = 1.0/d;
    del = d*c;
    h *= del;
    if (fabs (del-1.0) < eps)
      break;
  }
  if (m > itmax)
    warn ("rcp_betacf: a or b too big, or itmax too small");
  return h;
}

void rcp_gser (double *gamser,double a,double x,double *gln) {
  /**
     Series used by GAMMP and GAMMQ
     @return the incomplete gamma function P(a,x) evaluated by its
             series representation as gamser. Also returns gamma(a) as gln
  */
  int itmax = 100;
  double eps = 3.0e-7;
  int n;
  double del,sum,ap;

  *gln = rcp_gammln (a);
  if (x <= 0.0) {
    if (x < 0.0)
      die ("rcp_gser: x < 0.0");
    *gamser = 0.0;
    return;
  }
  else {
    ap = a;
    del = sum = 1.0/a;
    for (n=1;n<=itmax;n++) {
      ap++;
      del *= x/ap;
      sum += del;
      if (fabs (del) < fabs (sum)*eps) {
        *gamser = sum*exp (-x+a*log (x)-(*gln));
        return;
      }
    }
    die ("rcp_gser: a too large or itmax too small");
  }
}

void rcp_gcf (double *gammcf,double a,double x,double *gln) {
  /**
     Continued fraction used by GAMMP and GAMMQ
     @return the incomplete gamma function Q(a,x) evaluated by its
             continued fraction representation as gammcf. Also returns
             gamma(a) as gln
  */
  int itmax = 100;
  double eps = 3.0e-7;
  double fpmin = 1.0e-30;
  int i;
  double an,b,c,d,del,h;

  *gln = rcp_gammln (a);
  b = x + 1.0 - a;
  c = 1.0/fpmin;
  d = 1.0/b;
  h = d;
  for (i=1;i<=itmax;i++) {
    an = -i*(i-a);
    b += 2.0;
    d = an*d + b;
    if (fabs (d) < fpmin)
      d = fpmin;
    c = b + an/c;
    if (fabs (c) < fpmin)
      c = fpmin;
    d = 1.0/d;
    del = d*c;
    h *= del;
    if (fabs (del - 1.0) < eps)
      break;
  }
  if (i > itmax)
    die ("rcp_gcf: a too large or itmax too small");
  *gammcf = exp (-x + a*log (x) - (*gln))*h;
}

double rcp_gammq (double a,double x) {
  /**
     Complement of incomplete gamma function
  */
  double gamser,gammcf,gln;

  if (x < 0.0 || a <= 0.0)
    die ("rcp_gammq: invalid arguments a=%f or x=%f",a,x);
  if (x < (a+1.0)) {
    rcp_gser (&gamser,a,x,&gln);
    return 1.0-gamser;
  }
  else {
    rcp_gcf (&gammcf,a,x,&gln);
    return gammcf;
  }
}

double rcp_gammln (double xx) {
  /**
     Logarithm of gamma function
  */
  double x,y,tmp,ser;
  int j;
  double cof[6]= {76.18009172947146,-86.50532032941677,24.01409824083091,
                  -1.231739572450155,0.1208650973866179e-2,-0.5395239384953e-5};
  y = x = xx;
  tmp = x+5.5;
  tmp -= (x+0.5)*log (tmp);
  ser = 1.000000000190015;
  for (j=0;j<6;j++) {
    ser += cof[j]/++y;
  }
  return -tmp+log (2.5066282746310005*ser/x);
}

void rcp_ttest (double x1[],int num1,double x2[],int num2,double *t,
                double *prob) {
  /**
     Calculates the ttest, assumes equal variance in both samples
     @param[in] x1,x2 - vectors of values for the 2 conditions with num1 and
                        num2 values
                        @param[in] num1,num2 - length of vectors
     @param[out] t,prob - t-value and p-value (two-tailed) (input NULL if not
                          interested in result) in case of problems (degrees
                          of freedom = 0 or variance = 0, returns 2.0
  */
  double ave1,ave2;
  double var1,var2;
  int df;
  double var;
  double t1;

  rcp_avevar (x1,num1,&ave1,&var1);
  rcp_avevar (x2,num2,&ave2,&var2);
  df = num1+num2-2;
  if (df == 0) {
    if (t != NULL)
      *t = -1.0;
    if (prob != NULL)
      *prob = 2.0;
    return;
  }
  var = ((num1-1)*var1 + (num2-1)*var2)/df;
  if (var == 0.0) {
    if (t != NULL)
      *t = -1.0;
    if (prob != NULL)
      *prob = 2.0;
    return;
  }
  t1 = (ave1-ave2)/sqrt (var*(1.0/num1+1.0/num2));
  if (t != NULL)
    *t = t1;
  if (prob != NULL)
    *prob = rcp_betai (0.5*df,0.5,df/(df+t1*t1));
}

void rcp_ttest_m (double ave1,double ave2,double sd1,double sd2,
                  int num1,int num2,double *t,double *prob) {
  /**
     Calculates the ttest, assumes equal variance in both samples
     @param[in] ave1,ave2 - averages for the two conditions
     @param[in] sd1,sd2 - stddev for the two conditions
     @param[in] num1,num2 - number of values for the two conditions
     @param[out] t,prob - t-value and  p-value (two-tailed) (input NULL if not
                          interested in result) in case of problems (degrees
                          of freedom = 0 or variance = 0, returns 2.0
  */
  int df;
  double var;
  double t1;

  df = num1+num2-2;
  if (df == 0) {
    if (t)
      *t = -1.0;
    if (prob != NULL)
      *prob = 2.0;
    return;
  }
  var = ((num1-1)*sd1*sd1 + (num2-1)*sd2*sd2)/df;
  if (var == 0.0) {
    if (t != NULL)
      *t = -1.0;
    if (prob != NULL)
      *prob = 2.0;
    return;
  }
  t1 = (ave1-ave2)/sqrt (var*(1.0/num1+1.0/num2));
  if (t != NULL)
    *t = t1;
  if (prob != NULL)
    *prob = rcp_betai (0.5*df,0.5,df/(df+t1*t1));
}

void rcp_ttest_welch (double x1[],int num1,double x2[],int num2,
                      double *t,double *prob) {
  /**
     Calculates the ttest, does not assume equal variance in both samples
     @param[in] x1,x2 - values for the two conditions
     @param[in] num1,num2 - number of values for the two conditions
     @param[out] t,prob - t-value and p-value (two-tailed) (input NULL if not
                          interested in result) in case of problems (degrees
                          of freedom = 0 or variance = 0, returns 2.0
  */
  double ave1,ave2;
  double var1,var2;
  double df;
  double t1,d;

  rcp_avevar (x1,num1,&ave1,&var1);
  rcp_avevar (x2,num2,&ave2,&var2);
  d = sqrt (var1/num1 + var2/num2);
  if (d == 0.0) {
    if (t != NULL)
      *t = -1.0;
    if (prob != NULL)
      *prob = 2.0;
    return;
  }
  t1 = (ave1-ave2)/d;
  if (t != NULL)
    *t = t1;
  if (prob != NULL) {
    if (num1 < 2 || num2 < 2) {
      *prob = 2.0;
      return;
    }
    // formula used by Excel
    df = pow (var1/num1 + var2/num2,2) /
         ((var1*var1/(num1*num1))/(num1-1) + (var2*var2/(num2*num2))/(num2-1));
    /* formula from Lothar Sachs, Statistische Auswertungsmethoden
    df = pow (var1/num1 + var2/num2,2) /
         ((var1*var1/(num1*num1))/(num1+1) + (var2*var2/(num2*num2))/(num2+1)) - 2;
    */
    *prob = rcp_betai (0.5*df,0.5,df/(df+t1*t1));
  }
}

void rcp_ttest_welch_m (double ave1,double ave2,double sd1,double sd2,
                        int num1,int num2,double *t,double *prob) {
  /**
     Calculates the ttest, does not assume equal variance in both samples
     @param[in] ave1,ave2 - averages for the two conditions
     @param[in] sd1,sd2 - stddev for the two conditions
     @param[in] num1,num2 - number of values for the two conditions
     @param[in] t,prob - t-value and p-value (two-tailed) (input NULL if not
                         interested in result) in case of problems (degrees
                         of freedom = 0 or variance = 0, returns 2.0
  */
  double df;
  double v1,v2;
  double t1,d;

  v1 = sd1*sd1;
  v2 = sd2*sd2;
  d = sqrt (v1/num1 + v2/num2);
  if (d == 0.0) {
    if (t != NULL)
      *t = -1.0;
    if (prob != NULL)
      *prob = 2.0;
    return;
  }
  t1 = (ave1-ave2)/d;
  if (t != NULL)
    *t = t1;
  if (prob != NULL) {
    if (num1 < 2 || num2 < 2) {
      *prob = 2.0;
      return;
    }
    // formula used by Excel
    df = pow (v1/num1 + v2/num2,2) /
         ((v1*v1/(num1*num1))/(num1-1) + (v2*v2/(num2*num2))/(num2-1));
    /* formula from Lothar Sachs, Statistische Auswertungsmethoden
    df = pow (v1/num1 + v2/num2,2) /
         ((v1*v1/(num1*num1))/(num1+1) + (v2*v2/(num2*num2))/(num2+1)) - 2;
    */
    *prob = rcp_betai (0.5*df,0.5,df/(df+t1*t1));
  }
}

void rcp_ttestPaired (double x1[],double x2[],int num,double *t,double *prob) {
  /**
     Calculates the paired ttest
     @param[in] x1,x2 - vectors with values of the two conditions
     @param[in] num - number of values in both conditions
     @param[out] t,prob - t-value and p-value (two-tailed) (input NULL if not
                          interested in result) in case of problems (degrees
                          of freedom = 0 or variance = 0, returns 2.0
  */
  double d,s;
  double t1;
  int i,df;

  if (num < 2) {
    if (t != NULL)
      *t = -1.0;
    if (prob != NULL)
      *prob = 2.0;
    return;
  }
  d = 0.0;
  for (i=0;i<num;i++)
    d += (x1[i]-x2[i]);
  d /= num;
  s = 0.0;
  for (i=0;i<num;i++)
    s += pow ((x1[i]-x2[i]) - d,2);
  s /= (num-1);
  s = sqrt (s);
  if (s == 0.0) {
    if (t != NULL)
      *t = -1.0;
    if (prob != NULL)
      *prob = 2.0;
    return;
  }
  t1 = d / (s/sqrt (num));
  if (t != NULL)
    *t = t1;
  df = num - 1;
  if (prob != NULL)
    *prob = rcp_betai (0.5*df,0.5,df/(df+t1*t1));
}

void rcp_tutest (double x1[],int num1,double x2[],int num2,
                 double *t,double *prob) {
  /**
     Student's t-test for means, case of unequal variances
     @param[in] x1,x2 - values for the two conditions
     @param[in] num1,num2 - number of values for the two conditions
     @param[out] t,prob - t-value and p-value (two-tailed) (input NULL if not
                          interested in result) in case of problems (degrees
                          of freedom = 0 or variance = 0, returns 2.0
  */
  double ave1,ave2;
  double var1,var2;
  double df;

  rcp_avevar (x1,num1,&ave1,&var1);
  rcp_avevar (x2,num2,&ave2,&var2);
  *t = (ave1-ave2)/sqrt (var1/num1+var2/num2);
  df = (var1/num1+var2/num2)*(var1/num1+var2/num2) /
       ((var1/num1)*(var1/num1) / (num1-1) + (var2/num2)*(var2/num2) / (num2-1));
  *prob = rcp_betai (0.5*df,0.5,df/(df+(*t)*(*t)));
}

void rcp_ftest (double x1[],int num1,double x2[],int num2,
                double *f,double *prob) {
  /**
     F-test for difference of variances (not ANOVA)
     @param[in] x1,x2 - values for the two conditions
     @param[in] num1,num2 - number of values for the two conditions
     @param[out] f,prob - f-value and p-value (two-tailed) (input NULL if not
                          interested in result) in case of problems (degrees
                          of freedom = 0 or variance = 0, returns 2.0
  */
  double ave1,ave2;
  double var1,var2;
  int df1,df2;

  rcp_avevar (x1,num1,&ave1,&var1);
  rcp_avevar (x2,num2,&ave2,&var2);
  if (var1 > var2) {
    if (f !=NULL)
      *f = var1/var2;
    df1 = num1 - 1;
    df2 = num2 - 1;
  }
  else {
    if (f !=NULL)
      *f = var2/var1;
    df1 = num2 - 1;
    df2 = num1 - 1;
  }
  if (prob != NULL) {
    *prob = 2 * rcp_betai (0.5*df2,0.5*df1,df2/(df2+df1*(*f)));
    if (*prob > 1.0)
      *prob = 2.0 - *prob;
  }
}

void rcp_avevar (double x[],int num,double *ave,double *var) {
  /**
     Calculates mean and variance of a data set
     @param[in] x - vector of values
     @param[in] num - number of values
     @param[out] ave - mean value
     @param[out] var - variance
  */
  int j;
  double s;

  *ave = 0.0;
  *var = 0.0;
  for (j=0;j<num;j++)
    *ave += x[j];
  *ave /= num;
  for (j=0;j<num;j++) {
    s = x[j]-(*ave);
    *var += s*s;
  }
  *var /= (num-1);
}

void rcp_moment (double x[],int num,double *ave,double *adev,double *sdev,
                 double *var,double *skew,double *curt) {
  /**
     Calculates moments of a data set
     @param[in] x - vector of values
     @param[in] num - number of values
     @param[out] ave - mean value
     @param[out] adev - average deviation
     @param[out] sdev - standard deviation
     @param[out] var - variance
     @param[out] skew - skew
     @param[out] curt - kurtosis
  */
  double s,p;
  int j;
  double ep = 0.0;

  if (num < 2)
    die ("rcp_moment: at least 2 values required");
  s = 0.0;
  for (j=0;j<num;j++)
    s += x[j];
  *ave = s/num;
  *adev = (*var) = (*skew) = (*curt) = 0.0;
  for (j=0;j<num;j++) {
    *adev += fabs (s=x[j] - (*ave));
    ep += s;
    *var += (p=s*s);
    *skew += (p *= s);
    *curt += (p *= s);
  }
  *adev /= num;
  *var = (*var - ep*ep/num)/(num-1);
  *sdev = sqrt (*var);
  if (*var != 0.0) {
    *skew /= (num * (*var) * (*sdev));
    *curt = (*curt) / (num * (*var) * (*var)) - 3.0;
  }
  else {
    *skew = 999.999;
    *curt = 999.999;
  }
}

/*-------------------------------------------------------------*/

void rcp_eigsrt (double *d,double **v,int n) {
  /**
     Sorts eigenvalues in decending order and rearranges
     the columns of v accordingly
     @param[in] d - eigenvalues (from algorithms jacobi or tqli)
     @param[in] v - eigenvectors (from algorithms jacobi or tqli)
     @param[in] n - dimension
   */
  int i,j,k;
  double p;

  for (i=0;i<n-1;i++) {
    k = i;
    p = d[i];
    for (j=i+1;j<n;j++) {
      if (d[j] >= p) {
        k = j;
        p = d[j];
      }
    }
    if (k != i) {
      d[k] = d[i];
      d[i] = p;
      for (j=0;j<n;j++) {
        p = v[j][i];
        v[j][i] = v[j][k];
        v[j][k] = p;
      }
    }
  }
}

/*-------------------------------------------------------------*/

void rcp_pearsn (double *x,double *y,int n,double *r,double *prob,double *z) {
  /**
     Pearson's correlation between two data sets
     @param[in] x,y - arrays of dim n
     @param[in] n - dimension of x and y
     @param[out] r - correlation coefficient
     @param[out] prob - the significance level at which the null hypothesis of
                        zero correlation is disproved
     @param[out] z - Fisher's z
  */
  double TINY=1.0e-20;
  double ax,ay,xt,yt,sxx,syy,sxy;
  int j;
  double r1;

  ax = ay = 0.0;
  for (j=0;j<n;j++) {
    ax += x[j];
    ay += y[j];
  }
  ax /= n;
  ay /= n;
  sxx = syy = sxy = 0.0;
  for (j=0;j<n;j++) {
    xt = x[j] - ax;
    yt = y[j] - ay;
    sxx += xt*xt;
    syy += yt*yt;
    sxy += xt*yt;
  }
  if (sxx == 0.0 || syy == 0.0)
    r1 = 0.0;
  else
    r1 = sxy / sqrt (sxx*syy);
  if (r != NULL)
    *r = r1;
  if (z != NULL)
    *z = 0.5*log (((1.0+r1) + TINY)/((1.0-r1) + TINY));
  if (prob != NULL) {
    int df;
    double t;

    df = n - 2;
    t = r1*sqrt (df/(((1.0-r1) + TINY)*((1.0+r1) + TINY)));
    *prob = rcp_betai (0.5*df,0.5,df/(df+t*t));
  }
}

/// used in rcp_sort2
#define SWAP(a,b) {swap=(a);(a)=(b);(b)=swap;}
/// used in rcp_sort2
#define M 7
/// used in rcp_sort2
#define NSTACK 50

void rcp_sort2 (int n,double arr[],double brr[]) {
  /**
     Sorts two arrays by quicksort method
     arr is sorted in ascending order and brr is rearranged accordingly
  */
  int i,ir=n-1,j,k,l=0,*istack;
  int jstack = -1;
  double a,b,swap;

  istack = (int *)hlr_calloc (NSTACK,sizeof (int));
  for (;;) {
    if (ir-l < M) {
      for (j=l+1;j<=ir;j++) {
        a = arr[j];
        b = brr[j];
        for (i=j-1;i>=l;i--) {
          if (arr[i] <= a)
            break;
          arr[i+1] = arr[i];
          brr[i+1] = brr[i];
        }
        arr[i+1] = a;
        brr[i+1] = b;
      }
      if (jstack == -1) {
        hlr_free (istack);
        return;
      }
      ir = istack[jstack];
      l = istack[jstack-1];
      jstack -= 2;
    }
    else {
      k=(l+ir) >> 1;
      SWAP(arr[k],arr[l+1])
      SWAP(brr[k],brr[l+1])
      if (arr[l] > arr[ir]) {
        SWAP(arr[l],arr[ir])
        SWAP(brr[l],brr[ir])
      }
      if (arr[l+1] > arr[ir]) {
        SWAP(arr[l+1],arr[ir])
        SWAP(brr[l+1],brr[ir])
      }
      if (arr[l] > arr[l+1]) {
        SWAP(arr[l],arr[l+1])
        SWAP(brr[l],brr[l+1])
      }
      i = l+1;
      j = ir;
      a = arr[l+1];
      b = brr[l+1];
      for (;;) {
        do i++;
        while (arr[i] < a);
        do j--;
        while (arr[j] > a);
        if (j < i)
          break;
        SWAP(arr[i],arr[j])
        SWAP(brr[i],brr[j])
      }
      arr[l+1]=arr[j];
      arr[j]=a;
      brr[l+1]=brr[j];
      brr[j]=b;
      jstack += 2;
      if (jstack >= NSTACK)
        die ("NSTACK too small in rcp_sort2.");
      if (ir-i+1 >= j-l) {
        istack[jstack]=ir;
        istack[jstack-1]=i;
        ir=j-1;
      }
      else {
        istack[jstack]=j-1;
        istack[jstack-1]=l;
        l=i;
      }
    }
  }
}

void rcp_crank (int n,double w[],double *s) {
  /**
     Replaces array elements by their rank
  */
  int j=0,ji,jt;
  double t,rank;

  *s = 0.0;
  while (j < n-1) {
    if (w[j+1] != w[j]) {
      w[j]=j+1;
      ++j;
    }
    else {
      for (jt=j+1;jt<n && w[jt]==w[j];jt++);
      rank = 0.5*(j+jt-1);
      for (ji=j;ji<=(jt-1);ji++)
        w[ji] = rank+1;
      t = jt-j;
      *s += t*t*t-t;
      j = jt;
    }
  }
  if (j == n-1)
    w[n-1] = n;
}

double rcp_erfcc (double x) {
  /**
     Complementary error function, concise routine
     (fractional error less than 1.2e-7)
  */
  double t,z,ans;

  z = fabs (x);
  t = 1.0/(1.0+0.5*z);
  ans = t*exp(-z*z-1.26551223+t*(1.00002368+t*(0.37409196+t*(0.09678418+
               t*(-0.18628806+t*(0.27886807+t*(-1.13520398+t*(1.48851587+
               t*(-0.82215223+t*0.17087277)))))))));
  return x >= 0.0 ? ans : 2.0-ans;
}

void rcp_spear (double *x,double *y,int n,double *d,double *zd,
                double *probd,double *rs,double *probrs) {
  /**
     Spearman's rank correlation between two data sets
     @param[in] x,y - vectors containing values
     @param[in] n - dimension of vectors
     @param[out] d - sum-squared difference of ranks
     @param[out] zd - the number of stddevs by this d deviates from its null
                      hypothesis expected value
     @param[out] probd - the two-sided significance level of this deviation
     @param[out] rs - Spearman's rank correlation
     @param[out] probrs - the two-sided significance level of its deviation
                          from zero
  */
  int j;
  double vard,t,sg,sf,fac,en3n,en,df,aved,*wksp1,*wksp2;
  double rs1,zd1,d1;

  wksp1 = (double *)hlr_calloc (n,sizeof (double));
  wksp2 = (double *)hlr_calloc (n,sizeof (double));
  for (j=0;j<n;j++) {
    wksp1[j]=x[j];
    wksp2[j]=y[j];
  }
  rcp_sort2 (n,wksp1,wksp2);
  rcp_crank (n,wksp1,&sf);
  rcp_sort2 (n,wksp2,wksp1);
  rcp_crank (n,wksp2,&sg);
  d1 = 0.0;
  for (j=0;j<n;j++)
    d1 += (wksp1[j]-wksp2[j])*(wksp1[j]-wksp2[j]);
  if (d != NULL)
    *d = d1;
  en = n;
  en3n = en*en*en-en;
  aved = en3n/6.0-(sf+sg)/12.0;
  fac = (1.0-sf/en3n)*(1.0-sg/en3n);
  vard = ((en-1.0)*en*en*(en+1.0)*(en+1.0)/36.0)*fac;
  zd1 = (d1-aved)/sqrt (vard);
  if (zd != NULL)
    *zd = zd1;
  if (probd != NULL)
    *probd = rcp_erfcc (fabs (zd1)/1.4142136);
  rs1 = (1.0-(6.0/en3n)*(d1+(sf+sg)/12.0))/sqrt(fac);
  if (rs != NULL)
    *rs = rs1;
  if (probrs != NULL) {
    fac = (rs1+1.0)*(1.0-(rs1));
    if (fac > 0.0) {
      t = (rs1)*sqrt ((en-2.0)/fac);
      df = en-2.0;
      *probrs = rcp_betai (0.5*df,0.5,df/(df+t*t));
    }
    else
      *probrs = 0.0;
  }
  hlr_free (wksp2);
  hlr_free (wksp1);
}

/*-------------------------------------------------------------*/

void rcp_contTab (int **val,int num1,int num2,double *chisq,double *prob,
                  double *uygx,double *uxgy,double *likelihoodRatio) {
  /**
   */
  int i, j;
  double tiny = 1e-30;
  double sum = 0.0;
  double expect;
  double p;
  Array sumi; // of int
  Array sumj; // of int
  int df;
  double h,hx,hy;
  double hygx,hxgy;

  sumi = arrayCreate (num1,int);
  sumj = arrayCreate (num2,int);
  for (i=0;i<num1;i++) {
    array (sumi,i,int) = 0;
    for (j=0;j<num2;j++) {
      sum += val[i][j];
      arru (sumi,i,int) += val[i][j];
    }
  }
  for (j=0;j<num2;j++) {
    array (sumj,j,int) = 0;
    for (i=0;i<num1;i++) {
      arru (sumj,j,int) += val[i][j];
    }
  }
  df = num1 * num2 - num1 - num2 + 1;
  *chisq = 0;
  *likelihoodRatio = 1;
  for (i=0;i<num1;i++) {
    for (j=0;j<num2;j++) {
      expect = arru (sumi,i,int) * arru (sumj,j,int) / sum;
      p = val[i][j] - expect;
      *chisq = *chisq + p * p / (expect + tiny);
      p = (expect + tiny) / (tiny + val[i][j]);
      p = exp (log (p) * val[i][j]);
      *likelihoodRatio *= p;
    }
  }
  *prob = rcp_gammq (0.5*df,0.5* (*chisq));
  hx = hy = h = 0;
  for (i=0;i<num1;i++) {
    if (arru (sumi,i,int) != 0) {
      p = (double)arru (sumi,i,int) / sum;
      hx -= p * log (p);
    }
  }
  for (i=0;i<num2;i++) {
    if (arru (sumj,i,int) != 0) {
      p = (double)arru (sumj,i,int) / sum;
      hy -= p * log (p);
    }
  }
  for (i=0;i<num1;i++) {
    for (j=0;j<num2;j++) {
      if (val[i][j] != 0) {
        p = (double)val[i][j] / sum;
        h -= p * log (p);
      }
    }
  }
  hygx = h - hx;
  hxgy = h - hy;
  *uygx = (hy-hygx) / (hy+tiny); // the "dependency" of y on x: uncertainty coefficient of y
  *uxgy = (hx-hxgy) / (hx+tiny); // the "dependency" of x on y: uncertainty coefficient of x
  /* uxy = 2 * (hx+hy-h) / (hx+hy+tiny); the uncertainty coefficient when treating x and y symmetrically
   */
  arrayDestroy (sumi);
  arrayDestroy (sumj);
}

/*-------------------------------------------------------------*/

static void sortAscending (double x[], int num) {
  /**
     Sorts an array in ascending order
  */
  double tmp;
  int i,k;

  for (i=0;i<num-1;i++)
    for (k=i+1;k<num;k++)
      if (x[i] > x[k]) {
        tmp = x[i];
        x[i] = x[k];
        x[k] = tmp;
      }
}

double rcp_probks (double alam) {
  /**
   */
  double eps1 = 0.001;
  double eps2 = 1.0e-8;
  double a2,fac;
  double termbf,term;
  int j;
  double val;

  a2 = -2.0 * alam * alam;
  fac = 2.0;
  val = 0.0;
  termbf = 0.0;
  for (j=1;j<=100;j++) {
    term = fac * exp (a2 * j * j);
    val += term;
    if ((fabs (term) < eps1 * termbf) || (fabs (term) < eps2 * val))
      return val;
    fac = -fac;
    termbf = fabs (term);
  }
  return 1.0;
}

void rcp_ks (double x1[],int n1,double x2[],int n2,double *ks,double *prob) {
  /**
     Kolmogorov-Smirnov statistics
     @param[in] x1,x2 - 2 sets of n1 and n2 input values
     @param[in] n1,n2 - number of values
     @param[out] ks - the ks statistic
     @param[out] prob - the pValue
  */
  double en1,en2;
  int j1,j2;
  double fo1,fo2,fn1,fn2;
  double dt;
  double ks1;

  sortAscending (x1,n1);
  sortAscending (x2,n2);
  en1 = n1;
  en2 = n2;
  j1 = j2 = 0;
  fo1 = fo2 = 0.0;
  ks1 = 0.0;
lab1:
  if (j1 < n1 && j2 < n2) {
    if (x1[j1] < x2[j2]) {
      fn1 = j1/en1;
      dt = MAX (fabs (fn1-fo2),fabs (fo1-fo2));
      if (dt > ks1)
        ks1 = dt;
      fo1 = fn1;
      j1++;
    }
    else {
      fn2 = j2/en2;
      dt = MAX (fabs (fn2-fo1),fabs (fo2-fo1));
      if (dt > ks1)
        ks1 = dt;
      fo2 = fn2;
      j2++;
    }
    goto lab1;
  }
  if (ks != NULL)
    *ks = ks1;
  if (prob != NULL)
    *prob = rcp_probks (sqrt (en1*en2/(en1+en2))*ks1);
}

/*-------------------------------------------------------------*/
static float sign (float a,float b) {
  if (b >= 0.0)
    return fabs (a);
  return -fabs (a);
}

static float pythag (float a,float b) {
  float at,bt;

  at = fabs (a);
  bt = fabs (b);
  if (at > bt)
    return at * sqrt (1.0 + (bt/at) * (bt/at));
  else {
    if (bt == 0.0)
      return 0.0;
    else
      return bt * sqrt (1.0 + (at/bt) * (at/bt));
  }
}

void rcp_svdcmp (float **a,int m,int n,float *w,float **v) {
  /**
    Given a matrix a with dimensions m by n, this routine computes its
    singular value decomposition a = u * w * vtranspose.
    The matrix u replaces a on output. The diagonal matrix of singular values w
    is output as a vector W. v (not the transpose vt) is output as V.
    m must be greater or equal to n. If it is smaller then a should be filled up
    to square with zero rows.<br>
    Changing everything to float seems to make a big difference in some cases
    (rounding errors???)
    @param[in] a - matrix of dimensions m * n
    @param[in] m - number of rows
    @param[in] n - number of columns
    @param[out] w - vector of dimension n
    @param[out] v - matrix of dimensions n * n
  */
  float c,f,g,h,s,x,y,z;
  int i,j,k,l;
  int its,flag;
  int jj,nm;
  float scale,anorm;
  float *rv1;

  rv1 = (float *)hlr_malloc (n * sizeof (float));
  g = scale = anorm = 0.0;
  for (i=0;i<n;i++) {
    l = i+1;
    rv1[i] = scale * g;
    g = s = scale = 0.0;
    if (i < m) {
      for (k=i;k<m;k++)
        scale += fabs (a[k][i]);
      if (scale != 0.0) {
        for (k=i;k<m;k++) {
          a[k][i] /= scale;
          s += a[k][i] * a[k][i];
        }
        f = a[i][i];
        g = -sign (sqrt (s),f);
        h = f * g - s;
        a[i][i] = f - g;
        for (j=l;j<n;j++) {
          for (s=0.0,k=i;k<m;k++)
            s += a[k][i] * a[k][j];
          f = s/h;
          for (k=i;k<m;k++)
            a[k][j] += f * a[k][i];
        }
        for (k=i;k<m;k++)
          a[k][i] *= scale;
      }
    }
    w[i] = scale * g;
    g = s = scale = 0.0;
    if ((i < m) && (i != n-1)) {
      for (k=l;k<n;k++)
        scale += fabs (a[i][k]);
      if (scale != 0.0) {
        for (k=l;k<n;k++) {
          a[i][k] /= scale;
          s += a[i][k] * a[i][k];
        }
        f = a[i][l];
        g = -sign (sqrt (s),f);
        h = f * g - s;
        a[i][l] = f-g;
        for (k=l;k<n;k++)
          rv1[k] = a[i][k]/h;
        for (j=l;j<m;j++) {
          for (s = 0.0,k=l;k<n;k++)
            s += a[j][k] * a[i][k];
          for (k=l;k<n;k++)
            a[j][k] += s * rv1[k];
        }
        for (k=l;k<n;k++)
          a[i][k] *= scale;
      }
    }
    anorm = MAX (anorm,fabs (w[i])+fabs (rv1[i]));
  }
  // accumulation of right-hand transformations
  for (i=n-1;i>=0;i--) {
    if (i < n-1) {
      if (g != 0.0) {
        for (j=l;j<n;j++)
          v[j][i] = (a[i][j] / a[i][l]) / g;
        for (j=l;j<n;j++) {
          for (s = 0.0,k=l;k<n;k++)
            s += a[i][k] * v[k][j];
          for (k=l;k<n;k++)
            v[k][j] += s * v[k][i];
        }
      }
      for (j=l;j<n;j++)
        v[i][j] = v[j][i] = 0.0;
    }
    v[i][i] = 1.0;
    g = rv1[i];
    l = i;
  }
  // accumulation of left-hand transformations
  for (i=MIN (m,n)-1;i>=0;i--) {
    l = i+1;
    g = w[i];
    for (j=l;j<n;j++)
      a[i][j] = 0.0;
    if (g != 0.0) {
      g = 1.0/g;
      for (j=l;j<n;j++) {
        for (s = 0.0,k=l;k<m;k++)
          s += a[k][i] * a[k][j];
        f = (s/a[i][i]) * g;
        for (k=i;k<m;k++)
          a[k][j] += f * a[k][i];
      }
      for (j=i;j<m;j++)
        a[j][i] *= g;
    }
    else {
      for (j=i;j<m;j++)
        a[j][i] = 0.0;
    }
    a[i][i] += 1.0;
  }
  // diagonalization of the bidiagonal form
  for (k=n-1;k>=0;k--) {
    for (its=0;its<30;its++) {
      flag = 1;
      for (l=k;l>=0;l--) {
        nm = l-1; // note that rv1[0] is always zero
        if ((float)(fabs (rv1[l]) + anorm) == anorm) {
          flag = 0;
          break;
        }
        if ((float)(fabs (w[nm]) + anorm) == anorm)
          break;
      }
      if (flag) {
        c = 0.0; // cancellation of rv1[l],if l > 0
        s = 1.0;
        for (i=l;i<=k;i++) {
          f = s * rv1[i];
          rv1[i] = c*rv1[i];
          if ((float)(fabs(f) + anorm) == anorm)
            break;
          g = w[i];
          h = pythag (f,g);
          w[i] = h;
          h = 1.0/h;
          c = g * h;
          s = -f*h;
          for (j=0;j<m;j++) {
            y = a[j][nm];
            z = a[j][i];
            a[j][nm] = y*c + z*s;
            a[j][i] = -y*s + z*c;
          }
        }
      }
      z = w[k];
      if (l == k) { // convergence
        if (z < 0.0) { // singular value is made nonnegative
          w[k] = -z;
          for (j=0;j<n;j++)
            v[j][k] = -v[j][k];
        }
        break;
      }
      if (its == 29)
        die ("rcp_svdcmp: no convergence in 30 iterations");
      x = w[l]; // shift from bottom 2-by-2 minor
      nm = k-1;
      y = w[nm];
      g = rv1[nm];
      h = rv1[k];
      f = ((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
      g = pythag (f,1.0);
      f = ((x-z)*(x+z)+h*((y/(f+sign (g,f)))-h))/x;
      // next qr transformation
      c = s = 1.0;
      for (j=l;j<=nm;j++) {
        i = j+1;
        g = rv1[i];
        y = w[i];
        h = s*g;
        g = c*g;
        z = pythag (f,h);
        rv1[j] = z;
        c = f/z;
        s = h/z;
        f = x*c + g*s;
        g = -x*s + g*c;
        h = y*s;
        y *= c;
        for (jj=0;jj<n;jj++) {
          x = v[jj][j];
          z = v[jj][i];
          v[jj][j] = x*c + z*s;
          v[jj][i] = -x*s + z*c;
        }
        z = pythag (f,h);
        w[j] = z; // rotation can be arbitrary if z=0
        if (z != 0.0) {
          z = 1.0/z;
          c = f*z;
          s = h*z;
        }
        f = c*g + s*y;
        x = -s*g + c*y;
        for (jj=0;jj<m;jj++) {
          y =a[jj][j];
          z =a[jj][i];
          a[jj][j] = y*c + z*s;
          a[jj][i] = -y*s + z*c;
        }
      }
      rv1[l] = 0.0;
      rv1[k] = f;
      w[k] = x;
    }
  }
  hlr_free (rv1);
}

/****************************************************************************/

void rcp_mrqMin (float x[],float y[],float sig[],int ndata,float a[],int ia[],
                 int ma,float **covar,float **alpha,float *chiSq,
                 void (*funcs)(float,float[],float *,float[],int),
                 float *aLambda) {
  /**
   */
  int j,k,l;
  static int mfit;
  static float oChiSq,*aTry,*beta,*dA,**onedA;

  if (*aLambda < 0.0 ) {
    aTry = mv_vectorF (ma);
    beta = mv_vectorF (ma);
    dA = mv_vectorF (ma);
    for (mfit=0,j=0;j<ma;j++)
      if (ia[j] >= 0)
        mfit++;
    onedA = mv_matrixF (mfit,1);
    *aLambda = 0.001;
    rcp_mrqCof (x,y,sig,ndata,a,ia,ma,alpha,beta,chiSq,funcs);
    oChiSq = *chiSq;
    for (j=0;j<ma;j++)
      aTry[j] = a[j];
  }
  for (j=0;j<mfit;j++) {
    for (k=0;k<mfit;k++)
      covar[j][k] = alpha[j][k];
    covar[j][j] = alpha[j][j] * (1.0 + *aLambda);
    onedA[j][0] = beta[j];
  }
  rcp_gaussJ (covar,mfit,onedA,1);
  for (j=0;j<mfit;j++)
    dA[j] = onedA[j][0];
  if (*aLambda == 0.0) {
    rcp_covSrt (covar,ma,ia,mfit);
    mv_freeMatrixF (onedA);
    mv_freeVectorF (dA);
    mv_freeVectorF (beta);
    mv_freeVectorF (aTry);
    return;
  }
  for (j=0,l=0;l<mfit;l++)
    if (ia[l] >= 0)
      aTry[l] = a[l] + dA[j++];
  rcp_mrqCof (x,y,sig,ndata,aTry,ia,ma,covar,dA,chiSq,funcs);
  if (*chiSq < oChiSq) {
    *aLambda *= 0.1;
    oChiSq = *chiSq;
    for (j=0;j<mfit;j++) {
      for (k=0;k<mfit;k++)
        alpha[j][k] = covar[j][k];
      beta[j] = dA[j];
    }
    for (l=0;l<ma;l++)
      a[l] = aTry[l];
  }
  else {
    *aLambda *= 10.0;
    *chiSq = oChiSq;
  }
}

void rcp_mrqCof (float x[],float y[],float sig[],int nData,float a[],int ia[],
                 int ma,float **alpha,float beta[],float *chiSq,
                 void (*funcs)(float,float[],float *,float[],int)) {
  /**
   */
  int i,j,k,l,m,mfit=0;
  float ymod,wt,sig2i,dy,*dyda;

  dyda = mv_vectorF (ma);
  for (j=0;j<ma;j++)
    if (ia[j] >= 0)
      mfit++;
  for (j=0;j<mfit;j++) {
    for (k=0;k<=j;k++)
      alpha[j][k] = 0.0;
    beta[j] = 0.0;
  }
  *chiSq = 0.0;
  for (i=0;i<nData;i++) {
    (*funcs) (x[i],a,&ymod,dyda,ma);
    sig2i = 1.0 / (sig[i] * sig[i]);
    dy = y[i] - ymod;
    for (j=-1,l=0;l<mfit;l++) {
      if (ia[l] >= 0) {
        wt = dyda[l] * sig2i;
        for (j++,k=0,m=0;m<=l;m++)
          if (ia[m] >= 0)
            alpha[j][k++] += wt * dyda[m];
        beta[j] += wt * dy;
      }
    }
    *chiSq += dy * dy * sig2i;
  }
  for (j=1;j<mfit;j++)
    for (k=0;k<j;k++)
      alpha[k][j] = alpha[j][k];
  mv_freeVectorF (dyda);
}

void rcp_covSrt (float **covar,int ma,int ia[],int mfit) {
  /**
   */
  int i,j,k;
  float swap;

  for (i=mfit;i<ma;i++)
    for (j=0;j<=i;j++)
      covar[i][j] = covar[j][i] = 0.0;
  k = mfit-1;
  for (j=ma-1;j>=0;j--) {
    if (ia[j] >= 0) {
      for (i=0;i<ma;i++)
        SWAP (covar[i][k],covar[i][j]);
      for (i=0;i<ma;i++)
        SWAP (covar[k][i],covar[j][i]);
      k--;
    }
  }
}

void rcp_gaussJ (float **a,int n,float **b,int m) {
  /**
   */
  int *iPiv,*indxR,*indxC;
  int i,j,k,l,ll,iRow=0,iCol=0;
  float big,dummy,pivInv,swap;

  indxC = mv_vectorI (n);
  indxR = mv_vectorI (n);
  iPiv = mv_vectorI (n);
  for (j=0;j<n;j++)
    iPiv[j] = -1;
  for (i=0;i<n;i++) {
    big = 0.0;
    for (j=0;j<n;j++) {
      if (iPiv[j] != 0) {
        for (k=0;k<n;k++) {
          if (iPiv[k] == -1) {
            if (fabs (a[j][k]) >= big) {
              big = fabs (a[j][k]);
              iRow = j;
              iCol = k;
            }
          }
          else if (iPiv[k] > 0) {
            romsg ("rcp_gaussj: Singular matrix-1");
            mv_freeVectorI (iPiv);
            mv_freeVectorI (indxR);
            mv_freeVectorI (indxC);
            return;
          }
        }
      }
    }
    iPiv[iCol]++;

    if (iRow != iCol) {
      for (l=0;l<n;l++)
        SWAP (a[iRow][l],a[iCol][l]);
      for (l=0;l<m;l++)
        SWAP (b[iRow][l],b[iCol][l]);
    }
    indxR[i] = iRow;
    indxC[i] = iCol;
    if (a[iCol][iCol] == 0.0) {
      romsg ("gaussj: Singular matrix-2");
      mv_freeVectorI (iPiv);
      mv_freeVectorI (indxR);
      mv_freeVectorI (indxC);
      return;
    }
    pivInv = 1.0 / a[iCol][iCol];
    a[iCol][iCol] = 1.0;
    for (l=0;l<n;l++)
      a[iCol][l] *= pivInv;
    for (l=0;l<m;l++)
      b[iCol][l] *= pivInv;

    for (ll=0;ll<n;ll++) {
      if (ll != iCol) {
        dummy = a[ll][iCol];
        a[ll][iCol] = 0.0;
        for (l=0;l<n;l++)
          a[ll][l] -= a[iCol][l] * dummy;
        for (l=0;l<m;l++)
          b[ll][l] -= b[iCol][l] * dummy;
      }
    }
  }

  for (l=n-1;l>=0;l--) {
    if (indxR[l] != indxC[l]) {
      for (k=0;k<n;k++)
        SWAP (a[k][indxR[l]],a[k][indxC[l]]);
    }
  }
  mv_freeVectorI (iPiv);
  mv_freeVectorI (indxR);
  mv_freeVectorI (indxC);
}
