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
/** @file wiseparser.c
    @brief Knows how to parse genewise output.
    Module prefix wp_
*/
#include <ctype.h>
#include "log.h"
#include "format.h"
#include "sequtil.h"
#include "wiseparser.h"

/// the valid amino acid symbols in genewise output
#define VALID_AA "ACDEFGHIKLMNPQRSTVWY"

/* --------------- begin module wiseparser --------------------

works currently for genewise output only

implementation idea:
  first the user of this module registers functions that should
  be called when interesting points in the wise file are found.
  Then the user starts the parser, which in turn drives the functions
  the user registered before, feeding these functions with
  the current data (e.g. query name, subject name, ...).

public functions:
   void wp_init()
   void wp_run(filename)
   void wp_register_<name>(function); (returns 1 parsing should continue,
                                       else 0 to stop parser)
     where <name> is:
------------------------------------------------------------
name          when called  /
              what delivered
------------------------------------------------------------
exon          after an exon and the following intron was found
intron        after an intron was found
------------------------------------------------------------

private functions: see below, declared 'static'
*/

static Stringa lin1[6] = {NULL,NULL,NULL,NULL,NULL,NULL};
static Stringa lin[6] = {NULL,NULL,NULL,NULL,NULL,NULL};
static int gOffset,nStart;
static int (*exon_hook)(char *proQ,int pBeg,int pEnd,char *proS,int numID,
                        char *nuc,int frame,int nBeg,int nEnd,char *edit);
static int (*intron_hook)(int iBeg,int iEnd,char *spliceAAQ,char *spliceAAS);

static void processExon (int b,int e,int totLen,char **exonProQ,char **exonProS,
                         char **exonNuc,char **editFS,int *numID) {
  int i,i1;
  static Stringa nuc=NULL,pro0=NULL,pro2=NULL,edit=NULL;

  *numID = 0;
  stringCreateClear (pro0,100);
  stringCreateClear (pro2,100);
  for (i=b;i<e;i++) {
    if (strchr (VALID_AA,arru (lin[0],i,char)))
      stringCatChar (pro0,arru (lin[0],i,char));
    if (strchr (VALID_AA,arru (lin[2],i,char)))
      stringCatChar (pro2,arru (lin[2],i,char));
    if (strchr (VALID_AA,arru (lin[0],i,char)) &&
        strchr (VALID_AA,arru (lin[2],i,char)) &&
        arru (lin[0],i,char) == arru (lin[2],i,char))
      (*numID)++;
    if (arru (lin[2],i,char) == '!')
      stringCatChar (pro2,'X');
  }
  stringCreateClear (nuc,100);
  stringCreateClear (edit,10);
  for (i=b;i<e;i++) {
    if (isdigit (arru (lin[3],i,char))) {
      if (arru (lin[3],i,char) == '1') {
        stringCatInt (edit,totLen+arrayMax (nuc)+1);
        stringCat (edit,"=N:");
        stringCatInt (edit,totLen+arrayMax (nuc)+1);
        stringCat (edit,"+NN:");
        stringCatChar (nuc,'N');
      }
      else if (arru (lin[3],i,char) == '2') {
        stringCatInt (edit,totLen+arrayMax (nuc)+1);
        stringCat (edit,"=N:");
        stringCatInt (edit,totLen+arrayMax (nuc)+2);
        stringCat (edit,"=N:");
        stringCatInt (edit,totLen+arrayMax (nuc)+2);
        stringCat (edit,"+N:");
        stringCat (nuc,"NN");
      }
      else if (arru (lin[3],i,char) == '4') {
        stringCatInt (edit,totLen+arrayMax (nuc)+1);
        stringCat (edit,"-:");
        stringCatInt (edit,totLen+arrayMax (nuc)+2);
        stringCat (edit,"=N:");
        stringCatInt (edit,totLen+arrayMax (nuc)+3);
        stringCat (edit,"=N:");
        stringCatInt (edit,totLen+arrayMax (nuc)+4);
        stringCat (edit,"=N:");
        stringCat (nuc,"NNNN");
      }
      else if (arru (lin[3],i,char) == '5') {
        stringCatInt (edit,totLen+arrayMax (nuc)+1);
        stringCat (edit,"-:");
        stringCatInt (edit,totLen+arrayMax (nuc)+2);
        stringCat (edit,"-:");
        stringCatInt (edit,totLen+arrayMax (nuc)+3);
        stringCat (edit,"=N:");
        stringCatInt (edit,totLen+arrayMax (nuc)+4);
        stringCat (edit,"=N:");
        stringCatInt (edit,totLen+arrayMax (nuc)+5);
        stringCat (edit,"=N:");
        stringCat (nuc,"NNNNN");
      }
    }
    else {
      for (i1=3;i1<6;i1++)
        if (isalpha (arru (lin[i1],i,char)))
          stringCatChar (nuc,arru (lin[i1],i,char));
    }
  }
  *exonProQ = string (pro0);
  *exonProS = string (pro2);
  *exonNuc = string (nuc);
  *editFS = string (edit);
}

static void processIntron (char *introninfo,char *spliceinfo,
                           int *ibeg,int *iend,char **aaQ,char **aaS,
                           int *frame) {
  static char spliceAAQ[2];
  static char spliceAAS[2];
  char *pos;

  strcpy (spliceAAQ,"\0\0");
  strcpy (spliceAAS,"\0\0");
  if (strlen (spliceinfo) > 0) {
    if (spliceinfo[0] != '-')
      spliceAAQ[0] = spliceinfo[0];
    if (spliceinfo[2] != '-')
      spliceAAS[0] = spliceinfo[2];
  }
  *aaQ = spliceAAQ;
  *aaS = spliceAAS;
  pos = strchr (introninfo,'<');
  if (!pos)
    die ("Error looking for < in intron %s",introninfo);
  sscanf (pos+1,"%d",frame);
  pos = strchr (pos+1,'[');
  if (!pos)
    die ("Error in intron %s",introninfo);
  sscanf (pos+1,"%d",ibeg);
  pos = strchr (pos+1,':');
  if (!pos)
    die ("Error in intron %s",introninfo);
  sscanf (pos+1,"%d",iend);
  if (*ibeg > *iend) {
    *ibeg = -*ibeg;
    *iend = -*iend;
  }
}

void wp_init (int offset) {
  /**
     Initializes th wise parser.
     @param[in] offset - offset, will be added to all DNA coordinates
  */
  gOffset = offset;
  exon_hook = NULL;
  intron_hook = NULL;
}

void wp_register_exon (int (*f)(char *proQ,int pBeg,int pEnd,char *proS,
                                int numID,char *nuc,int frame,int nBeg,int nEnd,
                                char *edit)) {
  /**
     Register a function that should be called if an exon is encountered
     @param[in] f - the function
  */
  exon_hook = f;
}

void wp_register_intron (int (*f)(int iBeg,int iEnd,
                                  char *spliceAAQ,char *spliceAAS)) {
  /**
     Register a function that should be called if an intron is encountered
     @param[in] f - the function
  */
  intron_hook = f;
}

void wp_run (LineStream ls) {
  /**
     Runs the parser from a line stream.<br>
     Postcondition: the functions registered have been called
     @param[in] ls - input line stream with wise output
  */
  char *line;
  char pName[100],nName[100];
  int pStart;
  int n,p;
  int i,l,l1,c,c1;
  static Stringa intron=NULL,spliceinfo=NULL;
  int ibeg,iend;
  char *exonProQ,*exonProS,*exonNuc,*exonEdit;
  int frame;
  char *aaQ,*aaS;
  int numID;
  int cDNAlen;

  stringCreateClear (intron,100);
  stringCreateClear (spliceinfo,100);
  frame = 0;
  n = p = 0;
  cDNAlen = 0;
  for (i=0;i<6;i++) {
    stringCreateClear (lin1[i],100);
    stringCreateClear (lin[i],100);
  }
  while ((line = ls_nextLine (ls)) != NULL) {
    if (line[0] == '\0')
      continue;
    if (strNEqual (line,"See WWW help for more info",26)) {
      i = 0;
      while ((line = ls_nextLine (ls)) != NULL) {
        if (line[0] == '\0')
          continue;
        if (strNEqual (line,"//",2))
          break;
        stringCpy (lin1[i],line);
        i++;
        if (i == 6) {
          if (n == 0 && p == 0) { // scanf fails if sequence name is very long and there is no blank between name and coordinate
            sscanf (string (lin1[0]),"%99s %d %n",pName,&pStart,&p);
            while (*(line+p) == ' ')
              p++;
            sscanf (string (lin1[3]),"%99s %d %n",nName,&nStart,&n);
            while (*(line+n) == ' ')
              n++;
            if (n != p)
              die ("Format inconsistencies in lines\n%s\nand\n%s",
                   string (lin1[0]),string (lin1[3]));
          }
          l = strlen (string (lin1[0])) - 1;
          while (l > 0) {
            for (i=0;i<6;i++)
              if (arru (lin1[i],l,char) != ' ')
                break;
            if (i < 6) {
              for (i=0;i<6;i++)
                array (lin1[i],l+1,char) = '\0';
              break;
            }
            l--;
          }
          for (i=0;i<6;i++)
            stringCat (lin[i],string (lin1[i])+p);
          i = 0;
        }
      }
    }
  }
  if (stringLen (lin[0]) == 0)
    return;
  l1 = strlen (string (lin[0]));
  c = c1 = 0;
  p = pStart;
  n = nStart;
  while (c1<l1) {
    while (c1<l1 && arru (lin[4],c1,char) != '<')
      c1++;
    processExon (c,c1,cDNAlen,&exonProQ,&exonProS,&exonNuc,&exonEdit,&numID);
    cDNAlen += strlen (exonNuc);
    stringClear (intron);
    stringClear (spliceinfo);
    while (c1<l1 && arru (lin[4],c1,char) != '>') {
      stringCatChar (intron,arru (lin[4],c1,char));
      if (arru (lin[2],c1,char) != ' ')
        stringCatChar (spliceinfo,arru (lin[2],c1,char));
      c1++;
    }
    stringCatChar (intron,arru (lin[4],c1,char));
    if (arru (lin[2],c1,char) != ' ')
      stringCatChar (spliceinfo,arru (lin[2],c1,char));
    c1++;
    if (exon_hook) {
      int enb,ene;

      if (frame == 1)
        frame = 2;
      else if (frame == 2)
        frame = 1;

      if (n < 0) {
        enb = gOffset - 1 - n;
        ene = gOffset - 1 - n - strlen (exonNuc) + 1;
      }
      else {
        enb = gOffset - 1 + n;
        ene = gOffset - 1 + n + strlen (exonNuc) - 1;
      }
      if (exon_hook (exonProQ,p,p + strlen (exonProQ) - 1,exonProS,numID,
                     exonNuc,frame,enb,ene,exonEdit) == 0)
        return;
    }
    p += strlen (exonProQ);
    stringAdjust (intron); // if there is no intron string might contain a \0
    if (stringLen (intron) > 0) {
      processIntron (string (intron),string (spliceinfo),
                     &ibeg,&iend,&aaQ,&aaS,&frame);
      if (aaQ[0] != '\0')
        p++;
    }
    if (intron_hook != NULL && stringLen (intron) > 0) {
      int inb,ine;

      if (ibeg < 0) {
        inb = gOffset - 1 - ibeg;
        ine = gOffset - 1 - iend;
      }
      else {
        inb = gOffset - 1 + ibeg;
        ine = gOffset - 1 + iend;
      }
      if (intron_hook (inb,ine,aaQ,aaS) == 0)
        return;
    }
    n = iend + 1;
    c = c1;
  }
}

/* --------------- end module wiseparser -------------------- */

/*
static int exon (char *proQ,int pBeg,int pEnd,char *proS,int numID,
                 char *nuc,int frame,int nBeg,int nEnd,char *edit)
{
  printf ("%d %s %d\n",pBeg,proQ,pEnd);
  printf ("%d %s %d\n",pBeg,proS,pEnd);
  printf ("%d %s %d %d\n",nBeg,nuc,nEnd,frame);
  printf ("%s\n",edit);
  return 1;
}

static int intron (int iBeg,int iEnd,char *aaQ,char *aaS)
{
  printf ("%d %d aaq:%s aas:%s\n",iBeg,iEnd,aaQ,aaS);
  return 1;
}

int main (int argc,char **argv)
{
  LineStream ls;

  wp_init (atoi (argv[2]),atoi (argv[3]));
  wp_register_exon (exon);
  wp_register_intron (intron);
  ls = ls_createFromFile (argv[1]);
  wp_run (ls);
  ls_destroy (ls);
  return 0;
}
*/
