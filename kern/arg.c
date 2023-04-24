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
/** @file arg.c
    @brief Module to parse command line arguments.
    Module prefix arg_
*/

/*  ----- original design documentation

Let us first define four terms:
- argument
- optional argument
- required argument
- program parameter

When a command line program starts, it can draw
'program parameters' from various sources, e.g.
from '.files' in the user's home directory,
from environment variables and last no least
from the command line. We call the program parameters
given on the command line 'arguments'.
Some of these arguments may be 'optional' some
may be 'required'.

this module handles command lines obeying
the following structure:

  prog [-optname1] [-optname2 value] arg1 arg2

consisting of three parts:
program name, zero or more optional arguments,
one or more required arguments in this order.
The required arguments can be named, in which case they
are syntactially identical to optional arguments and enjoy
the same mobility. So the command line syntax from above
should be written as

  prog [-optname1] [-optname2 value] [-argname1 ]arg1 [-argname2 ]arg2

(the blank between argname1 and the ']' denotes that this is
not an option, but the name of the follwing required argument.
Arguments (optional or required) with one parameter can be of the
form -arg=param or -arg param

some additional assumption are made:

- the optional arguments '-h' and '-help' are not available to
  user programs
- calling
  progname
  -- without argument gives help
- calling
  progname -h
  -- gives more help
- calling
  progname -help
  -- prints a manual
- calling the program with a combination of unnamed arguments while
  specifying the same option mulitple times does usually not work.
  When specificing an option a the end of the command line,
  all arguments must be named.
- each options stands by itself, option clustering like e.g.
  in many UNIX commands is not supported
- we do not try to build a GCG-style system that automatically
  queries the user for missing required arguments, depending on
  the presence of the optional argument '-default'; what we do
  here is for non-interactive programs.

Example 1)

the command line

  clustern [-minsim f] [-nlist] filename

would use the code
  if (arg_init(argc,argv,"minsim,1 nlist","in",usagef) != argc)
    usage ("too many arguments; invoke program without params for help");
  if (arg_present ("nlist")) ...;
  fopen (arg_get ("in"));
  if (cp = arg_getPos ("minsim",1)) ...;

the user could enter:
  clustern
  clustern -h
  clustern -help
  clustern myfile
  clustern -in myfile
  clustern -in=myfile
  clustern -nlist myfile
  clustern -in myfile -nlist
  clustern -nlist -minsim 10 x
  clustern -nlist -minsim 10 -
  clustern -in=-
  clustern "file name with blanks in it"
  clustern -in="file name with blanks in it"

illegal usage would be:
  clustern -x          # unknown option
  clustern x -nlist    # no named arg after positional arg

Example 2)

the command line

  dbworth2embl [-sortft] filename...

would use the code

  int c = arg_init (argc,argv,"sortft",NULL,usagef);
  int i;
  for (i=c;i<argc;i++)
    printf ("filename=%s\n",argv[i]);

which accepts e.g.
  dbworth2embl -sortft f1 f2
  dbworth2embl - # use stdin/stdout

Example 3)

the command line
  prog3 [-debug] add name value..
  prog3 [-debug] [-user user] del name
is for a program that can be operated in two modes:
'add' to associate a name with a list of values,
'del' to delete the name from the database.

It would use the code:

  char *mode;
  if (!arg_initTry (argc,argv,"debug user,1","mode",usagef))
    usagef (1);
  mode = hlr_strdup (arg_get ("mode"));
  printf ("mode=%s debug=%d.\n",mode,arg_present ("debug"));
  if (strEqual (mode,"add")) {
    int i;
    int cnt;
    if ((cnt = arg_init (argc,argv,"debug","mode name",usagef)) == argc)
      die ("no value(s) supplied.");
    printf ("add name='%s' values",arg_get ("name"));
    for (i=cnt;i<argc;i++)
      printf (" '%s'",argv[i]);
    printf ("\n");
  }
  else if (strEqual (mode,"del")) {
    if (arg_init (argc,argv,"debug user,1","mode name",usagef) != argc)
      usagef (1);
    printf ("user=%s name='%s'\n",s0f (arg_get ("user")),arg_get ("name"));
  }
  else
    die ("unrecognized mode %s.",mode);

Runtime behaviour would be as follows:

argtest3 -debug add name1 v1 v2
  --> OK
argtest3 -mode=add -name=name1 v1 v2
  --> OK
argtest3 -mode add -name name1 v1 v2
  --> OK
argtest3 -mode=add -name='name 1' v1 v2
  --> OK (value with embedded blank)
argtest3 -debug add name1
  --> no value(s) supplied
argtest3 -user=a del name1 -user=b
  --> program called with incorrect arguments.
  (not possible to have named arguments after unnamed ones)
argtest3 -user=a -mode=del -name=name1 -user=b
  --> OK, user=b name='name1'


---------------------------------------

design goals:
- it should be possible to convert optional parameter into
  a required one and vice versa without changing the parts
  of the programs that access the value too much
- it should be possible to enter the command line as short
  as possible, relying on a fixed order of arguments
  (good for interactive use)
- it should be possible to enter the command line with all arguments
  explicitly named (good for batch scripts)
- it should not be necessary to remember the order of optional
  arguments; such arguments can come in any order
- it should be possible to have a trailing open-ended list of
  arguments
- it should be possible to test for a command line to adhere
  to a specific format, then let the program decide depending
  on the outcome of the test
- if optional arguments occur multiple times, the last
  occurence should count
*/

#include "log.h"
#include "format.h"
#include "hlrmisc.h"
#include "arg.h"

/// structure to hold one argument
typedef struct {
  char *name; //!< name of the argument
  Texta params; //!< values of the argument
}Arg;

static Array gArgs = NULL; // of Arg
static char *gArg0 = NULL;
static Texta gArgList = NULL;

static int testAndInit (int argc, char *argv[],
                        char *optargs, char *reqargs,
                        void (*usagef)(int level),int testOnly) {
  /**
     see arg_init and arg_initTry for comments.
     This function runs in normal or test mode: in both modes,
     wrong argument syntax leads to die(). In normal mode, if the
     arguments are not as expected, die() is called. In test mode,
     0 is returned if the arguments are not as expected, otherwise
     the number of parsed arguments is returned
  */
  int a;
  static char *tOptaSav = NULL;
  static char *tReqaSav = NULL;
  static char **tArgv = NULL;
  static char *reqa = NULL;
  static char *opta = NULL;
  char *o1,*r1;
  char *pos,*equ;
  int no; // number of parameters of argument
  Arg *currArg;
  WordIter wi;

  if (tArgv == NULL)
    tArgv = (char **)hlr_calloc (argc,sizeof (char *));
  for (a=0;a<argc;a++) {
    hlr_free (tArgv[a]);
    tArgv[a] = hlr_strdup (argv[a]);
  }
  gArg0 = argv[0];
  strReplace (&reqa,reqargs);
  strReplace (&opta,optargs);
  if (argc < 2) {
    usagef (1);
    exit (1);
  }
  else if (strCaseEqual (tArgv[1],"-h")) {
    usagef (2);
    exit (1);
  }
  else if (strCaseEqual (tArgv[1],"-help")) {
    usagef (3);
    exit (1);
  }
  if (gArgs == NULL)
    gArgs = arrayCreate (5,Arg);
  else {
    int i;

    for (i=0;i<arrayMax (gArgs);i++) {
      currArg = arrp (gArgs,i,Arg);
      arrayDestroy (currArg->params); // contained only references to existing memory
    }
    arrayClear (gArgs);
  }
  // construct list of arguments possible, for use by arg_present() and arg_getPos()
  textDestroy (gArgList);
  gArgList = textCreate (10);
  if (opta != NULL) {
    strReplace (&tOptaSav,opta);
    wi = wordIterCreate (tOptaSav," ",1);
    while ((o1 = wordNext (wi)) != NULL) {
      pos = strchr (o1,',');
      if (pos != NULL)
        *pos = '\0';
      textAdd (gArgList,o1);
    }
    wordIterDestroy (wi);
  }
  if (reqa != NULL) {
    strReplace (&tReqaSav,reqa); // required arguments
    wi = wordIterCreate (tReqaSav," ",1);
    while ((r1 = wordNext (wi)) != NULL)
      textAdd (gArgList,r1);
    wordIterDestroy (wi);
  }
  arraySort (gArgList,(ARRAYORDERF)arrayStrcmp);
  a = 1;
  while (a < argc) {
    if (tArgv[a][0] != '-')
      break;
    equ = strchr (tArgv[a],'=');
    if (equ != NULL)
      *equ = '\0';
    // optional arguments
    o1 = NULL;
    if (opta != NULL) {
      strReplace (&tOptaSav,opta);
      wi = wordIterCreate (tOptaSav," ",1);
      while ((o1 = wordNext (wi)) != NULL) {
        pos = strchr (o1,',');
        if (pos != NULL)
          *pos = '\0';
        if (strCaseEqual (tArgv[a]+1,o1))
          break;
      }
      wordIterDestroy (wi);
    }
    if (o1 != NULL) { // optional argument found
      currArg = arrayp (gArgs,arrayMax (gArgs),Arg);
      currArg->params = textCreate (2);
      no = 0;
      if (pos != NULL)
        sscanf (pos+1,"%d",&no);
      if (equ != NULL && no != 1)
        die ("%s: '=' only allowed if optional argument has exactly one parameter",
             tArgv[a]);
      if (no == 0) {
        currArg->name = tArgv[a]+1;
        a++;
      }
      else if (no == 1 && equ != NULL) { // one parameter in the form -arg=param
        currArg->name = tArgv[a]+1;
        array (currArg->params,arrayMax (currArg->params),char *) = equ+1;
        a++;
      }
      else { // one parameter in the form -arg param or more than one parameter
        int i;

        currArg->name = tArgv[a]+1;
        for (i=0;i<no;i++) {
          if (argc <= a+i+1 || (tArgv[a+i+1][0] == '-' &&
                                tArgv[a+i+1][1] != '\0'))
            die ("Not enough parameters for optional argument %s.\nType '%s -h' for help",
                 tArgv[a]+1,tArgv[0]);
          array (currArg->params,arrayMax (currArg->params),char *) = tArgv[a+i+1];
        }
        a += (no + 1);
      }
    }
    else { // is it a named required argument ?
      r1 = NULL;
      if (reqa != NULL) {
        strReplace (&tReqaSav,reqa); // required arguments
        wi = wordIterCreate (tReqaSav," ",1);
        while ((r1 = wordNext (wi)) != NULL) {
          if (strCaseEqual (tArgv[a]+1,r1))
            break;
        }
        wordIterDestroy (wi);
      }
      if (r1 != NULL) {
        pos = strstr (reqa,tArgv[a]+1); // replace argument by blanks in reqa
        for (;*pos && *pos != ' ';pos++)
          *pos = ' ';
        currArg = arrayp (gArgs,arrayMax (gArgs),Arg);
        currArg->params = textCreate (1);
        currArg->name = tArgv[a]+1;
        if (equ != NULL) {
          array (currArg->params,arrayMax (currArg->params),char *) = equ+1;
          a++;
        }
        else {
          if (argc <= a+1 || (tArgv[a+1][0] == '-' && tArgv[a+1][1] != '\0'))
            die ("Not enough parameters for named required argument %s.\nType '%s -h' for help",
                 tArgv[a]+1,tArgv[0]);
          array (currArg->params,arrayMax (currArg->params),char *) = tArgv[a+1];
          a += 2;
        }
      }
      else {
        if (tArgv[a][0] == '-' && strlen (tArgv[a]) > 1) {
          /*
            if (testOnly)
              return 0;
            else
             die ("Argument %s not recognized.\nType '%s -h' for help",tArgv[a],tArgv[0]);
          */
        }
        break;
      }
    }
  }
  // at this point all optional and named required arguments read, assign now non-named required arguments
  if (reqa != NULL) {
    strReplace (&tReqaSav,reqa);
    wi = wordIterCreate (tReqaSav," ",1);
    while ((r1 = wordNext (wi)) != NULL) {
      if (a >= argc) {
        if (testOnly)
          return 0;
        else
          die ("At least one required argument missing.\nType '%s -h' for help",
               tArgv[0]);
      }
      if (tArgv[a][0] == '-' && tArgv[a][1] != '\0') {
        if (testOnly)
          return 0;
        else
          die ("Named argument '%s' must preceed non-named required arguments.\nType '%s -h' for help",
               tArgv[a],tArgv[0]);
      }
      currArg = arrayp (gArgs,arrayMax (gArgs),Arg);
      currArg->params = textCreate (1);
      currArg->name = r1;
      array (currArg->params,arrayMax (currArg->params),char *) = tArgv[a];
      a++;
    }
    wordIterDestroy (wi);
  }
  // this point is only reached if no fatal error occured
  return a;
}

int arg_isInit (void) {
  /**
     Test whether module is initialized
     @return 1 if arg_init() has been called before, else return 0
  */
  return gArgs != NULL;
}

int arg_init (int argc, char *argv[],
              char *optargs, char *reqargs,
              void (*usagef)(int level)) {
  /**
     Prepare command line option and argument processing;
     if the command line contains an error, a diagnostic message of the
     form "PROBLEM: cannot process argument ...; please type 'progname' for help"
     (where progname is argv[0] without leading pathname)<br>
     Postcondition: arg_getPos(), arg_present() can be called
     @param[in] argc,argv - semantics like in C main()
     @param[in] optargs - string describing optional arguments; syntax:
                          optname1[,paramcnt1] optname2[,paramcnt2] ...<br>
                          example: "force outfile,1"<br>
                          means: the command line has the syntax:<br>
                          prog [-force] [-outfile name]<br>
                          optargs can be NULL or "" to express "no options";
                          the default for paramcnt is 0.
     @param[in] reqargs - string describing required arguments; syntax:
                          argname1 argname2 ...<br>
                          example: "infile outfile"<br>
                          means: the command line has the syntax:<br>
                          prog [-infile ]infile [-outfile ]outfile<br>
                          even if reqargs is empty or NULL, it is checked that
                          at least one non-optional value is given
     @param[in] usagef - usage function: three help levels are supported:<br>
                         prog (without any arguments or options)
                               --> usagef(1) is called; should only
                                   show few lines with command syntax<br>
                         prog -h   --> usagef(2) is called; show info
                                       from (1) and explain options<br>
                         prog -help --> usagef(3) is called
                                        show full user manual
     @return number of arguments processed (if argc == arg_init(),
             then the whole command line was processed);
             the return value is always >= 1 because of the program
             name in argv[0] is considered processed.
  */
  return testAndInit (argc,argv,optargs,reqargs,usagef,0);
}

int arg_initTry (int argc, char *argv[],
                 char *optargs, char *reqargs,
                 void (*usagef)(int level)) {
  /**
     Prepare command line option and argument processing;
     if the command line contains an error, a diagnostic message of the
     form "PROBLEM: cannot process argument ...; please type 'progname' for help"
     (where progname is argv[0] without leading pathname)<br>
     Postcondition: arg_getPos(), arg_present() can be called
     @param[in] argc,argv - semantics like in C main()
     @param[in] optargs - string describing optional arguments; syntax:
                          optname1[,paramcnt1] optname2[,paramcnt2] ...<br>
                          example: "force outfile,1"<br>
                          means: the command line has the syntax:<br>
                          prog [-force] [-outfile name]<br>
                          optargs can be NULL or "" to express "no options";
                          the default for paramcnt is 0.
     @param[in] reqargs - string describing required arguments; syntax:
                          argname1 argname2 ...<br>
                          example: "infile outfile"<br>
                          means: the command line has the syntax:<br>
                          prog [-infile ]infile [-outfile ]outfile<br>
                          even if reqargs is empty or NULL, it is checked that
                          at least one non-optional value is given
     @param[in] usagef - usage function: three help levels are supported:<br>
                         prog (without any arguments or options)
                               --> usagef(1) is called; should only
                                   show few lines with command syntax<br>
                         prog -h   --> usagef(2) is called; show info
                                       from (1) and explain options<br>
                         prog -help --> usagef(3) is called
                                        show full user manual
     @return number of arguments processed (if argc == arg_init(),
             then the whole command line was processed) or
             0, if (argc,argv) did not match (optargs,reqargs).
  */
  return testAndInit (argc,argv,optargs,reqargs,usagef,1);
}

static void arg_possible (char *name) {
  /*
    return if 'name' occurs in optional or required arguments,
    else die()
  */
  if (gArgList == NULL)
    die ("arg_init()/arg_try() has not been called.");
  if (!arrayFind (gArgList,&name,NULL,(ARRAYORDERF)arrayStrcmp))
    die ("arg: '%s' is neither a required nor an optional argument.", name);
}

int arg_present (char *name) {
  /**
     check whether some argument was present on the command line or not;
     for required arguments returns always 1;
     for retrieving the value of arguments, use arg_getPos().<br>
     Precondition: arg_init() must have been called<br>
     Note: this function dies, if it is asked with a name that was not
           specified to exist in arg_init()/arg_try()
     @param[in] name - name of the argument, as specified in
                       arg_init (..., opta, reqa, ...)
     @return 1 if argument is present on command line, 0 if not
  */
  int i;
  Arg *currArg;

  arg_possible (name);
  for (i=0;i<arrayMax (gArgs);i++) {
    currArg = arrp (gArgs,i,Arg);
    if (strCaseEqual (name,currArg->name))
      return 1;
  }
  return 0;
}

char *arg_getPos (char *name,int pos) {
  /**
     Get the value of an argument;
     see also the simplified version arg_get();<br>
     Precondition: arg_init()<br>
     This function dies, if it is asked with a name that was not
     specified to exist in arg_init()/arg_try()
     @param[in] name - name of the argument, as specified in
                       arg_init(..., opta, reqa, ...)
     @param[in] pos - for arguments followed by one or more values pos >= 1
     @return NULL if the argument was not present on the command line,
             else a pointer to the requested value; the value
             returned is read-only to the user of the routine
  */
  int i;
  Arg *currArg;

  arg_possible (name);
  for (i=arrayMax (gArgs)-1;i>=0;i--) {
    currArg = arrp (gArgs,i,Arg);
    if (strCaseEqual (name,currArg->name)) {
      if (pos > arrayMax (currArg->params))
        die ("Argument '%s' has less than %d parameters.",name,pos);
      return textItem (currArg->params,pos-1);
    }
  }
  return NULL; // to keep compiler happy
}

char *arg_getProgName (void) {
  /**
     Get the program name<br>
     Precondition: arg_init()
     @return the program name without path
  */
  char *pos;

  if (gArg0 == NULL)
    die ("arg_getProgName without arg_init");
  pos = strrchr (gArg0,'/');
  if (pos != NULL)
    return pos+1;
  else
    return gArg0;
}

/*
  a small program to write all encountered arguments and their parameters

void usagef (int level)
{
  switch (level) {
    case 1 : usage ("program called with no arguments"); break;
    case 2 : usage ("program called with -h"); break;
    case 3 : usage ("program called with -help"); break;
    default: die ("oops");
  }
}

main (int argc,char *argv[])
{
  int i,k;
  Arg *currArg;

  if (arg_init (argc,argv,"minsim,1 bullshit,3 nlist","in",usagef) != argc)
    die ("too many arguments; invoke program without params for help");

  for (i=0;i<arrayMax (gArgs);i++) {
    currArg = arrp (gArgs,i,Arg);
    fprintf (stderr,"%s\n",currArg->name);
    for (k=0;k<arrayMax (currArg->params);k++)
      fprintf (stderr,"  %s\n",textItem (currArg->params,k));
  }
  fprintf (stderr,"\n\n");
}
*/
