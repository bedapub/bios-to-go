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
/** @file combi.c
    @brief Combinatorics functions.
    Module prefix cmb_
*/
#include <stdlib.h>
#include "combi.h"

/* ***********************************************************************
*/
static int *pos;
static int first;
static int gK;

void cmb_combInit (int n,int k) {
  /**
     Calculate all ways to choose k items from n items
     n! / ((n-k)! * k!)<br>
     Postcondition: cmb_combNext() can been called
     @param[in] n - number of items in set
     @param[in] k - number of items to choose
  */
  int i;

  pos = (int *)malloc ((k+1) * sizeof (int)); // must be k+1
  for (i=0;i<k;i++)
    pos[i] = i;
  pos[k] = n;
  first = 1;
  gK = k;
}

int cmb_combNext (int **p) {
  /**
     Precondition: cmb_combInit() has been called
     @param[out] p - array of chosen items (length: k)
     @return value is 1 if valid combination, 0 otherwise
  */
  int i,k;

  *p = pos;
  if (first) {
    first = 0;
    return 1;
  }
  for (i=gK-1;i>=0;i--) {
    pos[i]++;
    for (k=i+1;k<gK;k++)
      pos[k] = pos[k-1]+1;
    if (pos[k-1] >= pos[k])
      continue;
    if (pos[i] < pos[i+1])
      return 1;
  }
  free (pos);
  return 0;
}

/*************************************************************************/

double cmb_factorial (int f) {
  /**
     Calculates the factorial of a number
     @param[in] f - the number
     @return the factorial
  */
  if (f == 0 || f == 1)
    return 1.0;
  return (f * cmb_factorial (f-1));
}

long cmb_NtiefK (int n,int k) {
  /**
     calculate n! / ((n-k)! * k!)
     @param[in] n,k
     @return n tief k
  */
  return (long)(cmb_factorial (n)/(cmb_factorial (n-k) * cmb_factorial (k)));
}
