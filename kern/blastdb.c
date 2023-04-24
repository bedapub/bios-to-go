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
/** @file blastdb.c
    @brief Reading sequences from blast databases.
    Module prefix bdb_
*/
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "log.h"
#include "format.h"
#include "blastdb.h"

static unsigned int *hPtr,*sPtr,*aPtr;
static unsigned int numSeq;
static int seqNum = 0;
static int sfh;
static int hfh;
static int gDbType;
static char base[] = "acgt";
static char ambigbase[] = "xxxmxrsvxwyhkdbn";
static char aa[] = "@ABCDEFGHIKLMNPQRSTVWXYZU";

static void uint4_read (int fh,unsigned int *valp) {
  int readRes;

#ifdef IS_BIG_ENDIAN
  readRes = read (fh,4,valp);
#else
  unsigned char b[4];

  readRes = read (fh,b,4);
  *valp = ((int)b[0]<<24) + ((int)b[1]<<16) + ((int)b[2]<<8) + b[3];
#endif

  if (readRes < 0) {
    die("unit4_read failed - please report the bug");
  }
}

static void long8_read (int fh,long *val) {
  int readRes;

#ifdef IS_BIG_ENDIAN
  readRes = read (fh,val,8);
#else
  unsigned char b[8];

  readRes = read (fh,b,8);
  *val = ((long)b[3]<<24) + ((long)b[2]<<16) + ((long)b[1]<<8) + ((long)b[0]);
#endif

  if (readRes < 0) {
    die("unit4_read failed - please report the bug");
  }
}

static void bdb_close (void) {
  close (sfh);
  close (hfh);
}

int bdb_read_next (char **name,char **seq) {
  /**
     Returns the next sequence
     @param[in] name - pointer to a \0 termnated string, NULL if not interested
     @param[in] seq - pointer to a \0 termnated string, NULL if not interested
     @param[out] name - name of the sequence
     @param[out] seq - thesequence itself
     @return 1: valid sequence, 0: no more sequence
  */
  int len;
  static char *n = NULL;
  static char *s0 = NULL;
  static char *s = NULL;
  int i;
  int readRes;

  if (seqNum == numSeq) {
    bdb_close ();
    return 0;
  }
  lseek (hfh,hPtr[seqNum],SEEK_SET);
  len = hPtr[seqNum+1]-hPtr[seqNum]-1;
  hlr_free (n);
  n = (char *)hlr_calloc (len+1,sizeof (char));
  readRes = read (hfh,n,len);
  if (readRes < 0) {
    die("error in bdb_read_next - please report the bug");
  }
  if (name != NULL)
    *name = n+8;
  if (gDbType == DB_TYPE_NUC) {
    int i1,k;
    unsigned int numAmbig;
    char ambig[4];
    unsigned int pos;
    int repeat;
    char b;
    unsigned char pad;

    lseek (sfh,sPtr[seqNum],SEEK_SET);
    len = aPtr[seqNum]-sPtr[seqNum];
    hlr_free (s0);
    s0 = (char *)hlr_calloc (len,sizeof (char));
    readRes = read (sfh,s0,len);
    if (readRes < 0) {
      die("error in bdb_read_next - please report the bug");
    }
    pad = (s0[len-1] & 3);
    hlr_free (s);
    s = (char *)hlr_calloc (4*len,sizeof (char));
    i1 = 0;
    for (i=0;i<len;i++) {
      s[i1++] = base[(s0[i] & 192) >> 6];
      s[i1++] = base[(s0[i] & 48) >> 4];
      s[i1++] = base[(s0[i] & 12) >> 2];
      s[i1++] = base[s0[i] & 3];
    }
    s[i1 - (4-pad)] = '\0';
    if (aPtr[seqNum] < sPtr[seqNum +1]) {
      lseek (sfh,aPtr[seqNum],SEEK_SET);
      uint4_read (sfh,&numAmbig);
      for (i=0;i<numAmbig;i++) {
        lseek (sfh,aPtr[seqNum] + (i+1)*4,SEEK_SET);
        readRes = read (sfh,ambig,4);
	if (readRes < 0) {
	  die("error in pdb_read_next (numAmbig) - please report the bug");
	}
        pos = ((unsigned char)ambig[1]<<16) + ((unsigned char)ambig[2]<<8) + (unsigned char)ambig[3];
        b = ambigbase[((unsigned int)ambig[0] & 240) >> 4];
        repeat = (int)ambig[0] & 15;
        for (k=0;k<=repeat;k++)
          s[pos+k] = b;
      }
    }
  }
  else { // protein
    lseek (sfh,sPtr[seqNum],SEEK_SET);
    len = sPtr[seqNum+1]-sPtr[seqNum]-1; /* do not read last \0 */
    hlr_free (s);
    s = (char *)hlr_calloc (len+1,sizeof (char));
    readRes = read (sfh,s,len);
    for (i=0;i<len;i++)
      s[i] = aa[(int)s[i]];
  }
  if (seq != NULL)
    *seq = s;
  seqNum++;
  return 1;
}

void bdb_open (char *dbname,int dbtype) {
  /**
     Opens the database for reading
     @param[in] dbname- full path of the database excluding extension
     @param[in] dbtype - DB_TYPE_NUC or DB_TYPE_PRO
  */
  Stringa fn;
  int fh;
  unsigned int version;
  unsigned int protein;
  unsigned int len;
  char *title;
  char *date;
  long numRes;
  unsigned int maxLenSeq;
  int i;
  int readRes;

  gDbType = dbtype;
  fn = stringCreate (20);
  if (dbtype == DB_TYPE_NUC)
    stringPrintf (fn,"%s.nin",dbname);
  else
    stringPrintf (fn,"%s.pin",dbname);
  fh = open (string (fn),O_RDONLY);
  if (fh < 0)
    die ("bdb_open: could not open database %s",string (fn));
  uint4_read (fh,&version);
  uint4_read (fh,&protein);
  uint4_read (fh,&len);
  title = (char *)calloc (len+1,sizeof (char));
  readRes = read (fh,title,len);
  uint4_read (fh,&len);
  date = (char *)calloc (len+1,sizeof (char));
  readRes = read (fh,date,len);
  uint4_read (fh,&numSeq);
  long8_read (fh,&numRes);
  uint4_read (fh,&maxLenSeq);
  hPtr = (unsigned int *)calloc (numSeq+1,sizeof (unsigned int));
  for (i=0;i<numSeq+1;i++)
    uint4_read (fh,hPtr+i);
  sPtr = (unsigned int *)calloc (numSeq+1,sizeof (unsigned int));
  for (i=0;i<numSeq+1;i++)
    uint4_read (fh,sPtr+i);
  if (dbtype == DB_TYPE_NUC) {
    aPtr = (unsigned int *)calloc (numSeq+1,sizeof (unsigned int));
    for (i=0;i<numSeq+1;i++)
      uint4_read (fh,aPtr+i);
  }
  close (fh);
  /*
  printf ("version=%d\n",version);
  printf ("protein=%d\n",protein);
  printf ("title=%s\n",title);
  printf ("date=%s\n",date);
  printf ("numSeq=%d\n",numSeq);
  printf ("numRes=%ld\n",numRes);
  printf ("maxLenSeq=%d\n",maxLenSeq);
  for (i=0;i<numSeq+1;i++)
    printf ("hPtr[%d]=%d\n",i,hPtr[i]);
  for (i=0;i<numSeq+1;i++)
    printf ("sPtr[%d]=%d\n",i,sPtr[i]);
  if (dbtype == DB_TYPE_NUC) {
    for (i=0;i<numSeq+1;i++)
      printf ("aPtr[%d]=%d\n",i,aPtr[i]);
  }
  */
  if (dbtype == DB_TYPE_NUC)
    stringPrintf (fn,"%s.nhr",dbname);
  else
    stringPrintf (fn,"%s.phr",dbname);
  hfh = open (string (fn),O_RDONLY);
  if (dbtype == DB_TYPE_NUC)
    stringPrintf (fn,"%s.nsq",dbname);
  else
    stringPrintf (fn,"%s.psq",dbname);
  sfh = open (string (fn),O_RDONLY);
  seqNum = 0;
}

/*
int main (int argc,char *argv[])
{
  char *name,*seq;
  int sl,i;

  bdb_open ("/DATA/bi/apps/blast/humann",DB_TYPE_NUC);
  while (bdb_read_next (&name,&seq)) {
    printf (">%s",name);
    sl = strlen (seq);
    for (i=0;i<sl;i++) {
      if (i % 60 == 0)
        puts ("");
      printf ("%c",seq[i]);
    }
    puts ("");
  }
  return 0;
}
*/
