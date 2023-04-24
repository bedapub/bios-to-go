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
/** @file sequenceAlignment.c
    @brief Allows to print an alignment between 2 sequences in a
    formatted fashion.
    Module prefix sa_
*/
/*
  upon input of the 2 sequence names, sequences, frames, begins and ends,
  segment type and original sequence type, the output will be of the form:

Identities = 72/240 (30%)
                80        90       100       110       120       130       1
SWX:CATD_HUMAN  YYGEIGIGTPPQCFTVVFDTGSSNLWVPSIHCKLLDIACWIHHKYNSDKSSTYVKNGTSF
                || |: :|:|||   :: ||||||  | :     |      |  |    ||||
GE:AF200346     YYVEMTVGSPPQTLNILVDTGSSNFAVGAAPHPFL------HRYYQRQLSSTYRDLRKGV
                310       300       290       280             270       260

                40       150       160        170       180       190
SWX:CATD_HUMAN  DIHYGSGSLSGYLSQDTVSVPCQSASSASA-LGGVKVERQVFGEATKQPGITFIA----A
                 : |  |   | |  | ||:|     :  | :  :    : |   :   ||  :|    |
GE:AF200346     YVPYTQGKWEGELGTDLVSIPHGPNVTVRANIAAITESDKFFINGSNWEGILGLAYAEIA
                      250       240       230       220       210       200

                    200       210       220       230        240       250
SWX:CATD_HUMAN  KFDGILGMAYPRISVNNVLPVFDNLMQQKLVDQNIFSFYLSR-DPDAQPGGELMLGGTDS
                : |  |   :  :     :|   |:   :|       | |:: :  |  || :::|| |
GE:AF200346     RPDDSLEPFFDSLVKQTHIP---NIFSLQLCGA---GFPLNQTEALASVGGSMIIGGIDH
                      190       180          170          160       150

                     260       270       280          290         300
SWX:CATD_HUMAN  KYYKGSLSYLNVTRKAYWQVHLDQVEVASGLTL---CKEGC--EAIVDTGTSLMVGPVDE
                  | ||| |  : |: |::| : :||: :|  |   |||    ::|||:||: :  |
GE:AF200346     SLYTGSLWYTPIRREWYYEVIIVRVEI-NGQDLKMDCKEYNYDKSIVDSGTTNLRLPKKV
                  140       130       120        110       100        90

                310
SWX:CATD_HUMAN  VRELQKAIGAV
                     |:| |
GE:AF200346     FEAAVKSIKAA
                    80
Note: when displaying the alignment in a browser, the output should be enclosed by <PRE> and </PRE>
an interval in the numbering of 10 or 30 indicates amino acid or nucleotide
positions respectively.

*/
#include <ctype.h>
#include "format.h"
#include "sequtil.h"
#include "hlrmisc.h"
#include "sequenceAlignment.h"

static char *getMatchLine (char *seq1,char *seq2,int isNuc,int *length,
                           int *idCount) {
  /**
     Compares the sequences given as argument amino acid per amino acid using
     the blosum scoring matrix or nucleotide per nucleotide
     @param[in] seq1,seq2 - amino acid or nucleotide sequences
     @param[in] isNuc - protein or nucleotide type
     @param[out] length - length of the query sequence
     @param[out] idCount - number of identitical aa or nucleic acid positions
     @return alignment similarity score line
  */
  int score;
  static Stringa line=NULL;
  int i,j,k;
  char tmp[2];
  strcpy (tmp," ");

  j=0;
  k=0;
  if (line == NULL)
    line = stringCreate (40);
  stringClear (line);
  for (i=0;i<strlen (seq1);i++){
    if (seq1[i] != '-')
      j++;
    if (seq1[i] == '-'|| seq2[i] == '-')
      tmp[0] = ' ';
    else {
      if (seq1[i] == seq2[i]){
        tmp[0] = '|';
        k++;
      }
      else {
        if (isNuc)
          tmp[0] = ' ';
        else {
          score = blosum62 (seq1[i],seq2[i]);
          if (score > 0)
            tmp[0] = ':';
          else
            tmp[0] = ' ';
        }
      }
    }
    stringCat (line,tmp);
  }
  *length = j;
  *idCount = k;
  return string (line);
}

char *sa_getPositionLine (char *seq,int frame,int begin,int end,
                          int isNuc,int orgIsNuc) {
  /**
     Returns a line marking the amino acid or nucleotide position every 10
     amino acids or nucleotides depending on the type of sequence
     @param[in] seq - the sequence to be counted,
     @param[in] frame - negative if the sequence is reversed
     @param[in] begin - the position in the complete sequence at which this
                        sequence (fragment) starts
     @param[in] end - the position in the complete sequence at which this
                      sequence ends
     @param[in] isNuc - the type of sequence
     @param[in] orgIsNuc - the type of the original sequence
     @return a string containing the character positions
  */
  static Stringa line = NULL;
  int i,j,length,seqlength;
  int factor,direction;
  int count;
  int temp;
  char counter[10];

  if (line == NULL)
    line = stringCreate (40);
  stringClear (line);
  /* Enhanced Alerting system: problem in some new fastA alignment objects:
     begin should always be smaller than end */
  if (begin > end){
    temp = end;
    end = begin;
    begin = temp;
  }
  seqlength = end - begin +1;
  length=strlen (seq);
  for (i=0;i<length;i++)
    stringCat (line," ");
  if (!isNuc && orgIsNuc){
    if (seqlength <= length)
      orgIsNuc = 0;
    /* this occurs with old version of FastA where begin and end represent
       amino acid instead of nucleotide positions */
  }
  if (!isNuc && orgIsNuc)
    factor = 3; // nucleotide positions
  else
    factor = 1; // amino acid positions
  if (frame < 0) {
    direction = -1;
    count = end;
  }
  else {
    direction = 1;
    count = begin;
  }
  for (i=0;i<length;i++) {
    if (isalpha (seq[i]) || (seq[i] == '*')) {
      if (count%10 == 0) {
        sprintf (counter,"%d",count);
        if (strlen (counter) <= i+1) {
          for (j=0;j<strlen (counter);j++)
            string (line)[i+1-strlen(counter)+j] = counter[j];
        }
      }
      count = count+factor*direction;
    }
  }
  return string (line);
}

static void printName (char *name) {
  /* prints the sequence name in front of the corresponding sequence (only the
     first 15 characters) or a blank string if there's no name given
     input: the sequence name
  */
  char line[NAME_LENGTH+1];

  if (name == NULL)
    sprintf (line,"%*s",NAME_LENGTH,"");
  else
    sprintf (line,"%-*.*s",NAME_LENGTH,NAME_LENGTH-1,name);
  printf ("%s",line);
}

static int printLine (char *line,int start,int end) {
  /* prints the contents of the line given as argument from start to end
     (the specified positions)
     input: the line to be printed, the start and end positions
     returns: 1 if the whole line has been printed, 0 otherwise
  */
  int j;

  for (j=start;j<end;j++){
    if (j >= strlen (line)){
        printf ("\n");
        return 1;
    }
    printf ("%c",line[j]);
  }
  printf ("\n");
  if (j == strlen (line))
    return 1;
  else
    return 0;
}

void sa_printAlignment (char *seq1,char *seq2,char *sName1,char *sName2,
                        int frame1,int frame2,int begin1,int begin2,
                        int end1,int end2,
                        int isNuc,int orgIsNuc1,int orgIsNuc2) {
  /**
     Compares seq1 and seq2 one amino acid or nucleotide at a time, then
     prints out the alignment of the 2 sequences and the parsed
     scoring results (if relevant) in a formatted fashion with the character
     positions- uses the blosum 62 matrix
     @param[in] seq1,seq2 - the sequences
     @param[in] sName1,sName2 - the sequence names
     @param[in] frame1,frame2 - the sequence frames
     @param[in] begin1,begin2 - their starting positions
     @param[in] end1,end2 - theirending positions
     @param[in] isNuc - indicates type of sequence segment
     @param[in] orgIsNuc1,orgIsNuc2 - indicates type of orginal sequences
  */
  int i, finished;
  char *scoreLine,*numberLine1,*numberLine2;
  int fromPos,toPos;
  int length,idCount;

  scoreLine = getMatchLine (seq1,seq2,isNuc,&length,&idCount);
  printf ("Identities = %d/%d (%d%%)\n",idCount,length,(idCount*100)/length);
  numberLine1 = hlr_strdup (sa_getPositionLine (seq1,frame1,begin1,end1,
                                                isNuc,orgIsNuc1));
  numberLine2 = hlr_strdup (sa_getPositionLine (seq2,frame2,begin2,end2,
                                                isNuc,orgIsNuc2));
  i=0;
  finished=0;
  while (!finished) {
    fromPos = i*LINE_LENGTH;
    toPos = i* LINE_LENGTH + LINE_LENGTH;
    printName (NULL);
    printLine (numberLine1,fromPos,toPos);
    printName (sName1);
    printLine (seq1,fromPos,toPos);
    printName (NULL);
    printLine (scoreLine,fromPos,toPos);
    printName (sName2);
    finished=printLine (seq2,fromPos,toPos);
    printName (NULL);
    printLine (numberLine2,fromPos,toPos);
    printf ("\n");
    i++;
  }
}
