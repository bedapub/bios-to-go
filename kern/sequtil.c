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
/** @file sequtil.c
    @brief Module containing sequence utilities.
*/
#include <ctype.h>
#include <time.h>

#include "log.h"
#include "format.h"
#include "hlrmisc.h"
#include "sequtil.h"

// #define DEBUG

char complement (char b) {
  /**
     Get the complement a nucleotide
     @param[in] b - valid base
     @return complement of b
  */
  char *from = "acgtumrwsykvhdbxnACGTUMRWSYKVHDBXN.- *";
  char *to =   "tgcaakywsrmbdhvxnTGCAAKYWSRMBDHVXN.- *";
  char *p = strchr (from,b);
  if (p == NULL)
    die ("complement: cannot complement base %c", b);
  return to[p-from];
}

void revcomp (char *s) {
  /**
     Calcuate reverse complement of a DNA sequence
     @param[in] s - sequence
     @param[out] s - reverse complement
  */
  char *p,*q;
  char c;

  // complement the whole sequence
  p = s-1;
  while ((c = *(++p)) != '\0')
    *p = complement (c);
  // reverse in-place
  p = s;
  q = s + strlen (s) - 1;
  while (p < q) {
    c = *p;
    *p = *q;
    *q = c;
    ++p;
    --q;
  }
}

char *iupac2alleles (char c) {
  /**
     Returns all bases encoded by an IUPAC code.
     @param[in] c - IUPAC code
     @return string containing matching bases
  */
  char c1 = toupper (c);
  if (c1 == 'A')
    return "A";
  if (c1 == 'C')
    return "C";
  if (c1 == 'G')
    return "G";
  if (c1 == 'T')
    return "T";
  if (c1 == 'U')
    return "T";
  if (c1 == 'M')
    return "AC";
  if (c1 == 'R')
    return "AG";
  if (c1 == 'W')
    return "AT";
  if (c1 == 'S')
    return "CG";
  if (c1 == 'Y')
    return "CT";
  if (c1 == 'K')
    return "GT";
  if (c1 == 'V')
    return "ACG";
  if (c1 == 'H')
    return "ACT";
  if (c1 == 'D')
    return "AGT";
  if (c1 == 'B')
    return "CGT";
  if (c1 == 'X')
    return "ACGT";
  if (c1 == 'N')
    return "ACGT";
  if (c1 == ' ')
    return " ";
  return "";
}

void printSeq (FILE *strm,char *s) {
  /**
     Print sequence 's' on stream 'strm' in EMBL format
     @param[in] strm - the stream
     @param[in] s - the sequence
  */
  int a,c,g,t,o;
  char ch,*cp;
  int i;

  a = 0;
  c = 0;
  g = 0;
  t = 0;
  o = 0;
  cp = s - 1;
  while ((ch = *(++cp)) != '\0') {
    ch = tolower (ch);
    switch (ch) {
      case 'a': a++; break;
      case 'c': c++; break;
      case 'g': g++; break;
      case 't': t++; break;
      default:  o++;
    }
  }
  fprintf (strm,"SQ   Sequence %ld BP; %d A; %d C; %d G; %d T; %d other;",
           strlen (s),a,c,g,t,o);
  cp = s - 1;
  i = 0;
  while ((ch = *(++cp)) != '\0') {
    if (i++ % 60 == 0)
      fprintf (strm,"\n     ");
    fputc (ch,strm);
  }
  fprintf (strm,"\n");
}

char *seqBufferFasta (char *desc,char *s) {
  /**
     Returna sequence 's' with description 'desc' in FASTA format.<br>
     Memory belongs to the function and is stable until next call.
     @param[in] desc - if NULL, no line with > will be printed, only sequence
     @param[in] s - sequence
     @return a buffer containing the fasta formatted seuence
  */
  static Stringa buf = NULL;
  char ch;
  char *cp = s - 1;
  int i = -1;
  stringCreateClear (buf,500);

  if (desc != NULL)
    stringPrintf (buf,">%s\n",desc);
  while ((ch = *(++cp)) != '\0') {
    if (++i % 50 == 0 && i)
      stringCatChar (buf,'\n');
    stringCatChar (buf,ch);
  }
  stringCatChar (buf,'\n');
  return string (buf);
}

void printSeqFasta (FILE *strm,char *desc,char *s) {
  /**
     Print sequence 's' with description 'desc'
     on stream 'strm' in FASTA format
     @param[in] strm - the stream
     @param[in] desc - if NULL then no line with > printed
     @param[in] s - the sequence
  */
  fprintf (strm,"%s",seqBufferFasta (desc,s));
}

void printSeqGCG (FILE *stream,char *name,char *seq,int isNuc)
{
  /**
     Prints a sequence in GCG format including the line with ..
     @param[in] stream - the stream
     @param[in] name - name of the sequence
     @param[in] seq - the sequence
     @param[in] isNuc - type of the sequence: 1=nuc, 0=pro
  */
  int sl;
  int i;

  sl = strlen (seq);
  fprintf (stream,"\n%.150s  Length: %d  %s  Type: %c  Check: %d  ..",
           name,sl,GCGtimeStamp (),isNuc ? 'N' : 'P',GCGcheckSum (seq));
  for (i=0;i<sl;i++) {
    if (i % 50 == 0) // new line starts
      fprintf (stream,"\n\n%8d  ",i+1);
    else if (i % 10 == 0)
      fprintf (stream," ");
    fprintf (stream,"%c",seq[i]);
  }
  fprintf (stream,"\n");
}

double calcMW (char *seq)
{
  /**
     Calculate molecular weight of a protein sequence.
     @param[in] seq - the sequence
     @return the molecular weight. Warnings can be retrieved via the log mdule
  */
  double aaWeights[26] = { 71.07940,114.59699,103.13940,115.08935,129.11644,
                          147.17818, 57.05231,137.14219,113.16067,  0.00000,
                          128.17534,113.16067,131.19358,114.10462,  0.00000,
                           97.11764,128.13171,156.18874, 87.07880,101.10589,
                            0.00000, 99.13358,186.21515,  0.00000,163.17758,
                          128.62408};
  int len;
  double weight;
  int i;
  int c;
  static Stringa msg = NULL;

  stringCreateOnce (msg,50);
  len = strlen (seq);
  weight = 18.01534;
  for (i=0;i<len;i++) {
    c = toupper (seq[i]);
    if (c < 'A' || c > 'Z') {
      stringPrintf (msg,"Invalid amino acid encountered: %c, skipped",c);
      warnAdd ("calcMW",string (msg));
    }
    weight += aaWeights[c-'A'];
  }
  return weight;
}

double getGEShydrophobicity (char aa) {
  /**
     Returns the hydrophobicity of an amino acid according to
     Goldman-Engelman-Steitz
     @param[in] aa - the amino acid in one letter code
     @return the hydrophobicity value
  */
  double gesHydrophobicity[26] = { 1.60,-8.70, 2.00,-9.20,-8.20,
                                   3.70, 1.00,-3.00, 3.10, 0.00,
                                  -8.80, 2.80, 3.40,-4.80, 0.00,
                                  -0.20,-4.10,-12.3, 0.60, 1.20,
                                   0.00, 2.60, 1.90,-1.35,-0.70,
                                  -4.45};
  char c;

  c = toupper (aa);
  if (c < 'A' || c > 'Z')
    return 0.0;
  return gesHydrophobicity[c-'A'];
}

static double kdHydrophobicity[26] = { 1.80,-3.50, 2.50,-3.50,-3.50,
                                       2.80,-0.40,-3.20, 4.50, 0.00,
                                      -3.90, 3.80, 1.90,-3.50, 0.00,
                                      -1.60,-3.50,-4.50,-0.80,-0.70,
                                       0.00, 4.20,-0.90,-0.27,-1.30,
                                      -3.50};

double getKDhydrophobicity (char aa) {
  /**
     Returns the hydrophobicity of an amino acid according to
     Kyte-Doolittle
     @param[in] aa - the amino acid in one letter code
     @return the hydrophobicity value
  */
  char c;

  c = toupper (aa);
  if (c < 'A' || c > 'Z')
    return 0.0;
  return kdHydrophobicity[c-'A'];
}

double calcAvgKDhydrophobicity (char *seq) {
  /**
     Calculates the average hydrophobicity of a protein sequence
     according to Kyte-Doolittle
     @param[in] seq - the protein sequence
     @return - the average hydrophobicity. Warnings can be retrieved via the
               log mdule
  */
  int len;
  double sum;
  int i,c,num;
  static Stringa msg = NULL;

  stringCreateOnce (msg,50);
  sum = 0.0;
  num = 0;
  len = strlen (seq);
  for (i=0;i<len;i++) {
    c = toupper (seq[i]);
    if (c < 'A' || c > 'Z') {
      stringPrintf (msg,"Invalid amino acid encountered: %c, skipped",c);
      warnAdd ("calcAvgKDhydrophobicity",string (msg));
    }
    else {
      sum += kdHydrophobicity[c-'A'];
      num++;
    }
  }
  return sum/num;
}

char translateCodon (char *cod) {
  /**
     Translates a codon consisting of normal or IUPAC bases into an amino acid
     @param[in] cod- the codon
     @return the amino acid in one letter code
  */
  char c1[] = "   ";
  int i;

  for (i=0;i<3;i++) {
    c1[i] = tolower (cod[i]);
    if (c1[i] == 'u')
      c1[i] = 't';
  }
  if (strStartsWithC (c1,"gc"))
    return 'A';
  if (strStartsWithC (c1,"tgt"))
    return 'C';
  if (strStartsWithC (c1,"tgc"))
    return 'C';
  if (strStartsWithC (c1,"tgy"))
    return 'C';
  if (strStartsWithC (c1,"gat"))
    return 'D';
  if (strStartsWithC (c1,"gac"))
    return 'D';
  if (strStartsWithC (c1,"gay"))
    return 'D';
  if (strStartsWithC (c1,"gaa"))
    return 'E';
  if (strStartsWithC (c1,"gag"))
    return 'E';
  if (strStartsWithC (c1,"gar"))
    return 'E';
  if (strStartsWithC (c1,"ttt"))
    return 'F';
  if (strStartsWithC (c1,"ttc"))
    return 'F';
  if (strStartsWithC (c1,"tty"))
    return 'F';
  if (strStartsWithC (c1,"gg"))
    return 'G';
  if (strStartsWithC (c1,"cat"))
    return 'H';
  if (strStartsWithC (c1,"cac"))
    return 'H';
  if (strStartsWithC (c1,"cay"))
    return 'H';
  if (strStartsWithC (c1,"att"))
    return 'I';
  if (strStartsWithC (c1,"atc"))
    return 'I';
  if (strStartsWithC (c1,"ata"))
    return 'I';
  if (strStartsWithC (c1,"aty"))
    return 'I';
  if (strStartsWithC (c1,"atw"))
    return 'I';
  if (strStartsWithC (c1,"atm"))
    return 'I';
  if (strStartsWithC (c1,"ath"))
    return 'I';
  if (strStartsWithC (c1,"aaa"))
    return 'K';
  if (strStartsWithC (c1,"aag"))
    return 'K';
  if (strStartsWithC (c1,"aar"))
    return 'K';
  if (strStartsWithC (c1,"ttg"))
    return 'L';
  if (strStartsWithC (c1,"tta"))
    return 'L';
  if (strStartsWithC (c1,"ttr"))
    return 'L';
  if (strStartsWithC (c1,"ct"))
    return 'L';
  if (strStartsWithC (c1,"ytr"))
    return 'L';
  if (strStartsWithC (c1,"atg"))
    return 'M';
  if (strStartsWithC (c1,"aat"))
    return 'N';
  if (strStartsWithC (c1,"aac"))
    return 'N';
  if (strStartsWithC (c1,"aay"))
    return 'N';
  if (strStartsWithC (c1,"cc"))
    return 'P';
  if (strStartsWithC (c1,"caa"))
    return 'Q';
  if (strStartsWithC (c1,"cag"))
    return 'Q';
  if (strStartsWithC (c1,"car"))
    return 'Q';
  if (strStartsWithC (c1,"cg"))
    return 'R';
  if (strStartsWithC (c1,"aga"))
    return 'R';
  if (strStartsWithC (c1,"agg"))
    return 'R';
  if (strStartsWithC (c1,"agr"))
    return 'R';
  if (strStartsWithC (c1,"mgr"))
    return 'R';
  if (strStartsWithC (c1,"tc"))
    return 'S';
  if (strStartsWithC (c1,"agt"))
    return 'S';
  if (strStartsWithC (c1,"agc"))
    return 'S';
  if (strStartsWithC (c1,"agy"))
    return 'S';
  if (strStartsWithC (c1,"ac"))
    return 'T';
  if (strStartsWithC (c1,"gt"))
    return 'V';
  if (strStartsWithC (c1,"tgg"))
    return 'W';
  if (strStartsWithC (c1,"tat"))
    return 'Y';
  if (strStartsWithC (c1,"tac"))
    return 'Y';
  if (strStartsWithC (c1,"tay"))
    return 'Y';
  if (strStartsWithC (c1,"taa"))
    return '*';
  if (strStartsWithC (c1,"tag"))
    return '*';
  if (strStartsWithC (c1,"tar"))
    return '*';
  if (strStartsWithC (c1,"tga"))
    return '*';
  return 'X';
}

char *translateCodonAmbig (char *cod) {
  /**
     Takes a codon of usually ambiguous nucleotides, translates
     it into all possible amino acids and returns the amino
     acids in a string.
     This function can be called if translateCodon() returns X
     @param[in] cod - the codon
     @return the valid amino acids as a string
  */
  char c1[] = "   ";
  char c2[] = "   ";
  Stringa aas;
  char aa;
  int i,l;
  char ambig[][5] = {{'a','a','\0','\0','\0'},
                     {'b','c','g', 't', '\0'},
                     {'c','c','\0','\0','\0'},
                     {'d','a','g', 't', '\0'},
                     {'g','g','\0','\0','\0'},
                     {'h','a','c', 't', '\0'},
                     {'k','g','t', '\0','\0'},
                     {'m','a','c', '\0','\0'},
                     {'n','a','c', 'g', 't'},
                     {'r','a','g', '\0','\0'},
                     {'s','c','g', '\0','\0'},
                     {'t','t','\0','\0','\0'},
                     {'v','a','c', 'g', '\0'},
                     {'w','a','t', '\0','\0'},
                     {'x','a','c', 'g', 't'},
                     {'y','c','t', '\0','\0'}};
  int l1[3],l2[3];

  for (i=0;i<3;i++) {
    c1[i] = tolower (cod[i]);
    if (c1[i] == 'u')
      c1[i] = 't';
  }
  for (i=0;i<3;i++) {
    for (l=0;l<NUMELE (ambig);l++) {
      if (c1[i] == ambig[l][0]) {
        l1[i] = l;
        break;
      }
    }
    if (l == NUMELE (ambig))
      l1[i] = 8; // 'n'
  }
  for (i=0;i<3;i++)
    l2[i] = 1;
  aas = stringCreate (10);
  for (;;) {
    for (i=0;i<3;i++)
      c2[i] = ambig[l1[i]][l2[i]];
    aa = translateCodon (c2);
    if (!strchr (string (aas),aa))
      stringCatChar (aas,aa);
    l2[2]++;
    if (l2[2] > 4 || ambig[l1[2]][l2[2]] == '\0') {
      l2[2] = 1;
      l2[1]++;
      if (l2[1] > 4 || ambig[l1[1]][l2[1]] == '\0') {
        l2[1] = 1;
        l2[0]++;
        if (l2[0] > 4 || ambig[l1[0]][l2[0]] == '\0')
          return string (aas);
      }
    }
  }
}

char *translateSeq (char *nuc) {
  /**
     Translates the sequence nuc into protein in frame 1.
     @param[in] nuc - the nucleotide sequence
     @return protein sequence.
             Memory for the protein sequence belongs to this function
  */
  static Array pro = NULL;
  int i,len;

  stringCreateClear (pro,100);
  len = strlen (nuc);
  for (i=0;i<len;i+=3)
    stringCatChar (pro,translateCodon (nuc+i));
  return string (pro);
}

char *threeLetterAA (char oneLetterAA) {
  /**
     Returns the three letter code of a one letter code amino acid.
     @param[in] oneLetterAA - the amino acid in one letter code
     @return three letter code
  */
  char aa1;

  aa1 = toupper (oneLetterAA);
  if (aa1 == 'A')
    return "Ala";
  if (aa1 == 'B')
    return "Asx";
  if (aa1 == 'C')
    return "Cys";
  if (aa1 == 'D')
    return "Asp";
  if (aa1 == 'E')
    return "Glu";
  if (aa1 == 'F')
    return "Phe";
  if (aa1 == 'G')
    return "Gly";
  if (aa1 == 'H')
    return "His";
  if (aa1 == 'I')
    return "Ile";
  if (aa1 == 'K')
    return "Lys";
  if (aa1 == 'L')
    return "Leu";
  if (aa1 == 'M')
    return "Met";
  if (aa1 == 'N')
    return "Asn";
  if (aa1 == 'P')
    return "Pro";
  if (aa1 == 'Q')
    return "Gln";
  if (aa1 == 'R')
    return "Arg";
  if (aa1 == 'S')
    return "Ser";
  if (aa1 == 'T')
    return "Thr";
  if (aa1 == 'V')
    return "Val";
  if (aa1 == 'W')
    return "Trp";
  if (aa1 == 'Y')
    return "Tyr";
  if (aa1 == 'Z')
    return "Glx";
  return "Unk";
}

char *GCGtimeStamp (void) {
  /**
     Returns the GCG formatted time stamp of the current time, e.g.
     February 6, 1998 15:59
     @return time stamp
  */
  time_t tp;
  struct tm *timeStru;
  static char tstring[50];

  time (&tp);
  timeStru = localtime (&tp);
  strftime (tstring,49,"%B %d, %Y  %H:%M",timeStru);
  return tstring;
}

int GCGcheckSum (char *seq) {
  /**
     Returns the GCG check sum of a sequence
     @param[in] seq - the sequence nuc or pro
     @return the check sum
  */
  int i,len;
  int check = 0;

  len = strlen (seq);
  for (i=0;i<len;i++)
    check += (((i) % 57) + 1) * toupper (seq[i]);
  return (check % 10000);
}

/// Bases that are recognized, including IUPAC
#define VALID_BASES "ACGNTUIXKMRYWSVHBD"

char getSeqType (char *seq) {
  /**
     Try to figure out sequence type from sequence.
     Works similar to GCG IsProtein but checks the whole sequence,
     not only the first 300 symbols.
     No space chars, digits, newlines etc. allowed
     @param[in] seq - the sequence
     @return 'N' for nuc, 'P' for protein, ' ' is sequence is empty
  */
  int i,len;

  len = strlen (seq);
  if (len == 0)
    return ' ';
  for (i=0;i<len;i++) {
    if (isGapChar(seq[i]))
      continue;
    if (seq[i] == '*')
      continue;
    if (!strchr (VALID_BASES,toupper (seq[i])))
      return 'P';
  }
  return 'N';
}

char *fileName2userseqName (char *nonDBname) {
  /**
     From a path/fileName.ext transform into userseq:fileName;
     e.g. /hlr/tmp/mpa0s5.seq generates userseq:mpa0s5
     @param[in] nonDBname - the name of a sequence that does not come from a
                            database
     @return the name in format userseq:...; memory belongs to this function
             and is stable until the next call
  */
  char *pos,*pos1;
  static Stringa id = NULL;

  stringCreateOnce (id,20);
  pos = strrchr (nonDBname,'/');
  if (pos)
    pos++;
  else
    pos = nonDBname;
  pos1 = strchr (pos,'.');
  if (pos1)
    *pos1 = '\0';
  stringPrintf (id,"userseq:%s",pos);
  if (pos1) // restore original name
    *pos1 = '.';
  return string (id);
}

Texta hlr_ecExtract (char *de_line) {
  /**
     This function reads a '\0'-terminated input line and scans it for EC
     numbers and returns them in a Texta;<br>
     EC numbers in [includes:]-fields are ignored.<br>
     Note: Memory is managed by this function and should only be read-accessed
     by the caller; memory is stable until next call to this function<br>
     @param[in] de_line - line to scan (usually a DE-line from database entries)
     @return all EC numbers found or NULL if nothing found
     @param[out] de_line - contents destroyed
  */
  /*
    Input examples:
    DE   PUTATIVE NAD(P)H NITROREDUCTASE YDGI (EC 1.-.-.-).
    DE   CYTOCHROME BD-II OXIDASE SUBUNIT II (EC 1.10.3.-) (EC 1.10.4.-).
    DE   the ADSL gene for Adenylosuccinate lyase (EC 4.3.2.2, Adenylosuccinase)
    DE   X55995 Rat mRNA for dimethylglycine dehydrogenase (EC numbers 1.5.99.2, 1.5.99.2).
    from swissprot sw:p22985:
    DE   (EC 1.1.1.204) (XD); Xanthine oxidase (EC 1.1.3.22) (XO) (Xanthine oxidoreductase)].
  */
  char *pos;
  char *word;
  static Texta ec_numbers=NULL;
  WordIter iterword;

  textCreateClear (ec_numbers,4);
  tolowerStr (de_line); // make function case insensitive
  pos=strstr (de_line,"(ec"); // check for key sequence "(ec"
  if (!pos)
    return NULL; // return directly if not found
  pos += 4;
  iterword = wordIterCreate (pos," ()\t,",1);
  while ((word = wordNext (iterword)) != NULL) { // examine all 'words' in the input line
    if (strNEqual (word,"[incl",5)) // skip rest of line when pattern '[includes: ec...' is found
      break;
    if (strTranslate (word,".",".") == 3) // EC number found
      textAdd (ec_numbers,word); // store EC number
  }
  if (arrayMax (ec_numbers))
    return ec_numbers;
  return NULL;
}

static char mat_aa[] =
  {'A','R','N','D','C','Q','E','G','H','I','L','K','M','F','P','S','T','W','Y','V','B','Z','X','*'};

static int blo62[NUMELE (mat_aa)][NUMELE (mat_aa)] =
  {{ 4,-1,-2,-2, 0,-1,-1, 0,-2,-1,-1,-1,-1,-2,-1, 1, 0,-3,-2, 0,-2,-1, 0,-4},
   {-1, 5, 0,-2,-3, 1, 0,-2, 0,-3,-2, 2,-1,-3,-2,-1,-1,-3,-2,-3,-1, 0,-1,-4},
   {-2, 0, 6, 1,-3, 0, 0, 0, 1,-3,-3, 0,-2,-3,-2, 1, 0,-4,-2,-3, 3, 0,-1,-4},
   {-2,-2, 1, 6,-3, 0, 2,-1,-1,-3,-4,-1,-3,-3,-1, 0,-1,-4,-3,-3, 4, 1,-1,-4},
   { 0,-3,-3,-3, 9,-3,-4,-3,-3,-1,-1,-3,-1,-2,-3,-1,-1,-2,-2,-1,-3,-3,-2,-4},
   {-1, 1, 0, 0,-3, 5, 2,-2, 0,-3,-2, 1, 0,-3,-1, 0,-1,-2,-1,-2, 0, 3,-1,-4},
   {-1, 0, 0, 2,-4, 2, 5,-2, 0,-3,-3, 1,-2,-3,-1, 0,-1,-3,-2,-2, 1, 4,-1,-4},
   { 0,-2, 0,-1,-3,-2,-2, 6,-2,-4,-4,-2,-3,-3,-2, 0,-2,-2,-3,-3,-1,-2,-1,-4},
   {-2, 0, 1,-1,-3, 0, 0,-2, 8,-3,-3,-1,-2,-1,-2,-1,-2,-2, 2,-3, 0, 0,-1,-4},
   {-1,-3,-3,-3,-1,-3,-3,-4,-3, 4, 2,-3, 1, 0,-3,-2,-1,-3,-1, 3,-3,-3,-1,-4},
   {-1,-2,-3,-4,-1,-2,-3,-4,-3, 2, 4,-2, 2, 0,-3,-2,-1,-2,-1, 1,-4,-3,-1,-4},
   {-1, 2, 0,-1,-3, 1, 1,-2,-1,-3,-2, 5,-1,-3,-1, 0,-1,-3,-2,-2, 0, 1,-1,-4},
   {-1,-1,-2,-3,-1, 0,-2,-3,-2, 1, 2,-1, 5, 0,-2,-1,-1,-1,-1, 1,-3,-1,-1,-4},
   {-2,-3,-3,-3,-2,-3,-3,-3,-1, 0, 0,-3, 0, 6,-4,-2,-2, 1, 3,-1,-3,-3,-1,-4},
   {-1,-2,-2,-1,-3,-1,-1,-2,-2,-3,-3,-1,-2,-4, 7,-1,-1,-4,-3,-2,-2,-1,-2,-4},
   { 1,-1, 1, 0,-1, 0, 0, 0,-1,-2,-2, 0,-1,-2,-1, 4, 1,-3,-2,-2, 0, 0, 0,-4},
   { 0,-1, 0,-1,-1,-1,-1,-2,-2,-1,-1,-1,-1,-2,-1, 1, 5,-2,-2, 0,-1,-1, 0,-4},
   {-3,-3,-4,-4,-2,-2,-3,-2,-2,-3,-2,-3,-1, 1,-4,-3,-2,11, 2,-3,-4,-3,-2,-4},
   {-2,-2,-2,-3,-2,-1,-2,-3, 2,-1,-1,-2,-1, 3,-3,-2,-2, 2, 7,-1,-3,-2,-1,-4},
   { 0,-3,-3,-3,-1,-2,-2,-3,-3, 3, 1,-2, 1,-1,-2,-2, 0,-3,-1, 4,-3,-2,-1,-4},
   {-2,-1, 3, 4,-3, 0, 1,-1, 0,-3,-4, 0,-3,-3,-2, 0,-1,-4,-3,-3, 4, 1,-1,-4},
   {-1, 0, 0, 1,-3, 3, 4,-2, 0,-3,-3, 1,-1,-3,-1, 0,-1,-3,-2,-2, 1, 4,-1,-4},
   { 0,-1,-1,-1,-2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-2, 0, 0,-2,-1,-1,-1,-1,-1,-4},
   {-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4, 1}};

int blosum62 (char aa1,char aa2) {
  /**
     Returns the score in the matrix for the amino acids given as arguments.
     @param[in] aa1 - the one-letter code of the first amino acid
     @param[in] aa2 - the one-letter code of the second amino acid
     @return the matrix score or 0 if not found
  */
  int i,j,k;

  i = j = NUMELE (mat_aa);
  for (k=0;k<NUMELE (mat_aa);k++){
    if (mat_aa[k]==toupper(aa1))
      j=k;
    if (mat_aa[k]==toupper(aa2))
      i=k;
  }
  if (i<NUMELE (mat_aa) && j<NUMELE (mat_aa))
    return blo62[i][j];
  warn ("blosum62: did not find a score for %c or %c",aa1,aa2);
  return 0;
}

int isSwissprotID (char *n) {
  /**
     Determines whether a sequence id is a Swissprot id (not a TrEMBL id
     @param[in] n - the id
     @return 1 if Swissprot id, 0 otherwise
  */
  static char *n1 = NULL;
  char *pos;

  pos = strchr (n,':');
  if (pos)
    strReplace (&n1,pos+1);
  else
    strReplace (&n1,n);
  pos = strchr (n1,'_');
  if (!pos)
    die ("isSwissprotID: _ expected in %s",n1);
  *pos = '\0';
  if (strlen (n1) < 6)
    return 1;
  return 0;
}

/* ------------- mini module rangeAc --------------------- */
/*
  Given a range of accession numbers, can enumerate each one in the range
*/
static int gRangeAcBeg,gRangeAcEnd,gRangeAcCurrent;
static int gRangeAcNumDigits;
static char *gRangeAcPrefix;

int rangeAc_set (char *rangeAc) {
  /**
     Set the range, e.g. AB101575-AB101581 or AB101575
     @param[in] rangeAc - the range
     @return 0 if there was a problem (use warnReport () to get the error
             message), 1 if successful
  */
  char *pos;
  char *ac;
  int i;
  static Stringa msg = NULL;

  stringCreateOnce (msg,100);
  ac = hlr_strdup (rangeAc);
  pos = strchr (ac,'-');
  if (pos)
    *pos = '\0';
  i=0;
  while (isalpha (ac[i]))
    i++;
  gRangeAcNumDigits = strlen (ac+i);
  sscanf (ac+i,"%d",&gRangeAcBeg);
  *(ac+i) = '\0';
  gRangeAcPrefix = hlr_strdup (ac);
  if (!pos)
    gRangeAcEnd = gRangeAcBeg;
  else {
    i=0;
    while (isalpha (pos[1+i]))
      i++;
    if (strlen (pos+1+i) != gRangeAcNumDigits) {
      stringPrintf (msg,"both ACs do not have the same number of digits: %s",
                    rangeAc);
      warnAdd ("rangeAc_set",string (msg));
      return 0;
    }
    sscanf (pos+1+i,"%d",&gRangeAcEnd);
    *(pos+1+i) = '\0';
    if (!strEqual (gRangeAcPrefix,pos+1)) {
      stringPrintf (msg,"both ACs do not have the same prefix: %s",
                    rangeAc);
      warnAdd ("rangeAc_set",string (msg));
      return 0;
    }
  }
  hlr_free (ac);
  gRangeAcCurrent = gRangeAcBeg;
  if (gRangeAcBeg > gRangeAcEnd) {
    stringPrintf (msg,"the numbers in the range are not increasing: %s",
                  rangeAc);
    warnAdd ("rangeAc_set",string (msg));
      return 0;
  }
  return 1;
}

char *rangeAc_next (void) {
  /**
     On each call returns the next accession number or NULL if none is left
     @return a valid accession number or NULL if none left
  */
  static Stringa ac = NULL;

  if (gRangeAcCurrent > gRangeAcEnd)
    return NULL;
  stringCreateOnce (ac,10);
  stringPrintf (ac,"%s%0*d",gRangeAcPrefix,gRangeAcNumDigits,gRangeAcCurrent);
  gRangeAcCurrent++;
  return string (ac);
}
