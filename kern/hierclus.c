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
/** @file hierclus.c
    @brief Hierarchical clustering of a square matrix with elements in the range
    [0.0..DBL_MAX].
    Module prefix hc_
*/
#include <math.h>
#include <float.h>
#include "log.h"
#include "hlrmisc.h"
#include "matvec.h"
#include "hierclus.h"

static int *rowsColsDone;

static int (*returnCluster_hook) (int numClus,double val,int left,int right,
                                  char *clus) = NULL;
/* a function which is called at each level of the tree. It gets the distance,
   the cluster number on the left and on the right and a string containing
   '0's, '1's and '2's, indicating
   which items to be clustered are in the left and right subtree.
*/

void hc_register_returnCluster (int (*f)(int numClus,double val,
                                         int left,int right,char *clus)) {
  /**
     Register a function to be called when a cluster is available
     @param[in] f - the function
  */
  returnCluster_hook = f;
}

/// structure for a cluster node
typedef struct {
  char *string; //!< indicating which individuals belong to the node
  int left; //!< node to the left of this node
  int right; //!< node to the right of this node
  int numIndi; //!< how many individuals in this below this node
  float val; //!< height of node [0.0..1.0]
}Cluster;

void hc_run (double **mat,int dim,int ctype) {
  /**
     @param[in] mat - a square matrix of distances
     @param[in] dim - the dimension of the matrix
     @param[in] ctype - the clustering method e.g. HC_AVERAGE_LINKAGE
  */
  Cluster *cluster;
  int minR,minC;
  int i;
  int r,c;
  double minVal;
  int clusNum;
  int wi,wk;
  double temp,temp1;
  int s,cls;

  if (returnCluster_hook == NULL)
    die ("hc_run: returnCluster function not registered");

  rowsColsDone = (int *)hlr_calloc (dim,sizeof (int));

  cluster = (Cluster *)hlr_calloc (dim-1,sizeof (Cluster));
  for (i=0;i<dim-1;i++) /* there are dim-1 clusters, '\0' at end of string */
    cluster[i].string = (char *)hlr_calloc (dim+1,sizeof (char));
  clusNum = -1;
  for (;;) {
    minVal = DBL_MAX;
    minR = minC = -1;
    for (r=0;r<dim-1;r++) {
      if (rowsColsDone[r])
        continue;
      for (c=r+1;c<dim;c++) {
        if (rowsColsDone[c])
          continue;
        if (mat[r][c] < minVal) {
          minVal = mat[r][c];
          minR = r;
          minC = c;
        }
      }
    }
    if (minVal == DBL_MAX)
      break;
    // combine row and column where minimum found
    // cl or cr = -1 means single individuum
    clusNum++;
    // initialize string to 0's
    for (i=0;i<dim;i++)
      cluster[clusNum].string[i] = '0';
    // find most recent cluster containing indi minR
    cls = clusNum-1;
    while (cls >= 0 && cluster[cls].string[minR] == '0')
      cls--;
    if (cls == -1) {
      cluster[clusNum].string[minR] = '1';
      cluster[clusNum].left = -1;
    }
    else {
      for (s=0;s<dim;s++) {
        if (cluster[cls].string[s] != '0')
          cluster[clusNum].string[s] = '1';
        cluster[clusNum].left = cls;
      }
    }
    // find most recent cluster containing indi minC
    cls = clusNum-1;
    while (cls >= 0 && cluster[cls].string[minC] == '0')
      cls--;
    if (cls == -1) {
      cluster[clusNum].string[minC] = '2';
      cluster[clusNum].right = -1;
    }
    else {
      for (s=0;s<dim;s++) {
        if (cluster[cls].string[s] != '0')
          cluster[clusNum].string[s] = '2';
        cluster[clusNum].right = cls;
      }
    }
    cluster[clusNum].numIndi = 0;
    for (s=0;s<dim;s++)
      if (cluster[clusNum].string[s] != '0')
        cluster[clusNum].numIndi++;
    cluster[clusNum].val = minVal;
    if (cluster[clusNum].left == -1)
      wi = 1;
    else
      wi = cluster[cluster[clusNum].left].numIndi;
    if (cluster[clusNum].right == -1)
      wk = 1;
    else
      wk = cluster[cluster[clusNum].right].numIndi;

    if (returnCluster_hook) {
      if (!(*returnCluster_hook) (clusNum,minVal,cluster[clusNum].left,
                                  cluster[clusNum].right,cluster[clusNum].string))
        break;
    }

    if (ctype == HC_SINGLE_LINKAGE) {
      double smaller;

      for (i=0;i<minR;i++) {
        if (rowsColsDone[i])
          continue;
        smaller = MIN (mat[i][minR],mat[i][minC]);
        mat[i][minR] = smaller;
      }
      for (i=minC+1;i<dim;i++) {
        if (rowsColsDone[i])
          continue;
        smaller = MIN (mat[minR][i],mat[minC][i]);
        mat[minR][i] = smaller;
      }
      for (i=minR+1;i<minC;i++) {
        if (rowsColsDone[i])
          continue;
        smaller = MIN (mat[minR][i],mat[i][minC]);
        mat[minR][i] = smaller;
      }
    }
    else if (ctype == HC_COMPLETE_LINKAGE) {
      double larger;

      for (i=0;i<minR;i++) {
        if (rowsColsDone[i])
          continue;
        larger = MAX (mat[i][minR],mat[i][minC]);
        mat[i][minR] = larger;
      }
      for (i=minC+1;i<dim;i++) {
        if (rowsColsDone[i])
          continue;
        larger = MAX (mat[minR][i],mat[minC][i]);
        mat[minR][i] = larger;
      }
      for (i=minR+1;i<minC;i++) {
        if (rowsColsDone[i])
          continue;
        larger = MAX (mat[minR][i],mat[i][minC]);
        mat[minR][i] = larger;
      }
    }
    else if (ctype == HC_AVERAGE_LINKAGE) {
      for (i=0;i<minR;i++) {
        if (rowsColsDone[i])
          continue;
        mat[i][minR] = (mat[i][minR] + mat[i][minC]) / 2;
      }
      for (i=minC+1;i<dim;i++) {
        if (rowsColsDone[i])
          continue;
        mat[minR][i] = (mat[minR][i] + mat[minC][i]) / 2;
      }
      for (i=minR+1;i<minC;i++) {
        if (rowsColsDone[i])
          continue;
        mat[minR][i] = (mat[minR][i] + mat[i][minC]) / 2;
      }
    }
    else if (ctype == HC_WEIGHTED_AVERAGE) {
      for (i=0;i<minR;i++) {
        if (rowsColsDone[i])
          continue;
        mat[i][minR] = (mat[i][minR] * wi + mat[i][minC] * wk) / (wi + wk);
      }
      for (i=minC+1;i<dim;i++) {
        if (rowsColsDone[i])
          continue;
        mat[minR][i] = (mat[minR][i] * wi + mat[minC][i] * wk) / (wi + wk);
      }
      for (i=minR+1;i<minC;i++) {
        if (rowsColsDone[i])
          continue;
        mat[minR][i] = (mat[minR][i] * wi + mat[i][minC] * wk) / (wi + wk);
      }
    }
    else if (ctype == HC_MEDIANE) {
      temp = mat[minR][minC] / 4;
      for (i=0;i<minR;i++) {
        if (rowsColsDone[i])
          continue;
        mat[i][minR] = (mat[i][minR] + mat[i][minC]) / 2 - temp;
      }
      for (i=minC+1;i<dim;i++) {
        if (rowsColsDone[i])
          continue;
        mat[minR][i] = (mat[minR][i] + mat[minC][i]) / 2 - temp;
      }
      for (i=minR+1;i<minC;i++) {
        if (rowsColsDone[i])
          continue;
        mat[minR][i] = (mat[minR][i] + mat[i][minC]) / 2 + temp;
      }
    }
    else if (ctype == HC_CENTROID) {
      temp = wi + wk;
      temp1 = mat[minR][minC];
      for (i=0;i<minR;i++) {
        if (rowsColsDone[i])
          continue;
        mat[i][minR] = (mat[i][minR] * wi + mat[i][minC] * wk) / temp - wi * wk * temp1 / (temp * temp);
      }
      for (i=minC+1;i<dim;i++) {
        if (rowsColsDone[i])
          continue;
        mat[minR][i] = (mat[minR][i] * wi + mat[minC][i] * wk) / temp - wi * wk * temp1 / (temp * temp);
      }
      for (i=minR+1;i<minC;i++) {
        if (rowsColsDone[i])
          continue;
        mat[minR][i] = (mat[minR][i] * wi + mat[i][minC] * wk) / temp - wi * wk * temp1 / (temp * temp);
      }
    }
    else if (ctype == HC_WARD) {
      int w,nn;

      temp = mat[minR][minC];
      for (i=0;i<minR;i++) {
        if (rowsColsDone[i])
          continue;
        nn = clusNum;
        while (nn >= 0 && cluster[nn].string[i] == '0')
          nn--;
        w = (nn < 0) ? 1 : cluster[nn].numIndi;
        mat[i][minR] = (mat[i][minR] * (w + wi) + mat[i][minC] * (w + wk) - temp * w) / (w + wi + wk);
      }
      for (i=minC+1;i<dim;i++) {
        if (rowsColsDone[i])
          continue;
        nn = clusNum;
        while (nn >= 0 && cluster[nn].string[i] == '0')
          nn--;
        w = (nn < 0) ? 1 : cluster[nn].numIndi;
        mat[minR][i] = (mat[minR][i] * (w + wi) + mat[minC][i] * (w + wk) - temp * w) / (w + wi + wk);
      }
      for (i=minR+1;i<minC;i++) {
        if (rowsColsDone[i])
          continue;
        nn = clusNum;
        while (nn >= 0 && cluster[nn].string[i] == '0')
          nn--;
        w = (nn < 0) ? 1 : cluster[nn].numIndi;
        mat[minR][i] = (mat[minR][i] * (w + wi) + mat[i][minC] * (w + wk) - temp * w) / (w + wi + wk);
      }
    }
    rowsColsDone[minC] = 1;
  }
  hlr_free (rowsColsDone);
  for (i=0;i<dim-1;i++) // there are dim-1 clusters, '\0' at end of string
    hlr_free (cluster[i].string);
  hlr_free (cluster);
}
