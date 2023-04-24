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
/** @file geometry.c
    @brief Various geometrical calculations
    Module prefix geom_
*/
#include <math.h>
#include "log.h"
#include "hlrmisc.h"
#include "recipes.h"
#include "geometry.h"

void geom_diff (float x[],float y[],float z[]) {
  /**
     Caluculates the difference between two vectors
     @param[in] x - first vector
     @param[in] y - second vector
     @param[out] z - difference
  */
  z[0] = x[0] - y[0];
  z[1] = x[1] - y[1];
  z[2] = x[2] - y[2];
}

float geom_dot (float x[],float y[]) {
  /**
     Caluculates the dot product of two vectors
     @param[in] x - first vector
     @param[in] y - second vetor
     @return the dot product
  */
  return x[0]*y[0] + x[1]*y[1] + x[2]*y[2];
}

void geom_cross (float x[],float y[],float z[]) {
  /**
     Caluculates the cross product of two vectors
     @param[in] x - first vector
     @param[in] y - second vetor
     @param[out] z - the dot product
  */
  z[0] = x[1]*y[2] - y[1]*x[2];
  z[1] = x[2]*y[0] - y[2]*x[0];
  z[2] = x[0]*y[1] - y[0]*x[1];
}

float geom_norm (float x[]) {
  /**
     Calculates the norm of a vector
     @param[in] x - the vector
     @return the norm
  */
  float xnorm;

  xnorm = x[0]*x[0] + x[1]*x[1] + x[2]*x[2];
  if (xnorm > 0.0) {
    xnorm = sqrt (xnorm);
    x[0] /= xnorm;
    x[1] /= xnorm;
    x[2] /= xnorm;
  }
  return xnorm;
}

float geom_dihedralAngle (float v1[],float v2[],float v3[],float v4[]) {
  /**
     Calculates the dihedral angle of 4 points
     @param[in] v1, v2, v3, v4 - the points
     @return the dihedral angle
  */
  float uu,vv;
  float v12[3],v43[3],x[3],y[3],z[3],p[3];
  float d;

  geom_diff (v1,v2,v12);
  geom_diff (v4,v3,v43);
  geom_diff (v2,v3,z);
  geom_cross (z,v12,p);
  geom_cross (z,v43,x);
  geom_cross (z,x,y);
  uu = geom_dot (x,x);
  vv = geom_dot (y,y);
  d = 360.0;
  if (uu > 0.0 && vv > 0.0) {
    uu = geom_dot (p,x) / sqrt (uu);
    vv = geom_dot (p,y) / sqrt (vv);
    if (uu != 0.0 || vv != 0.0)
      d = atan2 (vv,uu) * RADIAN;
  }
  return d;
}

float geom_cosAngle (float v1[],float v2[],float v3[],float v4[]) {
  /**
     Calculates the cosine of 4 points
     @param[in] v1, v2, v3, v4 - the points
     @return the cosine
  */
  float uu[3],vv[3];
  float x;

  geom_diff (v1,v2,uu);
  geom_diff (v3,v4,vv);
  x = geom_dot (uu,uu) * geom_dot (vv,vv);
  if (x > 0.0)
    return geom_dot (uu,vv) / sqrt (x);
  return 0.0;
}

float geom_distance (float u1[],float v1[]) {
  /**
     Calculates the distance between two points
     @param[in] u1, v1 - the points
     @return the distance
  */
  return sqrt ((u1[0]-v1[0])*(u1[0]-v1[0]) +
               (u1[1]-v1[1])*(u1[1]-v1[1]) +
               (u1[2]-v1[2])*(u1[2]-v1[2]));
}

float geom_distsq (float u1[],float v1[]) {
  /**
     Calculates the square of the distance between two points
     @param[in] u1, v1 - the points
     @return the squared distance
  */
  return (u1[0]-v1[0])*(u1[0]-v1[0]) +
    (u1[1]-v1[1])*(u1[1]-v1[1]) +
    (u1[2]-v1[2])*(u1[2]-v1[2]);
}

void geom_rotateAbout (float a[],float deg,float u[]) {
  /**
     Rotates a vector around an axis
     @param[in] a - the axis
     @param[in] deg - by how many degrees
     @param[in] u - the vector
     @param[out] u - the rotated vector
  */
  float rot[3][3];
  float uTemp[3];
  int i,k;

  rot[0][0] = a[0]*a[0] + cos (deg/RADIAN)*(1.0 - a[0]*a[0]);
  rot[0][1] = a[0]*a[1]*(1.0-cos (deg/RADIAN)) - a[2]*sin (deg/RADIAN);
  rot[0][2] = a[2]*a[0]*(1.0-cos (deg/RADIAN)) + a[1]*sin (deg/RADIAN);
  rot[1][0] = a[0]*a[1]*(1.0-cos (deg/RADIAN)) + a[2]*sin (deg/RADIAN);
  rot[1][1] = a[1]*a[1] + cos (deg/RADIAN)*(1.0 - a[1]*a[1]);
  rot[1][2] = a[1]*a[2]*(1.0-cos (deg/RADIAN)) - a[0]*sin (deg/RADIAN);
  rot[2][0] = a[2]*a[0]*(1.0-cos (deg/RADIAN)) - a[1]*sin (deg/RADIAN);
  rot[2][1] = a[1]*a[2]*(1.0-cos (deg/RADIAN)) + a[0]*sin (deg/RADIAN);
  rot[2][2] = a[2]*a[2] + cos (deg/RADIAN)*(1.0 - a[2]*a[2]);

  for (i=0;i<3;i++) {
    uTemp[i] = 0.0;
    for (k=0;k<3;k++)
      uTemp[i] += rot[i][k] * u[k];
  }
  for (i=0;i<3;i++)
    u[i] = uTemp[i];
}

float geom_bondAngle (float u1[],float u2[],float u3[]) {
  /**
     Calculates the angle between two bonds (three atoms)
     @param[in] u1, u2, u3 - the atoms forming bonds u1-u2 and u3-u2
     @return the angle
  */
  float vec1[3],vec2[3];

  geom_diff (u1,u2,vec1);
  geom_norm (vec1);
  geom_diff (u3,u2,vec2);
  geom_norm (vec2);

  return acos (geom_dot (vec1,vec2))*RADIAN;
}

void geom_matMult4x4 (float a[4][4],float b[4][4],float c[4][4]) {
  /**
     Multiplies two 4x4 matrices
     @param[in] a, b - the matrices
     @param[out] c - the product
  */
  int i,k,l;

  for (i=0;i<4;i++) {
    for (k=0;k<4;k++) {
      c[i][k] = 0.0;
      for (l=0;l<4;l++)
        c[i][k] += a[i][l] * b[l][k];
    }
  }
}

void geom_matId4x4 (float a[4][4]) {
  /**
     Creates a 4x4 identity matrix
     @param[out] a - the identity matrix
  */
  int i,k;

  for (i=0;i<4;i++) {
    for (k=0;k<4;k++) {
      if (i != k)
        a[i][k] = 0.0;
      else
        a[i][k] = 1.0;
    }
  }
}

/// very small value
#define TINY 1.0e-20;
/// dimension of the matrices
#define DIM 4

static void ludcmp (float a[DIM][DIM],int indx[DIM],float *d) {
  /**
     Function from Numerical Recipes,
     The art of scientific computing,
     Press, W.H., Flannery, B.P., Teukolsky, S.A., and Vetterling, W.T.,
     Cambridge University Press 1986
  */
  int i,j,k,imax=0;
  float sum,dum,big,temp;
  float vv[DIM];

  *d = 1.0;
  for (i=0;i<DIM;i++) {
    big = 0.0;
    for (j=0;j<DIM;j++)
      if ((temp = fabs (a[i][j])) > big)
        big = temp;
    if (big == 0.0)
      romsg ("Singular matrix");
    vv[i] =  1.0 / big;
  }
  for (j=0;j<DIM;j++) {
    for (i=0;i<j;i++) {
      sum = a[i][j];
      for (k=0;k<i;k++)
        sum -= a[i][k]*a[k][j];
      a[i][j] = sum;
    }
    big = 0.0;
    for (i=j;i<DIM;i++) {
      sum = a[i][j];
      for (k=0;k<j;k++)
        sum -= a[i][k]*a[k][j];
      a[i][j] = sum;
      if ((dum = vv[i]*fabs(sum)) >= big) {
        big = dum;
        imax = i;
      }
    }
    if (j != imax) {
      for (k=0;k<DIM;k++) {
        dum = a[imax][k];
        a[imax][k] = a[j][k];
        a[j][k] = dum;
      }
      *d = -(*d);
      vv[imax] = vv[j];
    }
    indx[j] = imax;
    if (a[j][j]== 0.0) a[j][j] = TINY;
    if (j!=DIM-1) {
      dum = 1.0 / a[j][j];
      for (i=j+1;i<DIM;i++)
        a[i][j] *= dum;
    }
  }
}

static void lubksb (float a[DIM][DIM],int indx[DIM],float b[]) {
  /**
     Function from Numerical Recipes,
     The art of scientific computing,
     Press, W.H., Flannery, B.P., Teukolsky, S.A., and Vetterling, W.T.,
     Cambridge University Press 1986
  */
  int i,ii = -1,j,ip;
  float sum;

  for (i=0;i<DIM;i++) {
    ip = indx[i];
    sum = b[ip];
    b[ip] = b[i];
    if (ii != -1) {
      for (j=ii;j<=i-1;j++)
        sum -= a[i][j]*b[j];
    }
    else if (sum != 0.0)
      ii = i;
    b[i] = sum;
  }
  for (i=DIM-1;i>=0;i--) {
    sum = b[i];
    for (j=i+1;j<DIM;j++)
      sum -= a[i][j]*b[j];
    b[i] = sum / a[i][i];
  }
}

void geom_matInv4x4 (float inv[4][4],float old[4][4]) {
  /**
     Inverts a 4x4 matrix
     @param[in] old - the original matrix
     @param[out] inv - the inverted matrix
  */
  float d;
  int indx[DIM];
  float col[DIM];
  int i,j;
  float o1[DIM][DIM];

  for (i=0;i<DIM;i++)
    for (j=0;j<DIM;j++)
      o1[i][j] = old[i][j];
  ludcmp (o1,indx,&d);
  for (j=0;j<DIM;j++) {
    for (i=0;i<DIM;i++)
      col[i] = 0.0;
    col[j] = 1.0;
    lubksb (o1,indx,col);
    for (i=0;i<DIM;i++)
      inv[i][j] = col[i];
  }
}

float geom_kabschRBM (int nat,float **xm1,float **xm2,float *wgt,
                      float *sh1,float rt[3][3],float *sh2) {
  /**
     Kabsch rigid body match of two structures.<br>
     With help from Paul Gerber (www.moloc.ch)
     @param[in] nat - number of atoms
     @param[in] xm1, xm2 - the corresponding nat points of both structures
     @param[in] wgt - the weights for each point (usually 1.0)
     @param[out] sh1 - translation matrix 1
     @param[out] rt - rotation matrix
     @param[out] sh2 - translation matrix 2
     @return rsd (positive) or -1.0 if problem 
  */
  int i,k,n;
  int i1,i2,i3;
  int k2,k3;
  int ir[3];
  int tempIr;
  float ur,wsum;
  float umx[3][3];
  float eval[3];
  double **evcPtr;
  double *evc;
  double **utuPtr;
  double *utu;
  float uvc[3][3];
  float acc = 1.0e-6;
  float bnorm;
  float sqrev;
  float rsd;
  float r1,r2,r12;
  float x1,x2,y;
  double d[3],e[3];

  evcPtr = hlr_calloc (3,sizeof (double *));
  evc = hlr_calloc (3*3,sizeof (double));
  for (i=0;i<3;i++)
    evcPtr[i] = &evc[3*i];
  utuPtr = hlr_calloc (3,sizeof (double *));
  utu = hlr_calloc (3*3,sizeof (double));
  for (i=0;i<3;i++)
    utuPtr[i] = &utu[3*i];
  wsum = 0.0;
  for (n=0;n<nat;n++)
    wsum += wgt[n];
  for (n=0;n<3;n++) {
    sh1[n] = 0.0;
    sh2[n] = 0.0;
  }
  r1 = r2 = 0.0;
  for (n=0;n<nat;n++) {
    for (i=0;i<3;i++) {
      sh1[i] += wgt[n] * xm1[n][i];
      sh2[i] += wgt[n] * xm2[n][i];
      r1 += wgt[n] * xm1[n][i] * xm1[n][i];
      r2 += wgt[n] * xm2[n][i] * xm2[n][i];
    }
  }
  for (i=0;i<3;i++) {
    sh1[i] /= wsum;
    sh2[i] /= wsum;
  }
  x1 = x2 = 0.0;
  for (i=0;i<3;i++) {
    x1 += sh1[i] * sh1[i];
    x2 += sh2[i] * sh2[i];
  }
  r1 = r1 / wsum - x1;
  r2 = r2 / wsum - x2;
  y = nat * 1.0e-13;
  if (r1 < x1 * sqrt (y))
    r1 = 0.0;
  if (r2 < x2 * sqrt (y))
    r2 = 0.0;
  for (i=0;i<3;i++) {
    for (k=0;k<3;k++) {
      ur = 0;
      for (n=0;n<nat;n++)
        ur += wgt[n] * (xm1[n][i] - sh1[i]) * (xm2[n][k] - sh2[k]);
      umx[k][i] = ur / wsum;
    }
  }
  for (i=0;i<3;i++) {
    for (k=0;k<3;k++) {
      utuPtr[i][k] = 0.0;
      for (n=0;n<3;n++)
        utuPtr[i][k] += umx[n][i] * umx[n][k];
    }
  }
  rcp_tred2 (utuPtr,3,d,e);
  for (i=0;i<3;i++)
    for (k=0;k<3;k++)
      evcPtr[i][k] = utuPtr[i][k];
  rcp_tqli (d,e,3,evcPtr);
  for (i=0;i<3;i++)
   eval[i] = d[i];
  for (i=0;i<3;i++)
    ir[i] = i;
  for (i=0;i<2;i++) {
    for (k=i+1;k<3;k++) {
      if (eval[k] < eval[i]) {
        tempIr = ir[i];
        ir[i] = ir[k];
        ir[k] = tempIr;
      }
    }
  }
  for (i=0;i<3;i++) {
    for (k=0;k<3;k++) {
      uvc[i][k] = 0.0;
      for (n=0;n<3;n++)
        uvc[i][k] += umx[i][n] * evcPtr[n][k];
    }
  }
  i1 = ir[2];
  i2 = ir[1];
  i3 = ir[0];
  if (eval[i1] != eval[i1]) // test for NaN
    return -1.0;
  sqrev = sqrt (eval[i1]);
  for (k=0;k<3;k++)
    uvc[k][i1] = uvc[k][i1] / sqrev;
  if (eval[i2] <= eval[i1] * acc) {
    romsg ("geom_kabschRbm: Linear molecule");
    for (k=0;k<3;k++) {
      k2 = ((k+1) % 3);
      k3 = ((k+2) % 3);
      uvc[k][i2] = uvc[k2][i1] * evcPtr[k3][i1] -
                   uvc[k3][i1] * evcPtr[k2][i1];
    }
    bnorm = 0.0;
    for (i=0;i<3;i++)
      bnorm += uvc[i][i2] * uvc[i][i2];
    if (bnorm <= acc) {
      romsg ("geom_kabschRbm: Molecules already aligned");
      for (i=0;i<3;i++)
        uvc[i][i2] = evcPtr[i][i2];
    }
    else {
      romsg ("Rigid body match: Molecules not aligned");
      bnorm = sqrt (bnorm);
      for (i=0;i<3;i++) {
        uvc[i][i2] = uvc[i][i2] / bnorm;
        evcPtr[i][i2] = uvc[i][i2];
      }
    }
    return -1.0;
  }
  else { // both molecules not linear
    sqrev = sqrt (eval[i2]);
    for (k=0;k<3;k++)
      uvc[k][i2] = uvc[k][i2] / sqrev;
  }
  for (k=0;k<3;k++) {
    k2 = ((k+1) % 3);
    k3 = ((k+2) % 3);
    uvc[k][i3] = uvc[k2][i1] * uvc[k3][i2] -
                 uvc[k3][i1] * uvc[k2][i2];
    evcPtr[k][i3] = evcPtr[k2][i1] * evcPtr[k3][i2] -
                    evcPtr[k3][i1] * evcPtr[k2][i2];
  }
  for (i=0;i<3;i++) {
    for (k=0;k<3;k++) {
      rt[i][k] = 0.0;
      for (n=0;n<3;n++)
        rt[i][k] += uvc[i][n] * evcPtr[k][n];
    }
  }
  r12 = 0.0;
  for (i=0;i<3;i++)
    for (k=0;k<3;k++)
      r12 += umx[i][k] * rt[i][k];
  rsd = r1 + r2 - 2.0 * r12;
  if (rsd > 0.0)
    rsd = sqrt (rsd);
  else
    return -1.0;
  hlr_free (evcPtr);
  hlr_free (evc);
  hlr_free (utuPtr);
  hlr_free (utu);
  return rsd;
}
