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
/** @file geometry.h
    @brief Various geometrical calculations
    Module prefix geom_
*/
#ifndef GEOMETRY_H
#define GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

/// 1 radian in degrees
#define RADIAN 57.29578

extern void geom_diff (float x[],float y[],float z[]);
extern float geom_dot (float x[],float y[]);
extern void geom_cross (float x[],float y[],float z[]);
extern float geom_norm (float x[]);
extern float geom_dihedralAngle (float v1[],float v2[],float v3[],float v4[]);
extern float geom_cosAngle (float v1[],float v2[],float v3[],float v4[]);
extern float geom_distance (float u1[],float v1[]);
extern float geom_distsq (float u1[],float v1[]);
extern void geom_rotateAbout (float a[],float deg,float u[]);
extern float geom_bondAngle (float u1[],float u2[],float u3[]);
extern void geom_matMult4x4 (float a[4][4],float b[4][4],float c[4][4]);
extern void geom_matId4x4 (float a[4][4]);
extern void geom_matInv4x4 (float inv[4][4],float old[4][4]);
extern float geom_kabschRBM (int nat,float **xm1,float **xm2,float *wgt,
                             float *sh1,float rt[3][3],float *sh2);

#ifdef __cplusplus
}
#endif

#endif
