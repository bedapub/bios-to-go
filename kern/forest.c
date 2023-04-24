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
/** @file forest.c
    @brief Module for representing hierarchical trees in main memory and
    printing them as an HTML selector.
    Module prefix forest_
*/
/*
Example:

Array forestDomains;
char domain[LEN_domain+1];
char superdomain[LEN_domain+1];
Texta preselectedDomains = textCreate (2);
forest_start ();

superdomain[0]= '\0';
rdb_sql (gRdb,"select domain,superdomain from domainsTable");
rdb_bind_varchar (gRdb,domain,sizeof (domain));
rdb_bind_varchar (gRdb,superdomain,sizeof (superdomain));
while (rdb_rowGet (gRdb) == RDB_SUCCESS) {
  forest_add (domain,superdomain,NULL);
  superdomain[0]= '\0';
}

forestDomains = forest_get ()

textAdd (preselectedDomains,"myPreferredDomain1");
textAdd (preselectedDomains,"myPreferredDomain2");

printf ("<select name=\"domains\" size=%d multiple> \n",
        arrayMax(forestDomains));
forest_selectorPrint (forestDomains,preselectedDomains));
puts ("</select>\n");

forest_destroy (forestDomains);
textDestroy (preselectedDomains);
*/
#include "hlrmisc.h"
#include "log.h"
#include "format.h"

#include "forest.h"

static int gPrintHTML = 1;
static Texta gItems = NULL;
static Array gMatches = NULL; // of int

static ForestNode nodeCreate (char *nodeValue,char *supernodeValue,
                              void *extension) {
  /**
     @param[in] nodeValue -
     @param[in] supernodeValue - not NULL
  */
  ForestNode this1 = (ForestNode)hlr_malloc (sizeof (struct nodeStruct));
  this1->name = hlr_strdup (nodeValue);
  this1->supernode = hlr_strdup (supernodeValue);
  this1->extension = extension;
  return this1;
}

static void nodeDestroy (ForestNode node) {
  hlr_free (node->name);
  hlr_free (node->supernode);
  hlr_free (node->extension);
  hlr_free (node);
}

static int nodeOrder (ForestNode *k1,ForestNode *k2) {
  int i = strcasecmp ((*k1)->supernode,(*k2)->supernode);
  if (i != 0)
    return i;
  return strcasecmp ((*k1)->name,(*k2)->name);
}

/* sorted by supernode, then by node --> for easy browsing
   empty supernodes first
*/
static Array gForestNodes = NULL; // of ForestNode, current version

void forest_start (void) {
  /**
     Is later picked up by forest_get
     precondition: none
     postcondition: forest_add() can be called
  */
  if (gForestNodes)
    die ("forest.c: forest_start was already called!");
  gForestNodes = arrayCreate (20,ForestNode);
}

void forest_add (char *nodeValue,char *supernodeValue,void *extension) {
  /**
     @param[in] nodeValue -
     @param[in] supernodeValue - supernodeValue[0] == '\0' indicates root node
     @param[in] extension - can be NULL
     Precondition: forestStart() or forest_add()
     Postcondition: forest_get() will containt the values added
  */
  array (gForestNodes,arrayMax (gForestNodes),ForestNode) =
    nodeCreate (nodeValue,supernodeValue,extension);
}

Array forest_get (void) {
  /**
     Precondition: forestStart()
     @return Array of ForestNode in nodeOrder(), filled with the nodes added
             using forest_add; user is responsible for freeing the forest
             returned.
  */
  Array t = gForestNodes;
  gForestNodes = NULL;
  arraySort (t,(ARRAYORDERF)nodeOrder);
  return t;
}

// end of construction phase code, from here on reading phase code:

static Texta gMatchNodes = NULL;
static int gFound;
static ForestNode gSearchNode = NULL; /* solely used by forestDescend,
                                         only here for efficiency reasons */

static char *nodeWithoutDots (char *node) {
  char *nodeOnly = strrchr (node,' ');
  return nodeOnly ? nodeOnly+1 : node;
}

static void forest_descend (ForestNode k,int level) {
  static char dots[] =
    "...................................................................................................."; // 100 dots
  static Stringa item = NULL;
  int i;
  int index = 0;
  ForestNode c;

  stringCreateOnce (item,50);
  if (level > (sizeof (dots)-1))
    die ("forest_descend: nested more than 100 deep");

  // print this node
  if (gPrintHTML)
    fputs ("<option",stdout);
  else
    array (gMatches,arrayMax (gMatches),int) = 0;
  while (gMatchNodes && index < arrayMax (gMatchNodes)) {
   if (strEqual (nodeWithoutDots (textItem (gMatchNodes,index)),k->name)) {
      gFound = 1;
      if (gPrintHTML)
        fputs (" selected",stdout);
      else
        arru (gMatches,(arrayMax (gMatches)-1),int) = 1;
      break;
    }
    index++;
  }
  dots[level] = '\0';
  if (gPrintHTML) {
    putchar ('>');
    fputs (dots,stdout);
    if (level > 0)
      putchar (' ');
    fputs (k->name,stdout);
    putchar ('\n');
  }
  else {
    stringCpy (item,dots);
    if (level > 0)
      stringCatChar (item,' ');
    stringCat (item,k->name);
    textAdd (gItems,string (item));
  }
  dots[level] = '.';
  // find and add children = nodes with this supernode
  level++;
  // hlr_strmcpy (gSearchNode->supernode,k->name);
  strReplace (&(gSearchNode->supernode),k->name);
  if (arrayFind (gForestNodes,&gSearchNode,&i,(ARRAYORDERF)nodeOrder))
    die ("forest_descend");
  // i is now one less then the start of 'supernode', if it exists
  i++;
  if (i >= arrayMax (gForestNodes))
    return;
  if (strDiffer (arru (gForestNodes,i,ForestNode)->supernode,k->name))
    return; // supernode does not exist
  // iterate until supernode ends
  while (i < arrayMax (gForestNodes)) {
    c = arru (gForestNodes,i,ForestNode);
    if (strDiffer (c->supernode,k->name))
      break;
    forest_descend (c,level);
    i++;
  }
}

int forest_selectorPrint (Array forest /*of ForestNode*/,Texta matchNodes) {
  /**
     Print HTML selector from forest;
     the nodes 'matchNodes' are highlighted
     @param[in] forest - generated using forest_get() or NULL
     @param[in] matchNodes - NULL if no match exists
     @return 1 if at least one matchNode found, else 0
  */
  ForestNode k;
  int i;
  if (forest == NULL)
    return 0;
  gForestNodes = forest;
  gMatchNodes = matchNodes;
  gFound = 0;
  if (gSearchNode == NULL)
    gSearchNode = nodeCreate ("","",NULL);
  for (i=0;i<arrayMax (gForestNodes);i++) {
    k = arru (gForestNodes,i,ForestNode);
    if (k->supernode[0])
      break; // end of the root nodes
    forest_descend (k,0);
  }
  gForestNodes = NULL;
  gMatchNodes = NULL;
  return gFound;
}

int forest_selector (Array forest /*of ForestNode*/,Texta matchNodes,
                     Texta nodesOut,Array matchesOut) {
  /**
     Same as forest_selectorPrint, but does not print out the selection tags.
     @param[in] forest - generated using forest_get() or NULL
     @param[in] matchNodes - NULL if no match exists
     @param[in]  nodesOut - must exist
     @param[in] matchesOut - array of int; must exist
     @param[out] nodesOut - filled with hierarchically ordered and
                            marked (with dots) nodes.
     @param[out] matchesOut - same order as nodesOut.
                              Element contains 1 if corresponding node is in
                              matchNodes, else 0.
     @return 1 if at least one matchNode found, else 0
  */
  gMatches = matchesOut;
  gItems = nodesOut;
  gPrintHTML = 0;
  return forest_selectorPrint (forest,matchNodes);
}

void forest_destroy (Array forestNodes) {
  /**
     Destroys a forest.
     @param[in] forestNodes - the forest
  */
  int i;
  for (i=0;i<arrayMax (forestNodes);i++)
    nodeDestroy (arru (forestNodes,i,ForestNode));
  arrayDestroy (forestNodes);
}
