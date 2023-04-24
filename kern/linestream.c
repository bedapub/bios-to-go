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
/** @file linestream.c
    @brief Module for reading arbitrarily long lines from files,
    pipes or buffers.
    Module prefix ls_
*/
#include "plabla.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include PLABLA_INCLUDE_IO_UNISTD

#include "log.h"
#include "format.h"
#include "hlrmisc.h"
#include "linestream.h"

static void register_nextLine (LineStream this1,char *(*f)(LineStream this1)) {
  /**
     Internally used to register the actual function which gets the next line
  */
  this1->nextLine_hook = f;
}

static char *nextLineFile (LineStream this1) {
  /**
     Returns the next line of a file and closes the file if
     no further line was found. The line can be of any length.
     A trailing \n or \r\n is removed.
     @param[in] this1 = line stream object
     @return the line (memory managed by this routine);
             NULL if no further line was found
  */
  int ll;

  if (this1 == NULL)
    die ("nextLineFile: NULL LineStream");
  if ((ll = getLine (this1->fp,&this1->line,&this1->lineLen)) == 0) {
    fclose (this1->fp);
    this1->fp = NULL;
    hlr_free (this1->line);
    return NULL;
  }
  if (ll > 1 && this1->line[ll-2] == '\r')
    this1->line[ll-2] = '\0';
  else if (ll > 0 && this1->line[ll-1] == '\n')
    this1->line[ll-1] = '\0';
  this1->count++;
  return this1->line;
}

LineStream ls_createFromFile (char *fn) {
  /**
     Creates a line stream from a file.<br>
     To learn details call warnReport() from module log.c
     @param[in] fn - file name ("-" means stdin)
     @return a line stream object;
             NULL if file could not been opened;
  */
  LineStream this1;

  if (fn == NULL)
    die ("ls_createFromFile: no file name given");
  this1 = (LineStream)hlr_malloc (sizeof (struct _lineStreamStruct_));
  this1->line = NULL;
  this1->count = 0;
  this1->status = 0;
  if (strcmp (fn,"-") == 0)
    this1->fp = stdin;
  else
    this1->fp = fopen (fn,"r");
  if (this1->fp == NULL) {
    static Stringa msg = NULL;

    stringCreateOnce (msg,100);
    stringPrintf (msg,"'%s': %s",fn,strerror (errno));
    warnAdd ("ls_createFromFile",string (msg));
    hlr_free (this1);
    return NULL;
  }
  register_nextLine (this1,nextLineFile);
  this1->buffer = NULL;
  return this1;
}

static char *nextLinePipe (LineStream this1) {
  /**
     Returns the next line from the pipe and closes the pipe if
     no further line was found. The line can be of any length and
     is returned without \n at the end
     @param[in] this1 - line stream object
     @return the line (memory managed by this routine);
             NULL if no further line was found
  */
  int ll;

  if (this1 == NULL)
    die ("nextLinePipe: NULL LineStream");
  if ((ll = getLine (this1->fp,&this1->line,&this1->lineLen)) == 0) {
    this1->status = PLABLA_PCLOSE (this1->fp);
    this1->fp = NULL;
    hlr_free (this1->line);
    return NULL;
  }
  if (ll > 1 && this1->line[ll-2] == '\r')
    this1->line[ll-2] = '\0';
  else if (ll > 0 && this1->line[ll-1] == '\n')
    this1->line[ll-1] = '\0';
  this1->count++;
  return this1->line;
}

LineStream ls_createFromPipe (char *command) {
  /**
     Creates a line stream from a pipe, e.g.
     ls_createFromPipe ("zcat test.dat.Z");<br>
     Postcondition: warnCount(NULL,NULL) !=0 if problem occured.
     @param[in] command - a command as it would be written on the command line
     @return a line stream object;
             NULL if the pipe could not been opened
  */
  LineStream this1;

  if (command == NULL)
    die ("ls_createFromPipe: no command given");
  this1 = (LineStream)hlr_malloc (sizeof (struct _lineStreamStruct_));
  this1->line = NULL;
  this1->count = 0;
  this1->status = -2; // undetermined
  this1->fp = PLABLA_POPEN (command,"r");
  if (this1->fp == NULL) {
    warnAdd ("ls_createFromPipe",
             stringPrintBuf ("'%s': %s",command,strerror (errno)));
    return NULL;
  }
  register_nextLine (this1,nextLinePipe);
  this1->buffer = NULL;
  return this1;
}

static char *nextLineBuffer (LineStream this1) {
  /**
     Returns the next line of a buffer. The line can be of any length and
     a trailing \n or \r\n is removed.
     @param[in] this1 - line stream object
     @return the line;
             NULL if no further line was found
  */
  char *s;
  int len;

  if (this1 == NULL)
    die ("nextLineBuffer: NULL LineStream");
  s = wordNextG(this1->wi, &len);
  if (s == NULL) {
    wordIterDestroy (this1->wi);
    return NULL;
  }
  this1->count++;
  if (len && s[len-1] == '\r')
    s[len-1] = '\0';
  return s;
}

LineStream ls_createFromBuffer (char *buffer) {
  /**
     Creates a line stream from a buffer.<br>
     The buffer will be destroyed after using ls_nextLine
     ('\n' replaced by '\0'). Work on a copy of the buffer
     if you want to use it again.
     @param[in] buffer - a buffer pointer, must not be NULL
     @return a line stream object
  */
  LineStream this1;
  int len;
  int manySepsAreOne = 0;

  if (buffer == NULL)
    die ("ls_createFromBuffer: NULL buffer");
  len = strlen (buffer);
  if (len > 0) {
    if (buffer[len-1] == '\n')
      buffer[--len] = '\0';
  }
  else
    manySepsAreOne = 1; // immediately return NULL
  this1 = (LineStream)hlr_malloc (sizeof (struct _lineStreamStruct_));
  this1->count = 0;
  this1->status = 0;
  this1->wi = wordIterCreate (buffer,"\n",manySepsAreOne);
  register_nextLine (this1,nextLineBuffer);
  this1->buffer = NULL;
  return this1;
}

void ls_destroy_func (LineStream this1) {
  /**
     Destroys a line stream object after closing the file or pipe
     if they are still open (stream not read to the end) or after
     destroying the word iterator if the stream was over a buffer.<br>
     Do not call this function but use the macro ls_destroy.
     @param[in] this1 - a line stream object
  */
  char line[1000];

  if (this1 == NULL)
    return;
  if (this1->nextLine_hook == nextLinePipe && this1->fp) {
    while (fgets (line,sizeof (line),this1->fp)) {}
    this1->status = PLABLA_PCLOSE (this1->fp);
    hlr_free (this1->line);
  }
  else if (this1->nextLine_hook == nextLineFile && this1->fp) {
    // if (this1->fp == stdin)
    if (!PLABLA_ISATTY (fileno (this1->fp)))
      while (fgets (line,sizeof (line),this1->fp))
        ;
    fclose (this1->fp);
    hlr_free (this1->line);
  }
  else if (this1->nextLine_hook == nextLineBuffer && this1->wi) {
    wordIterDestroy (this1->wi);
  }
  stringDestroy (this1->buffer);
  hlr_free (this1);
}

char *ls_nextLine (LineStream this1) {
  /**
     This function is called from the application programs
     independently whether the stream is from a file, pipe or buffer.<br>
     The memory returned belongs to this routine; it may
     be read and written to, but not free'd or realloc'd
     by the user of this routine; it stays stable until
     the next call ls_nextLine(this1).
     @param[in] this1 - a line stream
     @return a line without trailing \n if there is still a line,
             else NULL
  */
  char *line;
  if (this1 == NULL)
    die ("%s",warnCount (NULL,NULL) ? warnReport () :
         "ls_nextLine: invalid LineStream");
  if (this1->buffer != NULL) {
    if (this1->bufferBack > 0) {
      this1->bufferBack = 0;
      line = this1->bufferLine;
    }
    else {
      // only get the next line if there we did not yet see the end of file
      line = this1->bufferLine ? this1->nextLine_hook (this1) : NULL;
      if (line != NULL) {
        stringCpy (this1->buffer,line);
        this1->bufferLine = string (this1->buffer);
      }
      else
        this1->bufferLine = NULL;
    }
  }
  else
    line = this1->nextLine_hook (this1);
  return line;
}

void ls_bufferSet (LineStream this1,int lineCnt) {
  /**
     Set how many lines the linestream should buffer.<br>
     Precondition: ls_create*<br>
     Postcondition: ls_back() will work
     @param[in] this1 - a line stream
     @param[in] lineCnt - how many lines should ls_nextLine() repeat
                          (currently, only lineCnt==1 is supported)
  */
  if (this1->buffer != NULL || this1->count > 0)
    die ("ls_bufferSet() more than once or too late");
  if (lineCnt != 1)
    die ("ls_bufferSet() sorry, not yet implemented");
  this1->buffer = stringCreate (80);
  this1->bufferBack = 0;
  this1->bufferLine = ""; // dummy init, to kick off reading
}

void ls_back (LineStream this1,int lineCnt) {
  /**
     Push back 'lineCnt' lines.<br>
     Precondition: ls_bufferSet() was called.<br>
     Postcondition: next call to ls_nextLine() will return the same line again
     @param[in] this1 - a line stream
     @param[in] lineCnt - how many lines should ls_nextLine() repeat
                          (currently, only lineCnt==1 is supported)
  */
  if (this1->buffer == NULL)
    die ("ls_back() without preceeding ls_bufferSet()");
  if (this1->bufferBack != 0)
    die ("ls_back() twice in a row");
  if (lineCnt != 1)
    die ("ls_back: sorry, not yet implemented");
  this1->bufferBack = 1;
}

int ls_lineCountGet (LineStream this1) {
  /**
     Returns the number of the current line.
     @param[in] this1 - a line stream
  */
  return this1->count;
}

int ls_skipStatusGet (LineStream this1) {
  /**
     Skips remainder of line stream and returns exit status which is
     only meaningful when created from a pipe - exit status for file
     and buffer will always be 0.<br>
     Postcondition: line stream is read to its end - ls_nextLine()
                    must not be called anymore.<br>
     Note: for reasons of efficiency, skip does not actually read the
           linestream therefore ls_lineCountGet() will not return the
           correct line number after this function has been called.
     @param[in] this1 - a line stream
     @return if 'this1' was created by ls_createFromPipe(command),
             the exit status of 'command' is returned; else 0
  */
  if (this1->nextLine_hook == nextLineBuffer) {
    if (this1->wi != NULL)
      wordIterDestroy (this1->wi);
  }
  else if (this1->nextLine_hook == nextLineFile) {
    if (this1->fp != NULL) {
      fclose (this1->fp);
      this1->fp = NULL;
      hlr_free (this1->line);
    }
  }
  else if (this1->nextLine_hook == nextLinePipe) {
    if (this1->fp != NULL)
      while (nextLinePipe(this1))
        ;
  }
  return this1->status;
}

void ls_cat (LineStream this1,char *filename) {
  /**
     Redirect a linestream.<br>
     Postcondition: this1 - contains no more lines;
                            file 'filename' contains the contents of 'this1'
     @param[in] this1 - line stream created by one of ls_create*()
     @param[in] filename - name of file to write lines to; special cases:
                           '-'  means stdout;
                           NULL means /dev/null  (discard);
  */
  if (filename != NULL) {
    char *line;
    FILE *f;
    if (strEqual (filename,"-"))
      f = stdout;
    else
      f = fopen (filename,"w");
    if (f == NULL) {
      die ("%s: in ls_cat(%s)",strerror (errno),filename);
    }
    while ((line = ls_nextLine (this1)) != NULL) {
      fputs (line,f);
      putc ('\n',f);
    }
    if (f != stdout)
      fclose (f);
  }
  else
    while (ls_nextLine(this1))
      ;
}

int ls_isEof (LineStream this1) {
  /**
     This function is called to determine the state of a line stream,
     i.e. to decide which methods may be invoked on it.
     @param[in] this1 - a line stream
     @return 0 if there are more lines retrievable by ls_nextLine().
             1 if line stream is at end i.e. ls_nextLine() has returned
             NULL and must not be called anymore.
  */
  if (this1->nextLine_hook == nextLineBuffer)
    return this1->wi == NULL ? 1 : 0;
  else
    return this1->fp == NULL ? 1 : 0;
}
