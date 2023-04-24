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
/** @file hierclus.h
    @brief Hierarchical clustering of a square matrix with elements in the range
    [0.0..DBL_MAX].
    Module prefix hc_
*/
#ifndef HIERCLUS_H
#define HIERCLUS_H

#ifdef __cplusplus
extern "C" {
#endif

/// single linkage clustering
#define HC_SINGLE_LINKAGE    0
/// complete linkage clustering
#define HC_COMPLETE_LINKAGE  1
/// average clustering
#define HC_AVERAGE_LINKAGE   2
/// weighted clustering
#define HC_WEIGHTED_AVERAGE  3
/// mediane clustering
#define HC_MEDIANE           4
/// centroid clustering
#define HC_CENTROID          5
/// Ward clustering
#define HC_WARD              6

extern void hc_register_returnCluster (int (*f)(int numClus,double val,
                                                int left,int right,char *clus));
extern void hc_run (double **mat,int dim,int ctype);

#ifdef __cplusplus
}
#endif

#endif
