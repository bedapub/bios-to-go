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
/** @file log.c
    @brief Module for handling warnings, errors, etc.
*/
#include "plabla.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include PLABLA_INCLUDE_IO_UNISTD
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include "log.h"

/* --------------------------------------------------
   PART I:   printing messages to stderr - die/warn/usage/romsg

The idea is to write out parameterized messages potentially starting
with "PROBLEM: " and potentially terminating the program.
 - die() automatically prints "PROBLEM: " before the message
 - warn() automatically prints "WARNING: " before the message
 - die() and usage() automatically terminate the program
 - all (die(), warn(), usage(), romsg()):
   -- append a "\n" to the message
   -- flush all buffers before printing the message to stderr
   -- have the same interface as printf() i.e. a format string and a
      variable number of arguments

However, depending on the environment, something must
happen before the message is printed (e.g. write a HTTP CGI
header) or the program is terminated (e.g. close a connection).

For allowing this type of flexibility, own functions can be declared
using the type 'RoMsgFunc' and registered using log_registerDie(),
log_registerWarn(), log_registerUsage(), and log_registerRomsg().
The number of functions preceding the message printing is currently
limited to 5. If you need more check if you still understand your
program...

The application program can simply call die(), warn(), usage(), or
romsg() which will behave appropriately. The registered functions
will be called in the order they have been registered. To avoid
problems caused by direct or indirect recursion the program will be
terminated if one of the registered functions calls one of the error
reporting functions e.g. a call to die() in a function registered
with log_registerDie() will cause termination. It is however legal,
to register the same function twice although I'm not sure it makes
sense.

Registered functions can be deregistered by calling
log_deregisterDie(), log_deregisterWarn(), log_deregisterUsage(), or
log_deregisterRomsg. The order of deregistering calls is of no
importance and will delete the first occurrence of a function
registration when a function has been registered twice.
*/

#define MAX_HOOKS 5 //!< maximum of allowed hooks

static int gReentryFlag = 0;
static int gDieHookCount = 0;
static int gSuppressDefaultMessages = 0;

static RoMsgFunc gDieHooks[MAX_HOOKS];
static int gWarnHookCount = 0;
static RoMsgFunc gWarnHooks[MAX_HOOKS];
static int gUsageHookCount = 0;
static RoMsgFunc gUsageHooks[MAX_HOOKS];
static int gRomsgHookCount = 0;
static RoMsgFunc gRomsgHooks[MAX_HOOKS];
static LogExitFunc gExitHook = NULL;

static void logExit (void) {
  if (gExitHook)
    (*gExitHook) ();
  exit (1);
}

static void logRegisterHook (int *count,RoMsgFunc hooks[],RoMsgFunc f) {
  if (*count >= MAX_HOOKS)
    die ("logRegisterHook(): too many hooks (max: %d)",MAX_HOOKS);
  hooks[*count] = f;
  (*count)++;
}

static void logDeregisterHook (int *count,RoMsgFunc hooks[],RoMsgFunc f) {
  int i,j;

  for (i=0;i<*count;i++) {
    if (hooks[i] == f) {
      (*count)--;
      for (j=i;j<*count;j++)
        hooks[j] = hooks[j+1];
      return;
    }
  }
  die ("logDeregisterHook(): cannot deregister unknown hook");
}

static void logExecuteHooks (int count,RoMsgFunc hooks[],int doProblem,
                             int doExit,char *format,va_list args) {
  int i;

  if (gReentryFlag) {
    fflush (NULL); // write out all buffers
    fprintf (stderr,"PROBLEM: fatal reentry of log module: ");
    vfprintf (stderr,format,args);
    va_end (args);
    fprintf (stderr,"\n");
    logExit ();
  }
  else {
    gReentryFlag = 1;
    for (i=0;i<count;i++)
      (*hooks[i]) (format,args);
    gReentryFlag = 0;
    if (!gSuppressDefaultMessages || doExit) {
      fflush (NULL); // write out all buffers
      if (doProblem)
        fprintf (stderr,doExit ? "PROBLEM: " : "WARNING: ");
      vfprintf (stderr,format,args);
      va_end (args);
      fprintf (stderr,"\n");
    }
    if (doExit)
      logExit ();
  }
}

void log_suppressDefaultMessages (int i) {
  /**
     @param[in] i -- if called with 1: messages of warn or romsg will not be
                     written to stderr, if called with 0 (or default behavior):
                     messages are written to stderr
  */
  gSuppressDefaultMessages = i;
}

void log_registerExit (LogExitFunc f) {
  /**
     Register an exit function
     @param[in] f - the function to be called
  */
  gExitHook = f;
}

void log_registerDie (RoMsgFunc f) {
  /**
     Register a die function
     @param[in] f - the function to be added
  */
  logRegisterHook (&gDieHookCount,gDieHooks,f);
}

void log_deregisterDie (RoMsgFunc f) {
  /**
     De-register a die function
     @param[in] f - the function to be removed
  */
  logDeregisterHook (&gDieHookCount,gDieHooks,f);
}

void log_registerWarn (RoMsgFunc f) {
  /**
     Register a warn function
     @param[in] f - the function to be added
  */
  logRegisterHook (&gWarnHookCount,gWarnHooks,f);
}

void log_deregisterWarn (RoMsgFunc f) {
  /**
     De-register a warn function
     @param[in] f - the function to be removed
  */
  logDeregisterHook (&gWarnHookCount,gWarnHooks,f);
}

void log_registerUsage (RoMsgFunc f) {
  /**
     Register a usage function
     @param[in] f - the function to be added
  */
  logRegisterHook (&gUsageHookCount,gUsageHooks,f);
}

void log_deregisterUsage (RoMsgFunc f) {
  /**
     De-register a usage function
     @param[in] f - the function to be removed
  */
  logDeregisterHook (&gUsageHookCount,gUsageHooks,f);
}

void log_registerRomsg (RoMsgFunc f) {
  /**
     Register a romsg function
     @param[in] f - the function to be added
  */
  logRegisterHook (&gRomsgHookCount,gRomsgHooks,f);
}

void log_deregisterRomsg (RoMsgFunc f) {
  /**
     De-register a romsg function
     @param[in] f - the function to be removed
  */
  logDeregisterHook (&gRomsgHookCount,gRomsgHooks,f);
}

void die (char *format,...) {
  /**
     Exit the program but not before executing all registered die functions
     @param[in] format - the format of the message
  */
  va_list args;

  va_start (args,format);
  logExecuteHooks (gDieHookCount,gDieHooks,/*problem*/1,/*exit*/1,
                   format,args);
}

void warn (char *format,...) {
  /**
     Write a warning message after executing all registered warn functions
     @param[in] format - the format of the message
  */
  va_list args;

  va_start (args,format);
  logExecuteHooks (gWarnHookCount,gWarnHooks,/*problem*/1,/*exit*/0,
                   format,args);
}

void usage (char *format,...) {
  /**
     Write a usage message after executing all registered usage functions
     @param[in] format - the format of the message
  */
  va_list args;

  va_start (args,format);
  logExecuteHooks (gUsageHookCount,gUsageHooks,/*problem*/0,/*exit*/1,
                   format,args);
}

void romsg (char *format,...) {
  /**
     Write a romsg message after executing all registered romsg functions
     @param[in] format - the format of the message
  */
  va_list args;

  va_start (args,format);
  logExecuteHooks (gRomsgHookCount,gRomsgHooks,/*problem*/0,/*exit*/0,
                   format,args);
}

/* --------------------------------------------------------------------------
   PART II: printing messages to a log file
*/

static FILE *gLogFile = NULL;

void logOpen (char *fname) {
  /**
     The file name the log should go to can either be set statically
     via the argument fname, or if not specified can be set dynamically
     via the environment.
     postcondition: stderr is redirected into 'fname'
                    logWrite() writes into file 'fname'
                    logClose() can be called
     @param[in] fname -- name of logfile
                         if fname==NULL, fname is taken from
                         the environment variable LOGFILE
  */
  int fd;
  if (fname == NULL)
    fname = getenv ("LOGFILE");
  if (fname[0] == '\0')
    fname = NULL; // ignore empty file name
  if (fname != NULL) {
    /* redirect stderr into a file while keeping a stream variable
       as a name for the log stream
       so log output can go to gLogFile, errors from anywhere go to stderr,
       but reach the same log file
    */
    if (fflush (stderr))
      perror ("PROBLEM: log: fflush");
    if (PLABLA_CLOSE (2) == -1)
      perror ("PROBLEM: log: close(2)");
    fd = PLABLA_OPEN (fname,O_WRONLY | O_CREAT | O_APPEND,0664);
    if (fd == -1) {
      printf ("PROBLEM: could not open log file %s\n",fname);
      perror ("PROBLEM: log: open");
    }
    if (fd != 2)
      printf ("PROBLEM: could not redirect stderr (fd %d)\n",fd);
    setbuf (stderr,0);
    gLogFile = stderr;
    if (!gLogFile)
      die ("logOpen: could not open log file %s\n",fname);
  }
}

int logOpened (void) {
  /**
     @return 1 if Log() messages have been redirected,
             0 if such messsages go to stderr
  */
  return gLogFile != NULL;
}

void logWrite (char *format,...) {
  /**
     Format a log message
     @param[in] format - the format
  */
  va_list args;

  va_start (args,format);
  vfprintf ((gLogFile) ? gLogFile : stderr,format,args);
  va_end (args);
}

void logWriteT (char *format,...) {
  /**
     like logWrite() but prefix with timestamp
     (format see logPrintTime())
  */
  va_list args;

  logPrintTime ((gLogFile) ? gLogFile : stderr);
  va_start (args,format);
  vfprintf ((gLogFile) ? gLogFile : stderr,format,args);
  va_end (args);
}

void logPrintTime (FILE *f) {
  /**
     on stream f print a timestamp of the form
     123456789012345678901234
     2000-08-30_00:00:00 \0
     @param[in] f -- the stream to write messages to
  */
  char ts[22];
  time_t t = time (NULL);
  strftime (ts,sizeof (ts),"%Y-%m-%d_%T ",localtime (&t));
  fputs (ts,f);
}

void logClose (void) {
  /**
     Close the log file
  */
  if (gLogFile == NULL)
    return;
  fclose (gLogFile);
}

/* -------------------------------------------------------------
   PART III: assertions and messages at debug time

Assertions are meant to check for specific condtions to be true
at all times. These checks can be expensive. They can be turned off
(produce no code) at compile time by "#define HLR_ASSERT_OFF".

Debug messages should be helpful in debugging modules and programs
during debug time. At production time no code should be produced.
Messages can be associated with a 'detail level'. The more high level
a message is, the lower the number. Level 1 is the highest
abstraction level, level 9 shows even the minutest detail.
The debugging code can be turned on at compile time
by defining "#define HLR_DEBUG_ON".
*/

/* asserts that 'truecondition' is true;
   if 'truecondition' evaluates to 0,
   'message' is displayed and the program is aborted.;
   else (i.e. non-zero result) nothing happens;
   hlr_assert is an expression that returns 1; therefore things like
     while (hlr_assert(memory_check(),"msg"), i<10) { ... }
   are syntactically valid.
#ifdef HLR_ASSERT_OFF
#define hlr_assert(truecondition,message)
#else
#define hlr_assert(truecondition,message) ((truecondition)?1:(die(message),0))
#endif
*/

/**
  debug message example:<br>
  debugLevelSet (9); // see all messages (default)<br>
  debugLevelSet (1); // see only very serious messages<br>
  debugMsg (2,("error %s",var)); // prints nothing because detail level was set to two<br>
  Note: the parenthesis surrounding the second argument of the macro if
        omitted the compiler will produce messages which are hard to understand
  <br>
  implementation:<br>
  \#define debugLevelSet(level) (log_debugLevel=(level))<br>
  \#define debugLevelGet() log_debugLevel<br>
  \#ifdef HLR_DEBUG_ON<br>
  \#define debugMsg(level,msg) {if (level<=debugLevelGet()) printf msg;}<br>
  \#else<br>
  \#define debugMsg(level,msg) <br>
  \#endif<br>
*/
int log_debugLevel = 9;

/* -------------------------------------------------------------
   PART IV: buffering warnings

Exception Handling in BIOS

Design Concept

Motivation

When writing re-usable modules it is not forseeable in which context
they will be used. It could e.g. be in a batch run or in an
interactive program (WWW CGI). At runtime a module may discover
things going wrong, e.g. a function is called with illegal
parameters, a subsystem like Oracle denies service or the disk is
full. Module (re-)usability in different application contexts is
achieved by separating exception generation (where the code notices
something is going wrong) from exception handling (where it is
decided to inform the user, stop processing, start countermeasures,
etc). Modern computer languages like Java have this separation
built into their syntax (throw/catch/try). C does not have this,
but it can be emulated as described below.

In BIOS the following exception handling policy is adopted:
- the function warn() (from log.c) is NOT used in BIOS.
- if something goes wrong and is clearly a
  programmer's error (wrong usage or wrong module code),
  the module calls die() and the programmer is
  forced to fix the bug.
- if something goes wrong and it is not a programming
  error, the module does not print any messages anywhere,
  but returns some error status to the caller.
  To ease this task, module log.c provides a
  a common communication point via routines
  warnAdd(), warnCount(), etc., see below.
- for immediate problem reporting to the end user
  the following functions are available:
  die() -- for programmers errors ("PROBLEM:")
             end users should be educated to report PROBLEMs.
  usage() -- e.g. for displaying program usage (like die(),
             but without "PROBLEM:"))
  warn() -- for immediate reporting of problems/warning
             to users (prints "WARNING: ...")
  romsg() -- like warn(), but without prefix
*/

#define WARNMAX 30 //!< maximum of warn exceptions that can be registered
static char *gSources[WARNMAX];
static char *gMsgs[WARNMAX];
static int gWarnCnt = 0; // # of warn exceptions registered
static int gWarnIndex = -2; // for use by the iterator

void warnAdd (char *source,char *msg) {
  /**
     add exception notification;
     precondition: none;
     postcondition: warnReport() will show (source, msg);
     @param[in] source -- name of the function issuing the warning;
                          source must not contain a tab char
     @param[in] msg -- message describing the warning
  */
  gWarnIndex = -2;
  ++gWarnCnt;
  if (gWarnCnt > WARNMAX) {
    char s[80];

    free (gSources[WARNMAX-1]);
    free (gMsgs[WARNMAX-1]);
    gSources[WARNMAX-1] = strdup ("warnAdd");
    sprintf (s,"Warning buffer overflow. Last %d warning(s) discarded.",
             gWarnCnt - WARNMAX);
    gMsgs[WARNMAX-1] = strdup(s);
  }
  else {
    if (source == NULL || msg == NULL)
      die ("warnAdd() with NULL arg");
    gSources[gWarnCnt-1] = strdup (source);
    gMsgs[gWarnCnt-1] = strdup (msg);
  }
}

char *warnReport (void) {
  /**
     @return all warnings issued since process start
             or warnReset() as a string of the form
             source tab msg newline
             source tab msg newline
             ...
             memory managed by this routine; may be written to,
             but not free'd or realloc'd
             If warnCount(NULL,NULL) == 0, an empty string ("")
             is returned
  */
  int i;
  int s = 0;
  int cnt;
  static char *b = NULL;

  cnt = (gWarnCnt > WARNMAX) ? WARNMAX : gWarnCnt;
  for (i=0;i<cnt;i++) {
    s += strlen (gSources[i]);
    s += strlen (gMsgs[i]);
  }
  if (b != NULL)
    free (b);
  b = (char *)malloc (s + cnt * 2 + 1);
  b[0] = '\0';
  for (i=0;i<cnt;i++) {
    strcat (b,gSources[i]);
    strcat (b,"\t");
    strcat (b,gMsgs[i]);
    strcat (b,"\n");
  }
  return b;
}

void warnReset (void) {
  /**
     return the expection buffer to the state at process start
     (no expections added)
     postcondition: warnCount(NULL,NULL) == 0
  */
  int i;
  int cnt;

  cnt = (gWarnCnt > WARNMAX) ? WARNMAX : gWarnCnt;
  for (i=0;i<cnt;i++) {
    free (gSources[i]);
    free (gMsgs[i]);
  }
  gWarnIndex = -2;
  gWarnCnt = 0;
}

int warnCount (char *sourceStart,char *msgStart) {
  /**
     note: warnCount(NULL,NULL) is a fast way to check that
           no warnings have been reported since process start
           or the last call to warnReset()
     @param[in] sourceStart -- e.g. "seo_",
                               NULL or "" means no restriction
     @param[in] msgStart -- NULL or "" means no restriction
     @return number of warnings whose 'source' starts with
             'sourceStart' and whose 'msg' starts with
             'msgStart', 0 else
  */
  int i;
  int cnt;
  int c = 0;
  int ls = (sourceStart ? strlen (sourceStart) : 0);
  int lm = (msgStart ? strlen (msgStart) : 0);

  if (ls == 0 && lm == 0)
    return gWarnCnt;
  cnt = (gWarnCnt > WARNMAX) ? WARNMAX : gWarnCnt;
  for (i=0;i<cnt;i++) {
    if (ls != 0 && strncmp (gSources[i],sourceStart,ls) != 0)
      continue;
    if (lm != 0 && strncmp (gMsgs[i],msgStart,lm) != 0)
      continue;
    c++;
  }
  return c;
}

void warnIterInit (void) {
  /**
     Start iteration over all warning currently registered
     in chronological order. The first warning added will be
     the first one returned by warnIterNext().
     postcondition: warnIterNext() can be called
  */
  gWarnIndex = -1;
}

int warnIterNext (char **source,char **msg) {
  /**
     precondition: warnIterInit() or warnIterNext() has been
                   called since the last warnAdd()
     postcondition: next call to this function will return next warning
     @param[in] source -- place to store a char*
     @param[in] msg -- place to store a char*
     @param[out] source -- points to source description of warning
                           memory belongs to this routine; read-only to user
     @param[out] msg -- points to message text of warning
                        memory belongs to this routine; read-only to user
     @return 1 if warning returned, 0 if end of iteration
  */
  if (gWarnIndex == -2)
    die ("warnIterNext without warnIterInit");
  if (++gWarnIndex >= gWarnCnt || gWarnIndex >= WARNMAX)
    return 0;
  *source = gSources[gWarnIndex];
  *msg = gMsgs[gWarnIndex];
  return 1;
}
