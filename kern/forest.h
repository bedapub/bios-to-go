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
/** @file forest.h
    @brief Module for representing hierarchical trees in main memory and
    printing them as an HTML selector.
    Module prefix forest_
*/
#ifndef FOREST_H
#define FOREST_H

#ifdef __cplusplus
extern "C" {
#endif

/// structure of a forest node
typedef struct nodeStruct {
  char *name; //!< name of node
  char *supernode; //!< "" denotes a root node
  void *extension; //!< ???
}*ForestNode;

extern void forest_start (void);
extern void forest_add (char *nodeValue,char *supernodeValue,void *extension);
extern Array forest_get (void); // Array of ForestNode in notifier.c:nodeOrder()
extern int forest_selectorPrint (Array forest /*of ForestNode*/,
                                 Texta matchNodes);
extern int forest_selector (Array forest /*of ForestNode*/, Texta matchNodes,
                            Texta nodesOut,Array matchesOut /*of int*/);
extern void forest_destroy (Array forestNodes);

#ifdef __cplusplus
}
#endif

#endif
