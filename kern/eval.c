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
/** @file eval.c
    @brief Module that evaluates mathematical expressions.
    It handles unary + and -, binary +, -, *, / and ^,
    functions sin, cos, tan, log, exp and ln.
    It handles decisions such as a>1 ? b : c.
    It substitutes variable names by the assigned value.
    It knows constants pi and e.
    Usage: eval_assignEvaluate ("set a=1; a+(a+2)");
           eval_evaluate ("(-4)^2");
    Module prefix eval_
*/
#include <ctype.h>
#include <math.h>
#include "log.h"
#include "format.h"
#include "eval.h"

enum {null,term,constant,variable,operator,function,parenthesis,
      unary1,unary2,branch};
static int gDebug = 0;

/// structure of a token
typedef struct {
  char *val; //!< value of token
  int type; //!< type of token
}Tok;

static Array gTokens = NULL;

/// structure of a variable
typedef struct {
  char *name; //!< name of variable
  char *val; //!< value of variable
}Variable;

static Array gVariables=NULL;

static Stringa t = NULL;
static Tok *currTok=NULL;

static void addTok (int type) {
  if (stringLen (t) == 0)
    return;
  currTok = arrayp (gTokens,arrayMax (gTokens),Tok);
  currTok->val = hlr_strdup (string (t));
  if (type == term) {
    if (string (t)[0] == '"') {
      if (string (t)[stringLen (t)-1] != '"')
        die ("constant not endig with \": %s",string (t));
      currTok->type = constant;
    }
    else
      currTok->type = type;
  }
  else
    currTok->type = type;
  stringClear (t);
}

static void tokenize (char *s) {
  int i = 0;

  if (gTokens == NULL)
    gTokens = arrayCreate (10,Tok);
  else {
    for (i=0;i<arrayMax (gTokens);i++) {
      currTok = arrp (gTokens,i,Tok);
      hlr_free (currTok->val);
      currTok->type = null;
    }
    arrayClear (gTokens);
  }
  if (gDebug)
    fprintf (stderr,"-------------------\nTokenize: '%s'\n\n",s);

  stringCreateOnce (t,10);
  i = 0;
  while (s[i] != '\0') {
    if (strchr ("+-",s[i]) &&
        (i == 0 ||
         (i > 0 && s[i-1] != 'e') ||
         (i > 0 && s[i-1] == 'e' &&
          (i == 1 || (i > 1 && !strchr ("0123456789.",s[i-2])))) ||
         !strchr ("0123456789",s[i+1]))) {
      addTok (term);
      stringCatChar (t,s[i]);
      addTok (operator);
    }
    else if (strchr ("/*^%",s[i])) {
      addTok (term);
      stringCatChar (t,s[i]);
      addTok (operator);
    }
    else if (strchr ("()",s[i])) {
      addTok (term);
      stringCatChar (t,s[i]);
      addTok (parenthesis);
    }
    else if (strStartsWithC (s+i,"sin") ||
             strStartsWithC (s+i,"cos") ||
             strStartsWithC (s+i,"tan") ||
             strStartsWithC (s+i,"log") ||
             strStartsWithC (s+i,"exp")) {
      stringCatChar (t,s[i]);
      stringCatChar (t,s[i+1]);
      stringCatChar (t,s[i+2]);
      addTok (function);
      i+=2;
    }
    else if (strStartsWithC (s+i,"ln")) {
      stringCatChar (t,s[i]);
      stringCatChar (t,s[i+1]);
      addTok (function);
      i++;
    }
    else if (strchr ("0123456789.abcdefghijklmnopqrstuvwxyz-+\"",s[i]))
      stringCatChar (t,s[i]);
    else if (s[i] == '?' || s[i] == ':') {
      addTok (term);
      stringCatChar (t,s[i]);
      addTok (branch);
    }
    else if (strchr ("<>!=",s[i])) {
      addTok (term);
      stringCatChar (t,s[i]);
      if (s[i+1] == '=') {
        stringCatChar (t,s[i+1]);
        i++;
      }
      addTok (operator);
    }
    else
      die ("unexpected char %c",s[i]);
    i++;
  }
  addTok (term);
  /*
  if (gDebug) {
    fprintf ("\nTokens\n");
    for (i=0;i<arrayMax (gTokens);i++) {
      currTok = arrp (gTokens,i,Tok);
      fprintf (stderr,"%d\t%s\t%d\n",i,currTok->val,currTok->type);
    }
  }
  */
}

static void processTokens (void) {
  int i;

  // identify unary + and -
  for (i=0;i<arrayMax (gTokens);i++) {
    currTok = arrp (gTokens,i,Tok);
    if (strEqual (currTok->val,"+") || strEqual (currTok->val,"-")) {
      if (i == 0) {
        if (arrayMax (gTokens) > 2 &&
            arrp (gTokens,i+1,Tok)->type == term &&
            !strEqual (arrp (gTokens,i+2,Tok)->val,"^")) {
          if (strEqual (currTok->val,"+"))
            strReplace (&currTok->val,"p");
          else
            strReplace (&currTok->val,"m");
          currTok->type = unary1;
        }
      }
      else if (i == arrayMax (gTokens)-1)
        die ("expression cannot end in + or -");
      else {
        if (strEqual (arrp (gTokens,i+1,Tok)->val,"(") ) {
          if (arrp (gTokens,i-1,Tok)->type == operator) {
            if (strEqual (currTok->val,"+"))
              strReplace (&currTok->val,"P");
            else
              strReplace (&currTok->val,"M");
            currTok->type = unary2;
          }
        }
        else if (strEqual (arrp (gTokens,i-1,Tok)->val,")") )
          ;
        else if (!((arrp (gTokens,i-1,Tok)->type == term &&
                   (arrp (gTokens,i+1,Tok)->type == term ||
                    ((strEqual (arrp (gTokens,i+1,Tok)->val,"+") ||
                      strEqual (arrp (gTokens,i+1,Tok)->val,"-")) &&
                     arrp (gTokens,i+2,Tok)->type == term))))) {
          if (strEqual (currTok->val,"+"))
            strReplace (&currTok->val,"p");
          else
            strReplace (&currTok->val,"m");
          currTok->type = unary1;
        }
      }
    }
  }

  {
    Tok *currTok1 = malloc(sizeof(Tok));
    static Stringa val1=NULL;
    int k;
    int changed;

    for (i=0;i<arrayMax (gTokens);i++) {
      currTok = arrp (gTokens,i,Tok);
      if (currTok->type != unary1)
        continue;
      currTok1 = arrp (gTokens,i+1,Tok);
      stringCreateClear (val1,10);
      if (strEqual (currTok->val,"m"))
        stringCat (val1,"-");
      stringCat (val1,currTok1->val);
      strReplace (&currTok1->val,string (val1));
      hlr_free (currTok->val);
    }

    k = 0;
    for (i=0;i<arrayMax (gTokens);i++) {
      currTok = arrp (gTokens,i,Tok);
      if (currTok->type == unary1)
        continue;
      arru (gTokens,k,Tok) = arru (gTokens,i,Tok);
      k++;
    }
    arraySetMax (gTokens,k);

    changed = 1;
    while (changed) {
      changed = 0;
      for (i=0;i<arrayMax (gTokens);i++) {
        currTok = arrp (gTokens,i,Tok);
        if (currTok->type != unary2)
          continue;
        changed = 1;
        if (currTok->val[0] == 'P')
          hlr_free (currTok->val);
        else {
          k = arrayMax (gTokens)-1;
          while (k>i) {
            array (gTokens,k+1,Tok) = arru (gTokens,k,Tok);
            k--;
          }
          currTok = arrp (gTokens,i,Tok);
          currTok->val = hlr_strdup ("-1");
          currTok->type = term;
          currTok = arrp (gTokens,i+1,Tok);
          currTok->val = hlr_strdup ("*");
          currTok->type = operator;
        }
        break;
      }
      k = 0;
      for (i=0;i<arrayMax (gTokens);i++) {
        currTok = arrp (gTokens,i,Tok);
        if (currTok->val == NULL)
          continue;
        arru (gTokens,k,Tok) = arru (gTokens,i,Tok);
        k++;
      }
      arraySetMax (gTokens,k);
    }
  }

  if (gDebug) {
    fprintf (stderr,"Tokens processed\n");
    for (i=0;i<arrayMax (gTokens);i++) {
      currTok = arrp (gTokens,i,Tok);
      fprintf (stderr,"%d\t%s\t%d\n",i,currTok->val,currTok->type);
    }
    fprintf (stderr,"\n");
  }
}

/// a node in the parse tree
typedef struct _node_ {
  struct _node_ *parent; //!< parent node
  int bInd; //!< begin index into the expression
  int eInd; //!< end index into the expression
  char *op; //!< operator
  struct _node_ *left; //!< left node
  struct _node_ *right; //!< right node
  struct _node_ *t; //!< then branch
  struct _node_ *e; //!< else branch
}Node;

static Node topNode = {NULL,-1,-1,NULL,NULL,NULL,NULL,NULL};
static Node *currNode;
static int expanded = 0;
static Texta results = NULL;

static void removeRedundantParenths (Node *n) {
  int removed;

  removed = 1;
  while (removed) {
    int inPar = 0;
    int i;

    currTok = arrp (gTokens,n->bInd,Tok);
    if (currTok->val[0] != '(')
      break;
    removed = 0;
    for (i=n->bInd;i<=n->eInd;i++) {
      currTok = arrp (gTokens,i,Tok);
      if (currTok->val[0] == '(')
        inPar++;
      else if (currTok->val[0] == ')')
        inPar--;
      if (inPar == 0) {
        if (i == n->eInd) {
          n->bInd++;
          n->eInd--;
          removed = 1;
        }
        break;
      }
    }
  }
}

static void expandTree (Node *currNode,char *seps) {
  int i;
  Node *newNode;
  int inPar;

  if (currNode->bInd == currNode->eInd)
    return;
  removeRedundantParenths (currNode);
  currTok = arrp (gTokens,currNode->bInd,Tok);
  if (currTok->type == function &&
      currNode->left == NULL && currNode->right == NULL &&
      strEqual (arrp (gTokens,currNode->bInd+1,Tok)->val,"(") &&
      strEqual (arrp (gTokens,currNode->eInd,Tok)->val,")")) {
    if (gDebug) {
      int k;
      Tok *currTok1 =  malloc(sizeof(Tok));

      fprintf (stderr,"in: ");
      for (k=currNode->bInd;k<=currNode->eInd;k++) {
        currTok1 = arrp (gTokens,k,Tok);
        fprintf (stderr,"%s ",currTok1->val);
      }
      fprintf (stderr,"\n");
    }
    expanded = 1;
    currNode->op = hlr_strdup (currTok->val);
    newNode = calloc (1,sizeof (Node));
    newNode->parent = currNode;
    newNode->bInd = currNode->bInd+1;
    newNode->eInd = currNode->eInd;
    removeRedundantParenths (newNode);
    if (gDebug) {
      int k;
      Tok *currTok1 = malloc(sizeof(Tok));

      fprintf (stderr,"l:  ");
      for (k=newNode->bInd;k<=newNode->eInd;k++) {
        currTok1 = arrp (gTokens,k,Tok);
        fprintf (stderr,"%s ",currTok1->val);
      }
      fprintf (stderr,"\n");
    }
    newNode->left = NULL;
    newNode->right = NULL;
    currNode->left = newNode;
    return;
  }
  inPar = 0;
  if (strEqual (seps,"+-") || strEqual (seps,"*/")) {
    for (i=currNode->eInd;i>=currNode->bInd;i--) {
      currTok = arrp (gTokens,i,Tok);
      if (currTok->val[1] != '\0')
        continue;
      //skip parentheses
      if (currTok->val[0] == ')')
        inPar++;
      else if (currTok->val[0] == '(')
        inPar--;
      if (inPar < 0)
        die ("unbalanced parentheses");
      if (inPar > 0)
        continue;
      if (strchr (seps,currTok->val[0])) {
        if (gDebug) {
          int k;
          Tok *currTok1 = malloc(sizeof(Tok));

          fprintf (stderr,"in: ");
          for (k=currNode->bInd;k<=currNode->eInd;k++) {
            currTok1 = arrp (gTokens,k,Tok);
            fprintf (stderr,"%s ",currTok1->val);
          }
          fprintf (stderr,"\n");
        }
        expanded = 1;
        currNode->op = hlr_strdup (currTok->val);
        newNode = calloc (1,sizeof (Node));
        newNode->parent = currNode;
        newNode->bInd = currNode->bInd;
        newNode->eInd = i-1;
        removeRedundantParenths (newNode);
        if (gDebug) {
          int k;
          Tok *currTok1 = malloc(sizeof(Tok));

          fprintf (stderr,"l:  ");
          for (k=newNode->bInd;k<=newNode->eInd;k++) {
            currTok1 = arrp (gTokens,k,Tok);
            fprintf (stderr,"%s ",currTok1->val);
          }
          fprintf (stderr,"\n");
        }
        newNode->left = NULL;
        newNode->right = NULL;
        currNode->left = newNode;
        newNode = calloc (1,sizeof (Node));
        newNode->parent = currNode;
        newNode->bInd = i+1;
        newNode->eInd = currNode->eInd;
        removeRedundantParenths (newNode);
        if (gDebug) {
          int k;
          Tok *currTok1 = malloc(sizeof(Tok));

          fprintf (stderr,"r:  ");
          for (k=newNode->bInd;k<=newNode->eInd;k++) {
            currTok1 = arrp (gTokens,k,Tok);
            fprintf (stderr,"%s ",currTok1->val);
          }
          fprintf (stderr,"\n");
        }
        newNode->left = NULL;
        newNode->right = NULL;
        currNode->right = newNode;
        break;
      }
    }
  }
  else if (strEqual (seps,"^")) {
    for (i=currNode->bInd;i<=currNode->eInd;i++) {
      currTok = arrp (gTokens,i,Tok);
      if (currTok->val[1] != '\0')
        continue;
      //skip parentheses
      if (currTok->val[0] == '(')
        inPar++;
      else if (currTok->val[0] == ')')
        inPar--;
      if (inPar < 0)
        die ("unbalanced parentheses");
      if (inPar > 0)
        continue;
      if (strchr (seps,currTok->val[0])) {
        if (gDebug) {
          int k;
          Tok *currTok1 = malloc(sizeof(Tok));

          fprintf (stderr,"in: ");
          for (k=currNode->bInd;k<=currNode->eInd;k++) {
            currTok1 = arrp (gTokens,k,Tok);
            fprintf (stderr,"%s ",currTok1->val);
          }
          fprintf (stderr,"\n");
        }
        expanded = 1;
        currNode->op = hlr_strdup (currTok->val);
        newNode = calloc (1,sizeof (Node));
        newNode->parent = currNode;
        newNode->bInd = currNode->bInd;
        newNode->eInd = i-1;
        removeRedundantParenths (newNode);
        if (gDebug) {
          int k;
          Tok *currTok1 = malloc(sizeof(Tok));

          fprintf (stderr,"l:  ");
          for (k=newNode->bInd;k<=newNode->eInd;k++) {
            currTok1 = arrp (gTokens,k,Tok);
            fprintf (stderr,"%s ",currTok1->val);
          }
          fprintf (stderr,"\n");
        }
        newNode->left = NULL;
        newNode->right = NULL;
        currNode->left = newNode;

        newNode = calloc (1,sizeof (Node));
        newNode->parent = currNode;
        newNode->bInd = i+1;
        newNode->eInd = currNode->eInd;
        removeRedundantParenths (newNode);
        if (gDebug) {
          int k;
          Tok *currTok1 = malloc(sizeof(Tok));

          fprintf (stderr,"r:  ");
          for (k=newNode->bInd;k<=newNode->eInd;k++) {
            currTok1 = arrp (gTokens,k,Tok);
            fprintf (stderr,"%s ",currTok1->val);
          }
          fprintf (stderr,"\n");
        }
        newNode->left = NULL;
        newNode->right = NULL;
        currNode->right = newNode;
        break;
      }
    }
  }
  else if (strEqual (seps,"?")) {
    int i1;
    int k;
    Tok *currTok1 = malloc(sizeof(Tok));

    for (i=currNode->bInd;i<=currNode->eInd;i++) {
      currTok = arrp (gTokens,i,Tok);
      //skip parentheses
      if (currTok->val[0] == '(')
        inPar++;
      else if (currTok->val[0] == ')')
        inPar--;
      if (inPar < 0)
        die ("unbalanced parentheses");
      if (inPar > 0)
        continue;
      if (currTok->val[0] == '?') {
        int b,e;

        if (gDebug) {
          fprintf (stderr,"branch:  ");
          for (k=currNode->bInd;k<=currNode->eInd;k++) {
            currTok1 = arrp (gTokens,k,Tok);
            fprintf (stderr,"%s ",currTok1->val);
          }
          fprintf (stderr,"\n");
        }
        expanded = 1;
        b = currNode->bInd;
        e = i-1;
        while (strEqual (arrp (gTokens,b,Tok)->val,"(") &&
               strEqual (arrp (gTokens,e,Tok)->val,")")) {
          inPar = 0;
          for (i1=b;i1<=e;i1++) {
            currTok = arrp (gTokens,i1,Tok);
            if (currTok->val[0] == '(')
              inPar++;
            else if (currTok->val[0] == ')')
              inPar--;
            else if (currTok->type == operator) {
              if (inPar == 0)
                break;
            }
          }
          if (i1<=e)
            break;
          b++;
          e--;
        }
        for (k=currNode->bInd;k<i;k++) {
          currTok1 = arrp (gTokens,k,Tok);
          if (currTok1->type == operator) {
            currNode->op = hlr_strdup (currTok1->val);
            newNode = calloc (1,sizeof (Node));
            newNode->parent = currNode;
            newNode->bInd = b;
            newNode->eInd = k-1;
            removeRedundantParenths (newNode);
            if (gDebug) {
              int k1;
              Tok *currTok2;

              fprintf (stderr,"bl:  ");
              for (k1=newNode->bInd;k1<=newNode->eInd;k1++) {
                currTok2 = arrp (gTokens,k1,Tok);
                fprintf (stderr,"%s ",currTok2->val);
              }
              fprintf (stderr,"\n");
            }
            newNode->left = NULL;
            newNode->right = NULL;
            currNode->left = newNode;
            newNode = calloc (1,sizeof (Node));
            newNode->parent = currNode;
            newNode->bInd = k+1;
            newNode->eInd = e;
            removeRedundantParenths (newNode);
            if (gDebug) {
              int k1;
              Tok *currTok2;

              fprintf (stderr,"br:  ");
              for (k1=newNode->bInd;k1<=newNode->eInd;k1++) {
                currTok2 = arrp (gTokens,k1,Tok);
                fprintf (stderr,"%s ",currTok2->val);
              }
              fprintf (stderr,"\n");
            }
            newNode->left = NULL;
            newNode->right = NULL;
            currNode->right = newNode;
            break;
          }
        }
        if (currTok1->type != operator)
          die ("no operator found in if part of ?: expression");
        break;
      }
    }
    // do then/lese
    for (i1=i+1;i1<=currNode->eInd;i1++) {
      currTok = arrp (gTokens,i1,Tok);
      //skip parentheses
      if (currTok->val[0] == '(')
        inPar++;
      else if (currTok->val[0] == ')')
        inPar--;
      if (inPar < 0)
        die ("unbalanced parentheses");
      if (inPar > 0)
        continue;
      if (currTok->val[0] == ':') {
        newNode = calloc (1,sizeof (Node));
        newNode->parent = currNode;
        newNode->bInd = i+1;
        newNode->eInd = i1-1;
        removeRedundantParenths (newNode);
        if (gDebug) {
          int k1;
          Tok *currTok2;

          fprintf (stderr,"then:  ");
          for (k1=newNode->bInd;k1<=newNode->eInd;k1++) {
            currTok2 = arrp (gTokens,k1,Tok);
            fprintf (stderr,"%s ",currTok2->val);
          }
          fprintf (stderr,"\n");
        }
        newNode->left = NULL;
        newNode->right = NULL;
        currNode->t = newNode;
        newNode = calloc (1,sizeof (Node));
        newNode->parent = currNode;
        newNode->bInd = i1+1;
        newNode->eInd = currNode->eInd;
        removeRedundantParenths (newNode);
        if (gDebug) {
          int k1;
          Tok *currTok2;

          fprintf (stderr,"else:  ");
          for (k1=newNode->bInd;k1<=newNode->eInd;k1++) {
            currTok2 = arrp (gTokens,k1,Tok);
            fprintf (stderr,"%s ",currTok2->val);
          }
          fprintf (stderr,"\n");
        }
        newNode->left = NULL;
        newNode->right = NULL;
        currNode->e = newNode;
        break;
      }
    }
  }
  else
    die ("unknown separators %s",seps);
}

static void walkTree (Node *currNode,char *seps) {
  if (currNode->left == NULL && currNode->right == NULL)
    expandTree (currNode,seps);
  else {
    if (currNode->left != NULL)
      walkTree (currNode->left,seps);
    if (currNode->right != NULL)
      walkTree (currNode->right,seps);
    if (currNode->t != NULL)
      walkTree (currNode->t,seps);
    if (currNode->e != NULL)
      walkTree (currNode->e,seps);
  }
}

static void printTree (Node *currNode,char mode,int level) {
  int i;

  for (i=0;i<level;i++)
    fprintf (stderr,"  ");

  if (currNode->left == NULL && currNode->right == NULL) {
    if (mode != ' ')
      fprintf (stderr,"%c:",mode);
    for (i=currNode->bInd;i<=currNode->eInd;i++) {
      currTok = arrp (gTokens,i,Tok);
      fprintf (stderr,"%s ",currTok->val);
    }
    fprintf (stderr,"\n");
  }
  else {
    fprintf (stderr,"%s\n",currNode->op);
    if (currNode->left != NULL)
      printTree (currNode->left,' ',level+1);
    if (currNode->right != NULL)
      printTree (currNode->right,' ',level+1);
    if (currNode->t != NULL)
      printTree (currNode->t,'t',level+1);
    if (currNode->e != NULL)
      printTree (currNode->e,'e',level+1);
  }
}

static void destroyTree (Node *n) {
  if (n->left == NULL && n->right == NULL)
    return;
  if (n->left->left == NULL && n->left->right == NULL) {
    hlr_free (n->left->op);
    hlr_free (n->left);
    return;
  }
  if (n->right->left == NULL && n->right->right == NULL) {
    hlr_free (n->right->op);
    hlr_free (n->right);
    return;
  }
  destroyTree (n->left);
  destroyTree (n->right);
  if (n->t != NULL)
    destroyTree (n->t);
  if (n->e != NULL)
    destroyTree (n->e);
}

static void tree (void) {
  destroyTree (&topNode);
  if (gDebug)
    fprintf (stderr,"Create tree\n");
  topNode.parent = NULL;
  topNode.bInd = 0;
  topNode.eInd = arrayMax (gTokens)-1;
  hlr_free (topNode.op);
  topNode.left = NULL;
  topNode.right = NULL;
  topNode.t = NULL;
  topNode.e = NULL;
  currNode = &topNode;
  while (1) {
    expanded = 0;
    walkTree (currNode,"+-");
    if (!expanded) {
      walkTree (currNode,"*/");
      if (!expanded) {
        walkTree (currNode,"^");
        if (!expanded) {
          walkTree (currNode,"?");
        }
      }
    }
    if (expanded == 0) {
      break;
    }
  }
  if (gDebug) {
    fprintf (stderr,"\nTree:\n");
    printTree (&topNode,' ',0);
    fprintf (stderr,"\n");
  }
}

static void termType (void) {
  int i,k = 0;
  double d = 0.0;
  static Texta vars=NULL;
  Variable *currVariable = malloc(sizeof(Variable));
  int offs;

  if (vars == NULL)
    vars = textCreate (10);
  else
    textClear (vars);
  for (i=0;i<arrayMax (gTokens);i++) {
    currTok = arrp (gTokens,i,Tok);
    if (currTok->type != term)
      continue;
    if (strEqual (currTok->val,"+e") ||
        strEqual (currTok->val,"e")) {
      strReplace (&currTok->val,"2.71828183");
      currTok->type = constant;
    }
    else if (strEqual (currTok->val,"-e")) {
      strReplace (&currTok->val,"-2.71828183");
      currTok->type = constant;
    }
    else if (strEqual (currTok->val,"+pi") ||
             strEqual (currTok->val,"pi")) {
      strReplace (&currTok->val,"3.14159265");
      currTok->type = constant;
    }
    else if (strEqual (currTok->val,"-pi")) {
      strReplace (&currTok->val,"-3.14159265");
      currTok->type = constant;
    }
    else if (sscanf (currTok->val,"%lg",&d) == 1) {
      currTok->type = constant;
    }
    else if (isdigit (currTok->val[0]))
      die ("variable names cannot start with digit:",currTok->val);
    else {
      int offs;

      if (strEqual (currTok->val,"+") || strEqual (currTok->val,"-")) {
        currTok->type = operator;
      }
      else {
        currTok->type = variable;
        offs = 0;
        if (currTok->val[0] == '+' ||
            currTok->val[0] == '-')
          offs = 1;
        for (i=0;i<arrayMax (vars);i++)
          if (strEqual (currTok->val+offs,textItem (vars,i)))
            break;
        if (i==arrayMax (vars))
          textAdd (vars,currTok->val+offs);
      }
    }
  }
  if (arrayMax (vars) > 0) {
    if (gVariables == NULL)
      die ("variables occur in expression but no values assigned");
  }
  for (i=0;i<arrayMax (vars);i++) {
    for (k=0;k<arrayMax (gVariables);k++) {
      currVariable = arrp (gVariables,k,Variable);
      if (strEqual (textItem (vars,i),currVariable->name))
        break;
    }
    if (k==arrayMax (gVariables))
      die ("no value assigned to variable %s",textItem (vars,i));
  }
  if (gDebug) {
    if (gVariables != NULL) {
      fprintf (stderr,"Assigned variables\n");
      if (arrayMax (gVariables) > 0) {
        for (k=0;k<arrayMax (gVariables);k++) {
          currVariable = arrp (gVariables,k,Variable);
          fprintf (stderr,"%s = %s\n",currVariable->name,
                  currVariable->val != NULL ? currVariable->val :
                  "being evaluated");
        }
        fprintf (stderr,"\n");
      }
      else
        fprintf (stderr,"none\n");
    }
  }
  // replace variables
  for (i=0;i<arrayMax (gTokens);i++) {
    currTok = arrp (gTokens,i,Tok);
    if (currTok->type != variable)
      continue;
    offs = 0;
    if (currTok->val[0] == '+' || currTok->val[0] == '-')
      offs = 1;
    for (k=0;k<arrayMax (gVariables);k++) {
      currVariable = arrp (gVariables,k,Variable);
      if (strEqual (currTok->val+offs,currVariable->name)) {
        if (offs == 0)
          strReplace (&currTok->val,currVariable->val);
        else {
          static Stringa s = NULL;

          stringCreateClear (s,20);
          stringCatChar (s,currTok->val[0]);
          stringCat (s,currVariable->val);
          strReplace (&currTok->val,string (s));
        }
        break;
      }
    }
  }
}

static char *operation1 (char *op,char *vStr) {
  double v,res = 0.0;
  static Stringa resStr = NULL;

  v = atof (vStr);
  if (strEqual (op,"log"))
    res = log10 (v);
  else if (strEqual (op,"ln"))
    res = log (v);
  else if (strEqual (op,"exp"))
    res = exp (v);
  else if (strEqual (op,"sin"))
    res = sin (v);
  else if (strEqual (op,"cos"))
    res = cos (v);
  else if (strEqual (op,"tan"))
    res = tan (v);
  else
    die ("operation1:%s operation not defined",op);
  stringCreateOnce (resStr,20);
  stringPrintf (resStr,"%g",res);
  textAdd (results,string (resStr));
  return textItem (results,arrayMax (results)-1);
}

static char *operation2 (char *op,char *lStr,char *rStr) {
  double l,r,res = 0.0;
  static Stringa resStr = NULL;

  l = atof (lStr);
  r = atof (rStr);
  if (strEqual (op,"+"))
    res = l + r;
  else if (strEqual (op,"-"))
    res = l - r;
  else if (strEqual (op,"*"))
    res = l * r;
  else if (strEqual (op,"/"))
    res = l / r;
  else if (strEqual (op,"^"))
    res = pow (l,r);
  else
    die ("operation2:%s operation not defined",op);
  stringCreateOnce (resStr,20);
  stringPrintf (resStr,"%g",res);
  textAdd (results,string (resStr));
  return textItem (results,arrayMax (results)-1);
}

/// symbol for then branch
#define THEN 1
/// symbol for else branch
#define ELSE 2

static char *eval (Node *currNode) {
  if (currNode->t != NULL && currNode->e != NULL) {
    double l,r = 0.0;
    int branch = 0;

    l = atof (eval (currNode->left));
    r = atof (eval (currNode->right));
    if (strEqual (currNode->op,">")) {
      if (l > r)
        branch = THEN;
      else
        branch = ELSE;
    }
    else if (strEqual (currNode->op,">=")) {
      if (l >= r)
        branch = THEN;
      else
        branch = ELSE;
    }
    else if (strEqual (currNode->op,"<")) {
      if (l < r)
        branch = THEN;
      else
        branch = ELSE;
    }
    else if (strEqual (currNode->op,"<=")) {
      if (l <= r)
        branch = THEN;
      else
        branch = ELSE;
    }
    else if (strEqual (currNode->op,"==")) {
      if (l == r)
        branch = THEN;
      else
        branch = ELSE;
    }
    else
      die ("unexpected operator %s",currNode->op);
    if (branch == THEN)
      return eval (currNode->t);
    else
      return eval (currNode->e);

  }
  else  if (currNode->left != NULL && currNode->right != NULL) {
    if (currNode->left->op == NULL && currNode->right->op == NULL)
      return operation2 (currNode->op,
                         arrp (gTokens,currNode->left->bInd,Tok)->val,
                         arrp (gTokens,currNode->right->bInd,Tok)->val);
    else if (currNode->left->op != NULL && currNode->right->op != NULL)
      return operation2 (currNode->op,
                         eval (currNode->left),
                         eval (currNode->right));
    else if (currNode->left->op != NULL && currNode->right->op == NULL)
      return operation2 (currNode->op,
                         eval (currNode->left),
                         arrp (gTokens,currNode->right->bInd,Tok)->val);
    else
      return operation2 (currNode->op,
                         arrp (gTokens,currNode->left->bInd,Tok)->val,
                         eval (currNode->right));
  }
  else if (currNode->left == NULL && currNode->right != NULL) {
    if (currNode->right->op != NULL)
      return operation1 (currNode->op,
                         eval (currNode->right));
    else
      return operation1 (currNode->op,
                         arrp (gTokens,currNode->right->bInd,Tok)->val);
  }
  else if (currNode->left != NULL && currNode->right == NULL) {
    if (currNode->left->op != NULL)
      return operation1 (currNode->op,
                         eval (currNode->left));
    else
      return operation1 (currNode->op,
                         arrp (gTokens,currNode->left->bInd,Tok)->val);
  }
  else if (currNode->left == NULL && currNode->right == NULL)
    return arrp (gTokens,currNode->bInd,Tok)->val;
  die ("unknown condition in eval");
  return NULL;
}

static void pack (char *s) {
  int i,k;

  i=0;
  k=0;
  while (s[i] != '\0') {
    if (!isspace (s[i])) {
      s[k] = tolower (s[i]);
      k++;
    }
    i++;
  }
  s[k] = '\0';
}

char *eval_evaluate (char *s) {
  /**
     Takes an expression but no 'set' commands.
     The expression must be packed.
     @param[in] s - the expression
     @return the result of the evaluation
  */
  if (s[0] == '"') {
    if (s[strlen (s)-1] != '"')
      die ("expected \" at end of %s",s);
    s[strlen (s)-1] = '\0';
    return s+1;
  }
  tokenize (s);
  processTokens ();
  termType ();
  tree ();
  return eval (&topNode);
}

void eval_setVariable (char *name,char *val) {
  /**
     Assigns values to variables that can later be used in expressions
     submitted to eval_evaluate
     @param[in] name - name of the variable
     @param[in] val - value of the variable
  */
  int i;
  Variable *currVariable = malloc(sizeof(Variable));
  char *r;

  if (gVariables == NULL)
    gVariables = arrayCreate (10,Variable);
  for (i=0;i<arrayMax (gVariables);i++) {
    currVariable = arrp (gVariables,i,Variable);
    if (strEqual (currVariable->name,name))
      break;
  }
  if (i==arrayMax (gVariables)) {
    currVariable = arrayp (gVariables,arrayMax (gVariables),Variable);
    currVariable->name = hlr_strdup (name);
  }
  r = eval_evaluate (val);
  currVariable->val = hlr_strdup (r);
}

char *eval_assignEvaluate (char *s) {
  /**
     Takes expresssions including set statements such as
     set a=2;set b=a+1;2*(a+b);
     set statements must be separated by semicolon<br>
     Internally calls eval_setVariable and eval_evaluate
     @param[in] s - the expression
     @return the result of the evaluation
  */
  static char *s1 = NULL;
  WordIter wi;
  char *word;
  char *pos;

  strReplace (&s1,s);
  strTranslate (s1," \n\t","");
  pack (s1);
  if (results == NULL)
    results = textCreate (10);
  else
    textClear (results);
  wi = wordIterCreate (s1,";",1);
  while ((word = wordNext (wi)) != NULL) {
    if (strStartsWithC (word,"set")) {
      pos = strchr (word,'=');
      if (!pos)
        die ("expected = in variable assignment: %s",word);
      *pos = '\0';
      eval_setVariable (word+3,pos+1);
    }
    else {
      return eval_evaluate (word);
    }
  }
  wordIterDestroy (wi);
  return NULL;
}

void eval_setDebug (int mode) {
  /**
     Sets the debug behavior
     @param[in] mode - 1 writes debugging messages like tokens, parse tree,
                       etc. to stderr, in mode 0 (default) is silent
  */
  gDebug = mode;
}
