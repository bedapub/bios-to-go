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
/** @file avlTree.c
    @brief Module dealing with AVL balanced binary tree functions.
    Module prefix avl_
*/
#include "log.h"
#include "hlrmisc.h"
#include "avlTree.h"

static void avl_nodeCreate (AvlNode node) {
  node->leftNode = NULL;
  node->rightNode = NULL;
  node->parentNode = NULL;
  node->height = 1;
}

AvlTree avl_treeCreate (int (*cmpFunction)(void *,void *),
                        void (*cleanFunction)(void *)) {
  /**
     Creates an AvlTree, registers pointers to cleanFunction() and
     cmpFunction() and returns a pointer to the AvlTree structure.<br>
     Postcondition: User is responsible to free the memory allocated
                    by calling the function avl_treeDestroy().
     @param[in] cmpFunction - compare function to order two entries in the tree
     @param[in] cleanFunction - clean function to describe how memory allocated
                                to each entry should be freed
     @return new AvlTree
  */
  AvlTree this1 = (AvlTree)hlr_malloc (sizeof (struct _AvlTreeStruct_));
  this1->rootTree = (AvlNode)hlr_malloc (sizeof (struct _AvlNodeStruct_));
  avl_nodeCreate (this1->rootTree);
  this1->compareFunction = cmpFunction;
  this1->cleanFunction = cleanFunction;
  this1->balFlag = 0;
  return this1;
}

static void avl_nodeDestroy (AvlNode node,void (*cleanFunc)(void *)) {
  if (node == NULL)
    return;
  avl_nodeDestroy (node->leftNode,cleanFunc);
  avl_nodeDestroy (node->rightNode,cleanFunc);
  if (node->parentNode != NULL) {
    if (node->parentNode->leftNode == node)
      node->parentNode->leftNode = NULL;
    if (node->parentNode->rightNode == node)
      node->parentNode->rightNode = NULL;
  }
  cleanFunc (node->value);
  hlr_free (node);
}

static void avl_defaultCleanFunc (void *value) {
  return;
}
void avl_treeDestroyFunc (AvlTree this1) {
  /**
     Destroys the AvlTree and memory allocated to all the nodes as
     defined by the cleanFunc() registered through avl_treeCreate()<br>
     Precondition: avl_treeCreate() should be called before
                   calling this function<br>
     Postcondition: Pointer this one can not be used once this function
                    has been called.<br>
     Note: This function is only for internal use. Function avl_treeDestroy()
           should be used instead from outside the package.
     @param[in] this1 - AvlTree that should be destroyed
  */
  avl_nodeDestroy (this1->rootTree->leftNode,this1->cleanFunction);
  avl_nodeDestroy (this1->rootTree,avl_defaultCleanFunc);
  hlr_free (this1);
}

int avl_treeIsEmpty (AvlTree this1) {
  /**
     Checks if the input AvlTree has no nodes.<br>
     Precondition: avl_treeCreate() should be called before
                   calling this function
     @param[in] this1 - an AvlTree
  */
  if (this1->rootTree->leftNode == NULL &&
      this1->rootTree->rightNode == NULL)
    return 1;
  return 0;
}

static int avl_nodeChkHeavyDir (AvlNode unbalNode) {
  int heightLeft = 0;
  if (unbalNode->leftNode != NULL)
    heightLeft = unbalNode->leftNode->height;
  int heightRight = 0;
  if (unbalNode->rightNode != NULL)
    heightRight = unbalNode->rightNode->height;
  int diff = heightRight - heightLeft;
  if (abs (diff) > 2)
    die ("Height difference is greater than 3");
  if (diff < 0) {
    if (diff < -1)
      return -2;
    else
      return -1;
  }
  else if (diff > 0) {
    if (diff > 1)
      return 2;
    else
      return 1;
  }
  return 0;
}

static int avl_nodeCheckConsistency (AvlNode rootNode) {
  if (rootNode == NULL)
    return 1;
  int flagHeavy = avl_nodeChkHeavyDir (rootNode);
  if (abs (flagHeavy) > 1)
    return 0;
  int flagA = avl_nodeCheckConsistency (rootNode->leftNode);
  int flagB = avl_nodeCheckConsistency (rootNode->rightNode);
  return flagA*flagB;
}

int avl_treeIsConsistent (AvlTree this1) {
  /**
     Checks if the AvlTree is consistent in height difference.
     Precondition: avl_treeCreate() should be called before
                   calling this function
     @param[in] this1 - an AvlTree
     @return 1 if consistent, else 0
  */
  if (this1->rootTree->leftNode == NULL &&
      this1->rootTree->rightNode == NULL)
    return 1;
  return avl_nodeCheckConsistency (this1->rootTree->leftNode);
}

static int avl_nodeHeight (AvlNode parent) {
  int heightLeft = 0;
  if (parent->leftNode != NULL)
    heightLeft = parent->leftNode->height;
  int heightRight = 0;
  if (parent->rightNode != NULL)
    heightRight = parent->rightNode->height;
  return MAX (heightLeft,heightRight) + 1;
}

static void avl_nodeAssignChildren (AvlNode parent,
                                    AvlNode leftChild,
                                    AvlNode rightChild) {
  parent->leftNode = leftChild;
  if (leftChild != NULL)
    leftChild->parentNode = parent;
  parent->rightNode = rightChild;
  if (rightChild != NULL)
    rightChild->parentNode = parent;
  parent->height = avl_nodeHeight (parent);
}

static AvlNode avl_nodeGetInOrderMin (AvlNode rootNode) {
  if (rootNode->leftNode == NULL)
    return rootNode;
  AvlNode minNode = avl_nodeGetInOrderMin (rootNode->leftNode);
  return minNode;
}

static AvlNode avl_nodeGetInOrderSuccessorSubCalls (AvlNode rootNode,
                                                    int *flagRevTracking) {
  if (*flagRevTracking == 1) {
    if (rootNode->parentNode->value != NULL) {
      if (rootNode->parentNode->leftNode == rootNode) {
        *flagRevTracking = 0;
        return rootNode->parentNode;
      }
      else
        return avl_nodeGetInOrderSuccessorSubCalls (rootNode->parentNode,
                                                    flagRevTracking);
    }
    else
      return NULL;
  }
  if (rootNode->rightNode == NULL) {
    *flagRevTracking = 1;
    return avl_nodeGetInOrderSuccessorSubCalls (rootNode,flagRevTracking);
  }
  else
    return avl_nodeGetInOrderMin (rootNode->rightNode);
}

static AvlNode avl_nodeGetInOrderSuccessor (AvlNode rootNode) {
  int flagRevTracking = 0;
  AvlNode succNode = avl_nodeGetInOrderSuccessorSubCalls (rootNode,
                                                          &flagRevTracking);
  return succNode;
}

static void avl_nodeReAssignParentsChild (AvlNode node,AvlNode newNode) {
  int flagError = 1;
  if (node->parentNode != NULL){
    if (node->parentNode->leftNode == node) {
      node->parentNode->leftNode = newNode;
      flagError = 0;
    }
    if (node->parentNode->rightNode == node) {
      node->parentNode->rightNode = newNode;
      flagError = 0;
    }
    if (flagError)
      die ("Error in parents");
    node->parentNode->height = avl_nodeHeight (node->parentNode);
  }
}

static void avl_nodeReplace (AvlNode nodeOld,AvlNode nodeNew) {
  nodeNew->parentNode = nodeOld->parentNode;
  avl_nodeAssignChildren (nodeNew,nodeOld->leftNode,nodeOld->rightNode);
  avl_nodeReAssignParentsChild (nodeOld,nodeNew);
}

static AvlNode avl_nodeRestructure (AvlNode unBalNode,
                                    int *balFlag) {
  AvlNode arrNodes[7];
  int flagHeavy = avl_nodeChkHeavyDir (unBalNode);
  if (abs (flagHeavy) < 2)
    return unBalNode;
  if (flagHeavy < 0) {
    if (flagHeavy == -2) { // left heavy
      flagHeavy = avl_nodeChkHeavyDir (unBalNode->leftNode);
      if (flagHeavy == 1) { // left-right heavy
        arrNodes[0] = unBalNode->leftNode->leftNode;
        arrNodes[1] = unBalNode->leftNode;
        arrNodes[2] = unBalNode->leftNode->rightNode->leftNode;
        arrNodes[3] = unBalNode->leftNode->rightNode;
        arrNodes[4] = unBalNode->leftNode->rightNode->rightNode;
        arrNodes[5] = unBalNode;
        arrNodes[6] = unBalNode->rightNode;
      }
      else { // left-left heavy
        arrNodes[0] = unBalNode->leftNode->leftNode->leftNode;
        arrNodes[1] = unBalNode->leftNode->leftNode;
        arrNodes[2] = unBalNode->leftNode->leftNode->rightNode;
        arrNodes[3] = unBalNode->leftNode;
        arrNodes[4] = unBalNode->leftNode->rightNode;
        arrNodes[5] = unBalNode;
        arrNodes[6] = unBalNode->rightNode;
      }
    }
    else {
      die ("Missed restructuring B");
    }
  }
  else if (flagHeavy > 0) {
    if (flagHeavy == 2) { // right heavy
      flagHeavy = avl_nodeChkHeavyDir (unBalNode->rightNode);
      if (flagHeavy == -1) { // right-left heavy
        arrNodes[0] = unBalNode->leftNode;
        arrNodes[1] = unBalNode;
        arrNodes[2] = unBalNode->rightNode->leftNode->leftNode;
        arrNodes[3] = unBalNode->rightNode->leftNode;
        arrNodes[4] = unBalNode->rightNode->leftNode->rightNode;
        arrNodes[5] = unBalNode->rightNode;
        arrNodes[6] = unBalNode->rightNode->rightNode;
      }
      else { // right-right heavy
        arrNodes[0] = unBalNode->leftNode;
        arrNodes[1] = unBalNode;
        arrNodes[2] = unBalNode->rightNode->leftNode;
        arrNodes[3] = unBalNode->rightNode;
        arrNodes[4] = unBalNode->rightNode->rightNode->leftNode;
        arrNodes[5] = unBalNode->rightNode->rightNode;
        arrNodes[6] = unBalNode->rightNode->rightNode->rightNode;
      }
    }
    else {
      die ("Missed restructuring D");
    }
  }
  avl_nodeReAssignParentsChild (unBalNode,arrNodes[3]);
  arrNodes[3]->parentNode = unBalNode->parentNode;
  avl_nodeAssignChildren (arrNodes[1],arrNodes[0],arrNodes[2]);
  avl_nodeAssignChildren (arrNodes[5],arrNodes[4],arrNodes[6]);
  avl_nodeAssignChildren (arrNodes[3],arrNodes[1],arrNodes[5]);
  *balFlag = 0;
  return arrNodes[3];
}

static AvlNode avl_nodeDelete (AvlTree tree,AvlNode rootNode,
                               AvlNode nodeToDelete) {
  // Return 0 if not found
  // Return 1 if deleted
  AvlNode nodeDeleted = NULL;
  int cmpVal;
  if (rootNode == NULL)
    return nodeDeleted;
  cmpVal = tree->compareFunction (nodeToDelete->value,rootNode->value);
  if (cmpVal < 0)
    nodeDeleted = avl_nodeDelete (tree,rootNode->leftNode,nodeToDelete);
  else if (cmpVal > 0)
    nodeDeleted = avl_nodeDelete (tree,rootNode->rightNode,nodeToDelete);
  else {
    nodeDeleted = rootNode;
    if (rootNode->leftNode != NULL) {
      if (rootNode->rightNode != NULL) {
        AvlNode succNode = avl_nodeGetInOrderSuccessor (rootNode);
        AvlNode nodeDeleted2 = NULL;
        // nodeDeleted2 = avl_nodeDelete (tree,rootNode->rightNode,succNode);
        nodeDeleted2 = avl_nodeDelete (tree,tree->rootTree->leftNode,succNode);
        if (nodeDeleted2 != succNode) {
          // NodeDeleted2 should be same as succNode
          die ("Error in logic");
        }
        avl_nodeReplace (rootNode,nodeDeleted2);
        nodeDeleted2->height = avl_nodeHeight (nodeDeleted2);
        rootNode = nodeDeleted2;
      }
      else {
        avl_nodeReAssignParentsChild (rootNode,rootNode->leftNode);
        rootNode->leftNode->parentNode = rootNode->parentNode;
        rootNode = rootNode->parentNode;
      }
    }
    else {
      if (rootNode->rightNode != NULL) {
        avl_nodeReAssignParentsChild (rootNode,rootNode->rightNode);
        rootNode->rightNode->parentNode = rootNode->parentNode;
        rootNode = rootNode->parentNode;
      }
      else {
        // To be deleted node has no children
        // Simply remove the node from the tree
        avl_nodeReAssignParentsChild (rootNode,NULL);
        return nodeDeleted;
      }
    }
    rootNode->height = avl_nodeHeight (rootNode);
    return nodeDeleted;
  }
  if (nodeDeleted != NULL) {
    int balFlag = 0;
    rootNode = avl_nodeRestructure (rootNode,&balFlag);
  }
  rootNode->height = avl_nodeHeight (rootNode);
  return nodeDeleted;
}

int avl_treeDelete (AvlTree this1,void *valueToDelete) {
  /**
     Deletes the node corresponding to input value from the AvlTree.<br>
     Precondition: avl_treeCreate() should be called before
                   calling this function
     @param[in] this1 - an AvlTree
     @param[in] valueToDelete - a pointer to value to delete
     @return 0 if not found, else 1 if sucessfully deleted
  */
  AvlNode newNode = (AvlNode)hlr_malloc (sizeof (struct _AvlNodeStruct_));
  avl_nodeCreate (newNode);
  newNode->value = valueToDelete;
  AvlNode nodeDeleted = avl_nodeDelete (this1,this1->rootTree->leftNode,newNode);
  hlr_free (newNode);
  if (nodeDeleted != NULL) {
    avl_nodeDestroy (nodeDeleted,this1->cleanFunction);
    this1->numNodes--;
    return 1;
  }
  return 0;
}

static int avl_nodeInsert (AvlNode rootNode,AvlNode nodeToInsert,
                           int (*cmpFunction)(void *,void *),
                           void (*modifyFunc)(void *,void *),
                           int *balFlag) {
  // Return 0 if found
  // Return 1 if inserted
  int returnVal = 0;
  int cmpVal = cmpFunction (nodeToInsert->value,rootNode->value);
  if (cmpVal < 0) {
    if (rootNode->leftNode == NULL) {
      avl_nodeAssignChildren (rootNode,nodeToInsert,rootNode->rightNode);
      *balFlag = 1;
      returnVal = 1;
    }
    else
      returnVal = avl_nodeInsert (rootNode->leftNode,nodeToInsert,
                                  cmpFunction,modifyFunc,balFlag);
  }
  else if (cmpVal > 0) {
    if (rootNode->rightNode == NULL) {
      avl_nodeAssignChildren (rootNode,rootNode->leftNode,nodeToInsert);
      *balFlag = 1;
      returnVal = 1;
    }
    else
      returnVal = avl_nodeInsert (rootNode->rightNode,nodeToInsert,
                                  cmpFunction,modifyFunc,balFlag);
  }
  else {
    modifyFunc (rootNode->value,nodeToInsert->value);
    return 0;
  }
  rootNode->height = avl_nodeHeight (rootNode);
  if (*balFlag == 1)
    rootNode = avl_nodeRestructure (rootNode,balFlag);
  return returnVal;
}

int avl_treeInsert (AvlTree this1,void *valueToInsert,
                    void (*modifyFunc)(void *,void *)) {
  /**
     Inserts a new node in the AvlTree.<br>
     Precondition: avl_treeCreate() should be called before
                   calling this function<br>
     Postcondition: User is responsible for freeing the memory allocated to
                    new node by calling the avl_treeDestroy() function.<br>
     @param[in] this1 - an AvlTree
     @param[in] valueToInsert - a pointer to new value to insert
     @param[in] modifyFunc - defines what should be done when the
                             valueToInsert is already found on the tree.
     @return 0 if found, else 1 if sucessfully inserted
  */
  AvlNode newNode = (AvlNode)hlr_malloc (sizeof (struct _AvlNodeStruct_));
  avl_nodeCreate (newNode);
  newNode->value = valueToInsert;
  if (avl_treeIsEmpty (this1)) {
    avl_nodeAssignChildren (this1->rootTree,newNode,newNode);
    this1->numNodes++;
    return 1;
  }
  int flagInserted = avl_nodeInsert (this1->rootTree->leftNode,newNode,
                                     this1->compareFunction,
                                     modifyFunc,&this1->balFlag);
  if (!flagInserted)
    hlr_free (newNode);
  this1->numNodes += flagInserted;
  return flagInserted;
}

static AvlNode avl_nodeFind (AvlNode rootNode,AvlNode nodeToFind,
                             int (*cmpFunction)(void *,void *)) {
  // returns NULL if not found
  AvlNode foundNode = NULL;
  if (rootNode == NULL)
    return foundNode;
  int cmpVal = cmpFunction (nodeToFind->value,rootNode->value);
  if (cmpVal < 0)
    foundNode = avl_nodeFind (rootNode->leftNode,nodeToFind,cmpFunction);
  else if (cmpVal > 0)
    foundNode = avl_nodeFind (rootNode->rightNode,nodeToFind,cmpFunction);
  else
    foundNode = rootNode;
  return foundNode;
}

int avl_treeFind (AvlTree this1,void *valueToFind,void **valueFound) {
  /**
     Finds the value correponding to unique id of valueToFind in the AvlTree.<br>
     Precondition: avl_treeCreate() should be called before
                   calling this function
     @param[in] this1 - an AvlTree
     @param[in] valueToFind - a pointer to value to search for; valueToFind
                              should at least have the unique ID that are
                              used to distinguish two different values stored
                              on the tree
     @param[out] valueFound - a double pointer to which the value found on the
                              avlTree is copied.
     @return 0 if not found, else 1 if sucessfully found
  */
  if (avl_treeIsEmpty (this1))
    return 0;
  AvlNode tmpNode = (AvlNode)hlr_malloc (sizeof (struct _AvlNodeStruct_));
  avl_nodeCreate (tmpNode);
  tmpNode->value = valueToFind;
  AvlNode foundNode = NULL;
  foundNode = avl_nodeFind (this1->rootTree->leftNode,tmpNode,
                            this1->compareFunction);
  hlr_free (tmpNode);
  if (foundNode == NULL)
    return 0;
  *valueFound = foundNode->value;
  return 1;
}

int avl_treeFindMin (AvlTree this1,void **valueFound) {
  /**
     Finds the value with minimum unique id on the AvlTree.<br>
     Precondition: avl_treeCreate() should be called before
                   calling this function
     @param[in] this1 - an AvlTree
     @param[in] valueFound - a double pointer on which the minimum value
                             found on the avlTree is copied.
     @return 0 if no node exists on the tree, else 1
  */
  if (avl_treeIsEmpty (this1))
    return 0;
  AvlNode minNode = avl_nodeGetInOrderMin (this1->rootTree->leftNode);
  *valueFound = minNode->value;
  return 1;
}

static AvlNode avl_nodeGetInOrderMax (AvlNode rootNode) {
  if (rootNode->rightNode == NULL)
    return rootNode;
  AvlNode maxNode = avl_nodeGetInOrderMax (rootNode->rightNode);
  return maxNode;
}

int avl_treeFindMax (AvlTree this1,void **valueFound) {
  /**
     Finds the value with maximum unique id on the AvlTree.<br>
     Precondition: avl_treeCreate() should be called before
                   calling this function
     @param[in] this1 - an AvlTree
     @param[out] valueFound - a double pointer on which the  maximum value
                              found on the avlTree is copied.
     @return 0 if no node exists on the tree, else returns 1
  */
  if (avl_treeIsEmpty (this1))
    return 0;
  AvlNode maxNode = avl_nodeGetInOrderMax (this1->rootTree->leftNode);
  *valueFound = maxNode->value;
  return 1;
}

static AvlNode avl_nodeFindElseNeighbor (AvlNode rootNode,AvlNode nodeToFind,
                                         int (*cmpFunction)(void *,void *),
                                         int *flagPrevNext) {
  /*
    Returns NULL if not found
    Else Returns the node with value that is inOrder next (or previous)
    to the node that is being searched.
    Sets the flagPrevNext to 0 if nodeToFind exists on the tree
    Else sets the flagPrevNext to -1 if inorder previous value is returned
    Else Sets the flagPrevNext to 1 if inorder next value is returned
  */
  AvlNode foundNode = NULL;
  if (rootNode == NULL)
    return foundNode;
  *flagPrevNext = 0;
  int cmpVal = cmpFunction (nodeToFind->value,rootNode->value);
  if (cmpVal < 0)
    foundNode = avl_nodeFindElseNeighbor (rootNode->leftNode,nodeToFind,
                                          cmpFunction,flagPrevNext);
  else if (cmpVal > 0)
    foundNode = avl_nodeFindElseNeighbor (rootNode->rightNode,nodeToFind,
                                          cmpFunction,flagPrevNext);
  else
    foundNode = rootNode;
  if (foundNode == NULL) {
    if (cmpVal > 0)
      *flagPrevNext = -1;
    else if (cmpVal < 0)
      *flagPrevNext = 1;
    return rootNode;
  }
  return foundNode;
}

void *avl_treeNextValue (AvlTree this1,void *currVal,int *foundFlag) {
  /**
     Finds the successor node to the currVal
     If the currVal given as input is not found on the AvlTree then
     the closest largest to the currVal in the sorted order is returned
     and the foundFlag is set to 0.<br>
     Precondition: avl_treeCreate() should be called before
                   calling this function
     @param[in] this1 - an AvlTree
     @param[in] currVal - current value for which a successor value is desired
     @param[out] foundFlag - a pointer to the integer variable which is set
                             to 1 if the currVal was found on the tree
                             else it is set to 0.
     @return the value that comes next to currval in increasing order
  */
  *foundFlag = 0;
  if (avl_treeIsEmpty (this1))
    return NULL;
  *foundFlag = 1;
  AvlNode tmpNode = (AvlNode)hlr_malloc (sizeof (struct _AvlNodeStruct_));
  avl_nodeCreate (tmpNode);
  tmpNode->value = currVal;
  AvlNode currValNode = NULL;
  int flagPrevNext = 0;
  currValNode = avl_nodeFindElseNeighbor (this1->rootTree->leftNode,
                                          tmpNode,this1->compareFunction,
                                          &flagPrevNext);
  hlr_free (tmpNode);
  if (flagPrevNext != 0)
    *foundFlag = 0;
  if (flagPrevNext == 1)
    return currValNode->value;
  AvlNode succNode = avl_nodeGetInOrderSuccessor (currValNode);
  if (succNode == NULL)
    return NULL;
  return succNode->value;
}

static AvlNode avl_nodeGetInOrderPredecessorSubCalls (AvlNode rootNode,
                                                      int *flagRevTracking) {
  if (*flagRevTracking == 1) {
    if (rootNode->parentNode->value != NULL) {
      if (rootNode->parentNode->rightNode == rootNode) {
        *flagRevTracking = 0;
        return rootNode->parentNode;
      }
      else
        return avl_nodeGetInOrderPredecessorSubCalls (rootNode->parentNode,
                                                      flagRevTracking);
    }
    else
      return NULL;
  }
  if (rootNode->leftNode == NULL) {
    *flagRevTracking = 1;
    return avl_nodeGetInOrderPredecessorSubCalls (rootNode,flagRevTracking);
  }
  else
    return avl_nodeGetInOrderMax (rootNode->leftNode);
}

static AvlNode avl_nodeGetInOrderPredecessor (AvlNode rootNode) {
  int flagRevTracking = 0;
  AvlNode preNode = avl_nodeGetInOrderPredecessorSubCalls (rootNode,
                                                           &flagRevTracking);
  return preNode;
}

void *avl_treePrevValue (AvlTree this1,void *currVal,int *foundFlag) {
  /**
     Finds the predecessor node to the currVal
     If the currVal (given as input) is not found on the AvlTree then
     the closest previous value to the currVal in the sorted order
     is returned and the foundFlag is set to 0.<br>
     Precondition: avl_treeCreate() should be called before
                   calling this function
     @param[in] this1 - an AvlTree
     @param[in] currVal - current value for which a predecessor value is
                          desired
     @param[in] foundFlag - a pointer to the integer variable which is set to
                            1 if the currVal was found on the tree
                            else it is set to 0.
     @return the value that comes just one before the currval in
             increasing order
  */
  *foundFlag = 0;
  if (avl_treeIsEmpty (this1))
    return NULL;
  *foundFlag = 1;
  AvlNode tmpNode = (AvlNode)hlr_malloc (sizeof (struct _AvlNodeStruct_));
  avl_nodeCreate (tmpNode);
  tmpNode->value = currVal;
  AvlNode currValNode = NULL;
  int flagPrevNext = 0;
  currValNode = avl_nodeFindElseNeighbor (this1->rootTree->leftNode,
                                          tmpNode,this1->compareFunction,
                                          &flagPrevNext);
  hlr_free (tmpNode);
  if (flagPrevNext != 0)
    *foundFlag = 0;
  if (flagPrevNext == -1)
    return currValNode->value;
  AvlNode preNode = avl_nodeGetInOrderPredecessor (currValNode);
  if (preNode == NULL)
    return NULL;
  return preNode->value;
}

long avl_treeNumNodes (AvlTree this1) {
  /**
     Returns the number of nodes on the AvlTree.<br>
     Precondition: avl_treeCreate() should be called before
                   calling this function
     @param[in] this1 - an AvlTree
     @return the number of nodes
  */
  return this1->numNodes;
}

int avl_treeHeight (AvlTree this1) {
  /**
     Returns the height of AvlTree.<br>
     Precondition: avl_treeCreate() should be called before
                   calling this function
     @param[in] this1 - an AvlTree
     @return the height
  */
  if (avl_treeIsEmpty (this1))
    return 0;
  return avl_nodeHeight (this1->rootTree);
}

static void avl_nodeCallFunction (AvlNode node,void *applyFunc(),
                                  int nargs,va_list args){
  /*
    Only for internal use. avl_nodeApplyFunc() should be called to
    access this function.
    If number of arguments are greater than 6, then the applyFunc()
    should be able to parse  variable argument list in the same way
    as shown below.
  */
  switch (nargs) {
  case 0:
    applyFunc (node->value);
    break;
  case 1:
    {
      void *arg1 = va_arg (args,void *);
      applyFunc (node->value,arg1);
      break;
    }
  case 2:
    {
      void *arg1 = va_arg (args,void *);
      void *arg2 = va_arg (args,void *);
      applyFunc (node->value,arg1,arg2);
      break;
    }
  case 3:
    {
      void *arg1 = va_arg (args,void *);
      void *arg2 = va_arg (args,void *);
      void *arg3 = va_arg (args,void *);
      applyFunc (node->value,arg1,arg2,arg3);
      break;
    }
  case 4:
    {
      void *arg1 = va_arg (args,void *);
      void *arg2 = va_arg (args,void *);
      void *arg3 = va_arg (args,void *);
      void *arg4 = va_arg (args,void *);
      applyFunc (node->value,arg1,arg2,arg3,arg4);
      break;
    }
  case 5:
    {
      void *arg1 = va_arg (args,void *);
      void *arg2 = va_arg (args,void *);
      void *arg3 = va_arg (args,void *);
      void *arg4 = va_arg (args,void *);
      void *arg5 = va_arg (args,void *);
      applyFunc (node->value,arg1,arg2,arg3,arg4,arg5);
      break;
    }
  case 6:
    {
      void *arg1 = va_arg (args,void *);
      void *arg2 = va_arg (args,void *);
      void *arg3 = va_arg (args,void *);
      void *arg4 = va_arg (args,void *);
      void *arg5 = va_arg (args,void *);
      void *arg6 = va_arg (args,void *);
      applyFunc (node->value,arg1,arg2,arg3,arg4,arg5,arg6);
      break;
    }
  default:
    applyFunc (node->value,nargs,args);
  }
}

static void avl_nodeApplyFunc (AvlNode rootNode,void (*applyFunc)(),
                               int nargs,va_list args) {
  if (rootNode == NULL)
    return;
  avl_nodeApplyFunc (rootNode->leftNode,applyFunc,nargs,args);
  va_list tmpArgs;
  *tmpArgs = *args;
  avl_nodeCallFunction (rootNode,(void*(*)())applyFunc,nargs,args);
  *args = *tmpArgs;
  avl_nodeApplyFunc (rootNode->rightNode,applyFunc,nargs,args);
}

void avl_treeApplyFuncFixedArgs (AvlTree this1,void (*applyFunc) (),
                                 int nargs,va_list args) {
  /**
     Iterates through all the nodes and applies the function
     specified by the function pointer applyFunc() to the value stored
     on every node. Arguments are specified as va_list.<br>
     Precondition: avl_treeCreate() should be called before
                   calling this function
     @param[in] this1 - an AvlTree
     @param[in] applyFunc - a pointer to function
     @param[in] nargs - number of arguments as integer
     @param[in] args - variable list of arguments
  */
  avl_nodeApplyFunc (this1->rootTree->leftNode,(void (*)())applyFunc,
                     nargs,args);
}

void avl_treeApplyFunc (AvlTree this1,void (*applyFunc)(),int nargs,...) {
  /**
     Iterates through all the nodes and applies the function
     specified by the function pointer applyFunc() to the value stored
     on every node.<br>
     Number of arguments can be at max 6. For more arguments, define
     applyFunc() as applyFunc(value,int nargs,va_list args) in the main
     program. See avl_nodeCallFunction() for more details.<br>
     Precondition: avl_treeCreate() should be called before
                   calling this function
     @param[in] this1 - an AvlTree
     @param[in] applyFunc - a pointer to function
     @param[in] nargs - number of arguments as integer
  */
  va_list args;

  va_start (args,nargs);
  avl_nodeApplyFunc (this1->rootTree->leftNode,(void (*)())applyFunc,
                     nargs,args);
  va_end (args);
  return;
}

static void avl_nodePrint (AvlNode node,void (*printFunc)(void *,FILE *),
                           FILE * outFile){
  if (node == NULL)
    return;
  fprintf (outFile,"[[");
  printFunc (node,outFile);
  fprintf (outFile,"]]---->[[");
  if (node->leftNode == NULL)
    fprintf (outFile,"NULL]]\t");
  else {
    printFunc (node->leftNode->value,outFile);
    fprintf (outFile,"]]\t");
  }
  fprintf (outFile,"[[");
  printFunc (node,outFile);
  fprintf (outFile,"]]---->[[");
  if (node->rightNode == NULL)
    fprintf (outFile,"NULL]]\n");
  else {
    printFunc (node->rightNode->value,outFile);
    fprintf (outFile,"]]\n");
  }
  fflush (outFile);
  avl_nodePrint (node->leftNode,printFunc,outFile);
  avl_nodePrint (node->rightNode,printFunc,outFile);
}

void avl_treePrint (AvlTree this1,void (*printFunc)(void *,FILE *),
                    FILE * outFile){
  /**
     Prints the tree in the format
     [[root value]]---->[[left value]]tab_space[[root value]]---->[[right value]]new_line<br>
     Note: printFunc() should not print the following "NULL", "[[", "]]",
     "\t" and "\n".<br>
     Future Use: A tree structure can be stored on the file and read in
     directly without having to construct it from sratch (function not
     implemented yet).<br>
     Precondition: avl_treeCreate() should be called before
                   calling this function
     @param[in] this1 - an AvlTree
     @param[in] printFunc - a pointer to print function
     @param[in] outFile - FILE pointer where the tree should be printed out.
                          Use stdout to print on the terminal window
  */
  if (avl_treeIsEmpty (this1))
    return;
  fprintf (outFile,"Root Node\n");
  avl_nodePrint (this1->rootTree->leftNode,printFunc,outFile);
}

AvlIterator avl_itCreate (AvlTree this1) {
  /**
     Creates an iterator to access elements in increasing order on AvlTree.<br>
     Precondition: avl_treeCreate() should be called before
                   calling this function<br>
     Postcondition: User is responsible to free the memory allocated
                    by calling the function avl_itDestroy().
     @param[in] this1 - an AvlTree
     @return pointer to new AvlTree iterator
  */
  if (this1->rootTree == NULL)
    die ("AvlTree has to be initialized before using it.");
  AvlIterator newIterator = (AvlIterator)hlr_malloc (sizeof (AvlIterator));
  newIterator->currNode = avl_nodeGetInOrderMin (this1->rootTree->leftNode);
  return newIterator;
}

AvlIterator avl_itCreateRev (AvlTree this1) {
  /**
     Creates an iterator to access elements in decreasing order for AvlTree.<br>
     Precondition: avl_treeCreate() should be called before
                   calling this function<br>
     Postcondition: User is responsible to free the memory allocated
                    by calling the function avl_itDestroy().
     @param[in] this1 - an AvlTree
     @return pointer to new AvlTree iterator
  */
  if (this1->rootTree == NULL)
    die ("AvlTree has to be initialized before using it.");
  AvlIterator newIterator = (AvlIterator)hlr_malloc (sizeof (AvlIterator));
  newIterator->currNode = avl_nodeGetInOrderMax (this1->rootTree->leftNode);
  return newIterator;
}

void avl_itDestroyFunc (AvlIterator this1) {
  /**
     Destroys the AvlTree iterator
     Precondition: avl_itCreate() should be called before
                   calling this function
     Postcondition: iterator  can not be used once this function
                    has been called.
     Note: This function is only for internal use. Function avl_itDestroy()
     should be used instead from outside the package.
     @param[in] this1 - an AvlTree iterator
  */
  hlr_free (this1);
}

void *avl_itNextValue (AvlIterator this1) {
  /**
     Returns the next value in sorted order and increments the iterator.<br>
     Precondition: avl_itCreate() should be called before
                   calling this function
     @param[in] this1 - an AvlTree iterator
     @return pointer to next value in sorted order;
             NULL if no next element is found
  */
  if (this1 == NULL)
    die ("AvlIterator should be initialized before using it.");
  if (this1->currNode == NULL)
    return NULL;
  void *returnVal = this1->currNode->value;
  this1->currNode = avl_nodeGetInOrderSuccessor (this1->currNode);
  return returnVal;
}

void *avl_itPrevValue (AvlIterator this1) {
  /**
     Returns the previous value in sorted order and decrements the iterator.<br>
     Precondition: avl_itCreate() should be called before
                   calling this function
     @param[in] this1 - an AvlTree iterator
     @return pointer to next value in sorted order;
             NULL if no next element is found
  */
  if (this1 == NULL)
    die ("AvlIterator should be initialized before using it.");
  if (this1->currNode == NULL)
    return NULL;
  void *returnVal = this1->currNode->value;
  this1->currNode = avl_nodeGetInOrderPredecessor (this1->currNode);
  return returnVal;
}

AvlIterator avl_itCopy (AvlIterator this1) {
  /**
     Makes a copy of input iterator.<br>
     Precondition: avl_itCreate() should have been called on iterator to be
                   copied
     Postcondition: User is responsible to free the memory allocated
                    by calling the function avl_itDestroy().
     @param[in] this1 - an AvlTree iterator
     @return a new AvlTree iterator
  */
  if (this1 == NULL)
    die ("Avl iterator has to be initialized before using it.");
  AvlIterator newIterator = (AvlIterator)hlr_malloc (sizeof (AvlIterator));
  newIterator->currNode = this1->currNode;
  return newIterator;
}

/*
  Commented out because of no immediate use of this function
  and avoid generating warning at compile time

static void avl_nodeCopy (AvlNode nodeOld,AvlNode nodeNew) {
  nodeNew->parentNode = nodeOld->parentNode;
  nodeNew->value = nodeOld->value;
  nodeNew->leftNode = nodeOld->leftNode;
  nodeNew->rightNode = nodeOld->rightNode;
  nodeNew->height = nodeOld->height;
}
*/

/*
  Commented out because of no immediate use of this function
  and avoid generating warning at compile time

static int avl_nodeCheckIfGrandParentUnbalanced (AvlNode node) {
  // returns 1 if unbalanced
  // returns 0 otherwise
  if (node->parentNode == NULL)
    return 0;
  if (node->parentNode->parentNode == NULL)
    return 0;
  node->parentNode->height = avl_nodeHeight (node->parentNode);
  node->parentNode->parentNode->height =
    avl_nodeHeight (node->parentNode->parentNode);
  int flagHeavy = avl_nodeChkHeavyDir (node->parentNode->parentNode);
  if (abs (flagHeavy) < 2)
    return 0;
  return 1;
}
*/
