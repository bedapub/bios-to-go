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
/** @file log.h
    @brief Module for handling warnings, errors, etc.
*/
#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>

//! Signature of functions that can be registered for die, warn, usage or romsg
typedef void (*RoMsgFunc)(char *format,va_list args);
//! Signature of functions that can be registered for exit
typedef void (*LogExitFunc)(void);

extern void log_suppressDefaultMessages (int i);

extern void log_registerExit (LogExitFunc f);
extern void log_registerDie (RoMsgFunc f);
extern void log_deregisterDie (RoMsgFunc f);
extern void log_registerWarn (RoMsgFunc f);
extern void log_deregisterWarn (RoMsgFunc f);
extern void log_registerUsage (RoMsgFunc f);
extern void log_deregisterUsage (RoMsgFunc f);
extern void log_registerRomsg (RoMsgFunc f);
extern void log_deregisterRomsg (RoMsgFunc f);

extern void die (char *format,...);
extern void warn (char *format,...);
extern void usage (char *format,...);
extern void romsg (char *format,...);

/* private */ extern int log_debugLevel;

/**
   Set the debug level
   @param[in] level - the level
*/
#define debugLevelSet(level) (log_debugLevel=(level))

/**
   Get the debug level
   @return the current level
*/
#define debugLevelGet() log_debugLevel

#ifdef HLR_DEBUG_ON
/**
   Write a debugging message if severe enough<br>
   Example:<br>
   debugMsg(1,("error %s",var))<br>
   debugLevelSet(9) - see all messages<br>
   debugLevelSet(0) - see only very serious messages<br>
   Note: the parenthesis surrounding the second argument of the macro - if
   omitted the compiler will produce messages which are hard to understand
   @param[in] level - the level of the message
   @param[in] msg - the message
*/
#define debugMsg(level,msg) {if (level<=debugLevelGet()) printf msg;}
#else
/**
   Write a debugging message<br>
   @param[in] level - the level of the message
   @param[in] msg - the message
*/
#define debugMsg(level,msg)
#endif

extern void logOpen (char *fname);
extern int logOpened (void);
extern void logWrite (char *format,...);
extern void logWriteT (char *format,...);
extern void logPrintTime (FILE *f);
extern void logClose (void);

extern void warnAdd (char *source,char *msg);
extern char *warnReport (void);
extern void warnReset (void);
extern int warnCount (char *sourceStart,char *msgStart);
extern void warnIterInit (void);
extern int warnIterNext (char **source,char **msg);

#ifdef HLR_ASSERT_OFF
/**
   Asserts that 'truecondition' is true;<br>
   If 'truecondition' evaluates to 0,
     'message' is displayed and the program is aborted.;<br>
   else (i.e. non-zero result) nothing happens;<br>
   hlr_assert is an expression that returns 1; therefore things like
     while (hlr_assert(memory_check(),"msg"), i<10) { ... }
   are syntactically valid.<br>
   If at compile time the symbol 'HLR_ASSERT_OFF' is defined,
   no code is generated for hlr_assert. The idea is to allow for
   programs to run in production mode as fast a possible.
   @paramp[in] truecondition - the condition that is expected
   @param[in] message - message to be printed if condition is not met
*/
#define hlr_assert(truecondition,message) 1
#else
/**
   Asserts that 'truecondition' is true;<br>
   If 'truecondition' evaluates to 0,
     'message' is displayed and the program is aborted.;<br>
   else (i.e. non-zero result) nothing happens;<br>
   hlr_assert is an expression that returns 1; therefore things like
     while (hlr_assert(memory_check(),"msg"), i<10) { ... }
   are syntactically valid.<br>
   If at compile time the symbol 'HLR_ASSERT_OFF' is defined,
   no code is generated for hlr_assert. The idea is to allow for
   programs to run in production mode as fast a possible.
   @param[in] truecondition - the condition that is expected
   @param[in] message - message to be printed if condition is not met
*/
#define hlr_assert(truecondition,message) ((truecondition)?1:(die(message),0))
#endif

#ifdef __cplusplus
}
#endif

#endif
