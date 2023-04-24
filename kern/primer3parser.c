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
/** @file primer3parser.c
    @brief Parses output of the primer3 program.
    Module prefix pr3p_
*/
#include <ctype.h>
#include "log.h"
#include "format.h"
#include "primer3parser.h"

/* --------------- begin module primeparser --------------------

implementation idea:
  first the user of this module registers functions that should
  be called when interesting points in the prime output file are found.
  Then the user starts the parser, which in turn drives the functions
  the user registered before, feeding these functions with
  the current data (e.g. query name, subject name, ...).

public functions:
   void pr3p_init()
   void pr3p_run(linestream)
   void pr3p_register_<name>(function); (returns 1 parsing should continue,
                                         else 0 to stop parser)
     where <name> is:
------------------------------------------------------------
name          when called  /
              what delivered
------------------------------------------------------------
product       after a new product paragraph was read
forward       after product is available, information about forward primer
reverse       after product is available, information about reverse primer
------------------------------------------------------------

*/
static int (*sequence_hook) (char *seqname);
static int (*product_hook) (int num,int length);
static int (*forward_hook) (int beg,int end,char *seq,float gc,float tm);
static int (*reverse_hook) (int beg,int end,char *seq,float gc,float tm);
static int (*internal_hook) (int beg,int end,char *seq,float gc,float tm);

void pr3p_register_sequence (int (*f)(char *seqname)) {
  /**
     Registers a function to be called when the sequence name is found
     @param[in] f - the function
  */
  sequence_hook = f;
}

void pr3p_register_product (int (*f)(int num,int length)) {
  /**
     Registers a function to be called when a product is found
     @param[in] f - the function
  */
  product_hook = f;
}

void pr3p_register_forward (int (*f)(int beg,int end,char *seq,
                                     float gc,float tm)) {
  /**
     Registers a function to be called when a forward primer is found
     @param[in] f - the function
  */
  forward_hook = f;
}

void pr3p_register_reverse (int (*f)(int beg,int end,char *seq,
                                     float gc,float tm)) {
  /**
     Registers a function to be called when a reverse primer is found
     @param[in] f - the function
  */
  reverse_hook = f;
}

void pr3p_register_internal (int (*f)(int beg,int end,char *seq,
                                      float gc,float tm)) {
  /**
     Registers a function to be called when an internal oligo is found
     @param[in] f - the function
  */
  internal_hook = f;
}

void pr3p_init (void) {
  /**
     Initializes the primer3 parser
  */
  sequence_hook = NULL;
  product_hook = NULL;
  forward_hook = NULL;
  reverse_hook = NULL;
}

void pr3p_run (LineStream ls) {
  /**
     Parse the primer3 output and call the registered functions.<br>
     Postcondition: the functions registered have been called
     @param[in] ls - input line stream with prime output
  */
  int goOn = 1;
  char *line;

  while ((line = ls_nextLine (ls)) != NULL) {
    if (strStartsWithC (line,"# EPRIMER3 RESULTS FOR ")) {
      char name[100];

      sscanf (line+23,"%99s",name);
      if (goOn && sequence_hook != NULL)
        goOn = (*sequence_hook) (name);
    }
    else if (strStartsWithC (line,"# CLONINGPRIMERS RESULTS FOR ")) {
      char name[100];

      sscanf (line+29,"%99s",name);
      if (goOn && sequence_hook != NULL)
        goOn = (*sequence_hook) (name);
    }
    else if (line[0] == '#' || line[0] == '\0')
      continue;
    else if (strstr (line,"PRODUCT SIZE:")) {
      int num,len;

      if (sscanf (line,"%d %*s %*s %d",&num,&len) != 2)
        die ("primer3parser: format error on line %s",line);
      if (goOn && product_hook != NULL)
        goOn = (*product_hook) (num,len);
    }
    else if (strstr (line,"FORWARD PRIMER") || strstr (line,"REVERSE PRIMER") ||
             strstr (line,"INTERNAL OLIGO")) {
      int start,len;
      float tm,gc;
      char seq[101];

      if (sscanf (line,"%*s %*s %d %d %f %f %100s",
                  &start,&len,&tm,&gc,seq) != 5)
        die ("primer3parser: format error on line %s",line);
      if (strstr (line,"FORWARD PRIMER")) {
        if (goOn && forward_hook != NULL)
          goOn = (*forward_hook) (start,start+len-1,seq,gc,tm);
      }
      else if (strstr (line,"REVERSE PRIMER")) {
        if (goOn && reverse_hook != NULL)
          goOn = (*reverse_hook) (start,start+len-1,seq,gc,tm);
      }
      else if (strstr (line,"INTERNAL OLIGO")) {
        if (goOn && internal_hook != NULL)
          goOn = (*internal_hook) (start,start+len-1,seq,gc,tm);
      }
    }
  }
}
