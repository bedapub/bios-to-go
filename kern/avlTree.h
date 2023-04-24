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
/** @file avlTree.h
    @brief Module dealing with AVL balanced binary tree functions.
    Module prefix avl_
*/
#ifndef AVLTREE_H
#define AVLTREE_H

#ifdef __cplusplus
extern "C" {
#endif

/// the AvlNode object
typedef struct _AvlNodeStruct_ {
  void *value; //!< value of the node
  int height; //!< height of the node
  struct _AvlNodeStruct_ *leftNode; //!< left child
  struct _AvlNodeStruct_ *rightNode; //!< right child
  struct _AvlNodeStruct_ *parentNode; //!< parent
}*AvlNode;

/// the AvlIterator object
typedef struct _AvlIteratorStruct_ {
  AvlNode currNode; //!< to remember where we are in the iteration
}*AvlIterator;

/// the AvlTree object
typedef struct _AvlTreeStruct_ {
  AvlNode rootTree; //!< root of the tree
  int (*compareFunction)(void *,void *); //!< function to use to compare nodes
  void (*cleanFunction)(void *); //!< function to use to clean nodes
  int balFlag; //!< indicates whether the tree is balanced
  long numNodes; //!< keeps track of the current number of nodes
}*AvlTree;

extern AvlTree avl_treeCreate (int (*cmpFunction)(void *,void *),
                               void (*cleanFunction)(void *));
extern void avl_treeDestroyFunc (AvlTree this1);
/// call this macro not the function avl_treeDestroyFunc()
#define avl_treeDestroy(x) ((x) ? avl_treeDestroyFunc(x),x=NULL,1:0)
extern int avl_treeIsEmpty (AvlTree this1);
extern int avl_treeIsConsistent (AvlTree this1);
extern int avl_treeDelete (AvlTree this1,void *valueToDelete);
extern int avl_treeInsert (AvlTree this1,void *valueToInsert,
                           void (*modifyFunc)(void *,void *));
extern int avl_treeFind (AvlTree this1,void *valueToFind,void **valueFound);
extern int avl_treeFindMin (AvlTree this1,void **valueFound);
extern int avl_treeFindMax (AvlTree this1,void **valueFound);
extern void *avl_treeNextValue (AvlTree this1,void *currVal,int *foundFlag);
extern void *avl_treePrevValue (AvlTree this1,void *currVal,int *foundFlag);
extern long avl_treeNumNodes (AvlTree this1);
extern int avl_treeHeight (AvlTree this1);
extern void avl_treeApplyFuncFixedArgs (AvlTree this1,void (*applyFunc)(),
                                        int nargs,va_list args);
extern void avl_treeApplyFunc (AvlTree this1,void (*applyFunc)(),
                               int nargs,...);
extern void avl_treePrint (AvlTree this1,void (*printFunc)(void *,FILE *),
                           FILE * outFile);

extern AvlIterator avl_itCreate (AvlTree this1);
extern AvlIterator avl_itCreateRev (AvlTree this1);
extern void avl_itDestroyFunc (AvlIterator this1);
/// call this macro not the function avl_itDestroyFunc()
#define avl_itDestroy(x) ((x) ? avl_itDestroyFunc(x),x=NULL,1 : 0)
extern void avl_itDestroyFunc (AvlIterator this1);
extern void *avl_itNextValue (AvlIterator this1);
extern void *avl_itPrevValue (AvlIterator this1);
extern AvlIterator avl_itCopy (AvlIterator this1);

#ifdef __cplusplus
}
#endif

#endif
