/*****************************************************************************
*                                                                            *
*  Copyright (C) 2001,  F. Hoffmann-La Roche & Co., AG, Basel, Switzerland.  *
*                                                                            *
* This file is part of "Roche Bioinformatics Software Objects and Services"  *
*    written by the bioinformatics group at Hoffmann-La Roche, Basel.        *
*      It shall not be reproduced or copied or disclosed to others           *
*                      without written permission.                           *
*                                                                            *
*                          - All rights reserved -                           *
*                                                                            *
*****************************************************************************/

/* file: rofutil.c
file handling utilities (and related stuff)
module prefix: hlr_, dio_
   (beware: hlrmisc also uses prefix hlr_)

the master version of this file is in location: 
bioinfo.bas.roche.com:/SOFT/bi/bios/src/libdev/

2021-08-03  zhangj83: fixing file size checker's warning of 2<<31 in GCC
2008-12-19  wolfd: add hlr_setCoredumpsize()
2007-10-18  gaiserb,wolfd: add hlr_fileRead()
2005-10-07  wolfd: add _FILE_OFFSET_BITS 64 for 32 bit linux
2005-09-20  wolfd: switch to 64bit file size
2005-08-02  ebelingm: changed hlr_fileSizeGet and hlr_dirScanFileSizeGet to functions returning long
2005-04-01  steineg3: added hlr_dirScanLastAccessGet()
2004-08-16  steineg3: Added warning about flock failing for NFS mounts to documentation of hlr_fileLock()
2004-01-07  wolfd, steineg3: changed user interface for hlr_fileUnlock():
                             file handles are now passed as argument rather than names;
			     hlr_fileLock() and hlr_fileLockNB() return handles for
			     the locked file
2002-03-05  wolfd: add BIOS_PLATFORM == BIOS_PLATFORM_LINUX
2001-12-03  wolfd: make scandir_BSD() return 0 if no files found
2001-12-03  wolfd: add hlr_ipcfSet() and hlr_ipcfGetOnce()
2001-01-15  strahmm: added hlr_fopenAppend()
2001-01-03  CB added hlr_fileLastUpdateGet
2000-09-27  holzwarj/wolfd: added directory object (dio)
2000-09-25  klenka: added hlr_getDir(), hlr_getLink()
2000-06-15  wellsi: added hlr_fileSizeGet()
1999-10-28  holzwarj:  added hlr_getHomeDir()
1999-10-14  klenka: added hlr_fileCopy(), fixed bug in implementation of hlr_fileMove()
1999-09-27  klenka: completed directory iterator
1999-09-24  klenka: renamed hlr_isPlainFile() to hlr_queryPlainFile(), 
                    hlr_isDir() to hlr_queryDir(), 
		    added hlr_fileEqual(), hlr_fileRemove(), hlr_fileMove(), hlr_mkDir(),
		    and directory iterator
1999-06-15  wolfd: make fileUnlock() do what it says
1998-11-04  wolfd: add lock/unlock
1997-01-14 13:31  wolfd -- renamed file from fileutil to
                           rofutil because of name collision with GCG
started: 1996-11-07 11:15  wolfd

*/

#if BIOS_PLATFORM == BIOS_PLATFORM_LINUX && BIOS_BITS_PER_INT == 32
/* this overrides features.h (which in included by stat.h 
   make sure you do not #include <sys/stat.h> before #include "rofutil.h" (!)
*/
#define _FILE_OFFSET_BITS 64
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <pwd.h>
#include <errno.h>
#include <dirent.h>
#include <fnmatch.h>

#include "plabla.h"
#include "log.h"
#include "hlrmisc.h"
#include "format.h"

#include "rofutil.h"

void hlr_setCoredumpsize(int size) 
{ /*
  input: size -- max core dump size in bytes, 0 = unlimited 
  */
  struct rlimit rl ; 
  rl.rlim_cur = size ? size : RLIM_INFINITY ;
  setrlimit(RLIMIT_CORE, &rl) ; 
}

#if BIOS_PLATFORM == BIOS_PLATFORM_SOLARIS
static int scandir_BSD(char *dirname,struct dirent ***in_names,char *filter);

#endif

static int gLastLocked = -1; /* file descriptor for last file that has been locked */


char* hlr_queryPlainFile(char *filename) 
{ /* 
  checks if 'filename' is the name of a plain file 
  returns NULL if filename is ok, else a diagnostic string 
  */
  struct PLABLA_STAT s ;
  if (PLABLA_STAT(filename, &s)) return "file not accessible" ;
  if ((s.st_mode & S_IFMT) != S_IFREG) return "is not a plain file" ;
  return NULL ;
}

char* hlr_queryDir(char *dirname) 
{ /* 
  checks if dirname is a directory
  returns NULL if dirname is ok, else a diagnostic string 
  */
  struct PLABLA_STAT s ;
  if (PLABLA_STAT(dirname, &s)) return "directory not accessible" ;
  if ((s.st_mode & S_IFMT) != S_IFDIR) return "is not a directory" ;
  return NULL ;
}

char *hlr_tail(char *fileNameWithPath) { /*
  produces the same result as the :t (tail) modifier
  available in the UNIX C-shell (csh), i.e.
  returns: pointer to the first character of the filename
           without the leading path
  examples: ../somepath/file  --> pointer to the beginning of 'file'
            file              --> pointer to the beginning of 'file'
  */
  char *slashp = strrchr(fileNameWithPath, '/') ;
  return slashp ? slashp+1 : fileNameWithPath ;
}

char *hlr_getDir(char *fileNameWithPath) { /*
  returns the directory part of a full path name omitting the filename
  --> opposite of hlr_tail(), no analog modifier in csh
  input: fileNameWithPath -- full path name to be split, must not be NULL
  returns: pointer to directory part without a trailing separator '/' 
           except for root directory, memory is read-only to user
  examples: "/" --> "/"
            "abc" --> ""
	    "/abc" --> "/"
	    "/abc/" --> "/abc"
	    "/abc/def" --> "/abc"
	    "abc/def" --> "abc"
	    "./abc" --> "."
	    "../abc/def" --> "../abc"
  */
  static char buf[MAXPATHLEN+1] ;
  char *cp = ILLADR ;
  strcpy(buf, fileNameWithPath) ;
  if (cp = strrchr(buf, '/'))
    if (cp == &buf[0]) /* if rightmost '/' is at start, keep it */
      *(cp + 1) = '\0' ;
    else
      *cp = '\0' ;
  else
    buf[0] = '\0' ;
  return buf ;
}

FILE *hlr_fopenRead(char *filename) { /*
  open a file for read, resp. stdin if filename is '-'
  abort if file not readable
  returns: file pointer -- use fclose() when done
  */
  FILE *f = strcmp(filename, "-") ? fopen(filename, "r") : stdin ;
  if (!f) 
    die("hlr_fopenRead: %s: %s", strerror(errno), filename) ;
  return f ;
}
  
FILE *hlr_fopenWrite(char *filename) { /*
  open a file for writing, resp. stdout if filename is '-'
  abort if file not writeable
  returns: file pointer -- use fclose() when done
  */
  FILE *f = strcmp(filename, "-") ? fopen(filename, "w") : stdout ;
  if (!f) 
    die("hlr_fopenWrite: %s: %s", strerror(errno), filename) ;
  return f ;
}
  
FILE *hlr_fopenAppend(char *filename) { /*
  open a file for appending, resp. stdout if filename is '-'
  abort if file not writeable
  returns: file pointer -- use fclose() when done
  */
  FILE *f = strcmp(filename, "-") ? fopen(filename, "a") : stdout ;
  if (!f) 
    die("hlr_fopenAppend: %s: %s", strerror(errno), filename) ;
  return f ;
}
  
char *hlr_getUser(void) 
{ /*
  determine name of user of current process by asking
  the operating system
  returns: username
  */
  static char *user=NULL ;
  if (!user) {
    struct passwd *pp ;
    if (! (pp = getpwuid(getuid()))) {
      die("hlr_getUser: %s", strerror(errno)) ;
    }
    user = hlr_strdup(pp->pw_name) ;
  }
  return user ;
}

int hlr_isAbsolutePath(char *path)
{ /*
  determine if supplied path is absolute or relative
  input: any path, directory or filename
  returns: 1 if path is absolute, 0 if relative
  */
  /*
  1998-06-22  klenka
  */

  return path[0]=='/' ;
}

char *hlr_extension(char *filename)
{ /*
  find file extension
  input: filename
  returns: pointer to first character of extension in filename, if any, else NULL
  */
  /*
  1998-06-22  klenka
  */
  return strrchr(hlr_tail(filename), '.') ;
}

int hlr_fileLock(char *filename)
{ /*
  acquire lock on file with name 'filename'. Wait, if the lock is
  held by some other process until released.

  input:         filename  -- name of an existing file that is readable
                              to the caller
  returns:       lockID
                 this is used as an argument for hlr_fileUnlock()
  postcondition: another process calling hlr_fileLock(filename) will wait
                 until hlr_fileUnlock(filename) is called or the process
                 holding the lock terminates. hlr_fileLockNB(filename) will see the lock.
                 hlr_fileUnlock(-1) is equivalent to hlr_fileUnlock(lockID just returned)
  notes: 
   - one process can acquire several locks (one per file); for concurrent
     processes be sure to acquire the locks in the same order or
     deadlock will result
   - be careful with fork(), dup() and the like in this context
   - it is not advisable to lock the same file multiple times within one
     process
   - The last lockID is remembered:
       hlr_fileLock("myfile") ;
       hlr_fileUnlock(-1) // unlocks "myfile" 

       hlr_fileLock("myfile1") ;
       hlr_fileLockNB("myfile2") ;
       hlr_fileUnlock(-1) // unlocks "myfile2" ! 
       hlr_fileUnlock(-1) // die() 
   - The flock mechanism does not work for NFS mounted media and produces
     the error 'No locks available' if tried. fcntl could be used instead
     of flock to avoid this problem, if needed.
  */
  /*
  implementation note:
  the 'lockid' returned is acutually a file handle and can be used
  with read() and write() -- but at your own risk!
  */

  gLastLocked = open(filename, PLABLA_FLOCK_OPENFFLAG, 0) ;
  if (gLastLocked == -1) 
    die("hlr_fileLock: cannot open: %s: %s", strerror(errno), filename) ;
  
  if (PLABLA_FLOCK(gLastLocked) == -1) 
    die("hlr_fileLock: flock failed: %s",strerror(errno)) ;

  return gLastLocked;
}

int hlr_fileLockNB(char *filename)
{ /*
  same as hlr_fileLock(), but NB (non-blocking).

  input:         like for hlr_fileLock()
  returns:       lockID (>=0) if lock could be obtained;
                 -1 if file is already locked by another process
  postcondition: like for hlr_fileLock() if lock was obtained;
                 If lock was not obtained, hlr_fileUnlock(-1) is illegal.
  notes: 
   see hlr_fileLock()
  */
  
  int status;
  
  gLastLocked = open(filename, PLABLA_FLOCK_OPENFFLAG, 0) ;
  
  if (gLastLocked == -1) 
    die("hlr_fileLockNB: cannot open: %s: %s", strerror(errno), filename) ;
  
  if ((status=PLABLA_FLOCKNB(gLastLocked)) == -1) {
    if (errno!=PLABLA_ISLOCKED)
      die("hlr_fileLockNB: flock failed : %s",strerror(errno)) ;
    close(gLastLocked) ;
    gLastLocked = -1 ;
  }
  else {
    hlr_assert(status==0, "hlr_fileLockNB") ;
  }
  return gLastLocked;
}


void hlr_fileUnlock(int lockID)
{ /*
  release lock identifed by lockID. If some other
  process was waiting on this lock, this other process will
  continue running.
  input:    lockID -- ID of lock obtained from hlr_fileLock() or hlr_fileLockNB()
                      or -1 to release the last lock obtained (see example in hlr_fileLock())
  precondition: successful call to hlr_fileLock() or 
                hlr_fileLockNB() not having returned -1
  */
  if (lockID == -1) {
    if (gLastLocked==-1)
      die("hlr_fileUnlock: the last locked file is not defined") ;
    lockID=gLastLocked;
  }
  
  if (PLABLA_FUNLOCK(lockID) == -1) 
    die("hlr_fileUnlock: unlock failed: %s",strerror(errno)) ;
  close(lockID) ;
  gLastLocked=-1;
}
 
char *hlr_fileRead(char *filename, int isText, int *len)
{ /*
  Read file input main memory as fast as possible 
  input: filename --
         isText -- if 1 '\0' is appended to return string, else 0.
         len -- can be NULL
  output: *len -- if not NULL, contains length of file
  returns: pointer to memory block containing file contents.
           Memory block has size '*len' if isText==0, else one byte more (for the '\0').
           NULL if file could not be read. 
           Memory returned belongs to caller -- call hlr_free() after use.
  postcondition: if NULL is returned, warnReport() tells the reason for failure
  limitations: Only for files up to 2GB size.
               Cannot be used if the file is simultanously written to while reading.
  */
  size_t size ;
  struct stat filestatus ;
  FILE *f ;
  int fd;
  char *contents ;

  if (!(f = fopen(filename, "r"))) {
    warnAdd("hlr_fileRead()", stringPrintBuf("%s: %s", strerror(errno), filename)) ;
    return NULL;
  }

  fd = fileno(f) ;
  if (fstat(fd, &filestatus)) {
    warnAdd("hlr_fileRead()", "fstat failed") ;
    fclose(f) ;
    return NULL;
  }

  unsigned long fsize_limit = 2<<31 -1;
  if (filestatus.st_size > fsize_limit) {
    warnAdd("hlr_fileRead()", "file size is limited to 2GB") ;
    fclose(f) ;
    return NULL;
  }
  contents = hlr_malloc(filestatus.st_size + (isText ? 1 : 0)) ;
  size = fread(contents, 1, filestatus.st_size, f) ;
  fclose(f) ;
  hlr_assert(size == filestatus.st_size, "hlr_fileRead(): size mismatch. Simultanous writer active?") ;
  if (isText)
    contents[filestatus.st_size] = '\0' ;
  if (len)
    *len = filestatus.st_size ;
  return contents ;
}

int hlr_fileEqual(char *fn1, char *fn2)
{ /* 
  returns 1 if file fn1 and fn2 have identical contents,
  else 0
  */
  FILE *f1 ;
  FILE *f2 ;
  int c1 = 0 ;
  int c2 = 0 ;
  int isEqual ;
  if (! (f1=fopen(fn1, "rb")))
    die("fileEqual: %s: %s", fn1, strerror(errno)) ;
  if (! (f2=fopen(fn2, "rb")))
    die("fileEqual: %s: %s", fn2, strerror(errno)) ;
  while (c1 == c2 && c1 != EOF) {
    c1 = getc(f1) ;
    c2 = getc(f2) ;
  }
  isEqual = (c1 == EOF && c2 == EOF) ;
  fclose(f1) ;
  fclose(f2) ;
  return isEqual ;
}

char *hlr_fileRemove(char *fn)
{/*
  remove a file or empty directory
  input: fn -- name of file or directory to be removed
  output: -
  returns: NULL if ok, else an appropriate error message
           memory pointed to is read-only
  */
  if (remove(fn) != 0)
    return strerror(errno) ;
  return NULL ;
}

char *hlr_fileCopy(char *fn1, char *fn2)
{ /*
  copy a file
  input: fn1 -- source filename
         fn2 -- target filename
  output: -
  returns: NULL if ok, else an appropriate error message
           memory pointed to is read-only
  note: an existing file fn2 is overwritten when permissions allow it;
        the new file fn2 receives new creation/modification times;
	the referred file will be copied instead of a symbolic link;
	directories cannot be copied this way
  note: the current implementation might be ineffcient for very large
        files. this can be optimized if necessary.
  */

  char *result = NULL ;
  static char msg[32] ;
  FILE *f1 = ILLADR ;
  FILE *f2 = ILLADR ;
  int byte = 0 ;

  if (f1 = fopen(fn1, "rb")) {
    if (f2 = fopen(fn2, "wb")) {
      while ((byte = getc(f1)) != EOF)
	putc(byte, f2) ;

      fclose(f2) ;
    }
    else {
      sprintf(msg, "file cannot be written") ;
      result = msg ;
    }

    fclose(f1) ;
  }
  else {
    sprintf(msg, "file cannot be read") ;
    result = msg ;
  }

  return result ;
}

char *hlr_fileMove(char *fn1, char *fn2)
{ /*
  move/rename a file
  input: fn1 -- source filename
         fn2 -- target filename
  output: -
  returns: NULL if ok, else an appropriate error message
           memory pointed to is read-only
  */

  char *msg;

  if ((msg = hlr_fileCopy(fn1, fn2)) == NULL)
    msg = hlr_fileRemove(fn1);

  return msg ;
}

char *hlr_mkDir(char *dirname)
{ /*
  create a directory
  input: dirname -- name of directory to be created
  output: -
  returns: NULL if ok, else an appropriate error message
           memory pointed to is read-only
  */

  /* default mode for directory creation: 775 */
  mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH ;

  if (mkdir(dirname, mode) != 0)
    return strerror(errno) ;
  return NULL ;
}

char *hlr_getHomeDir(char *username)
{ /*
  get the home directory of username  
  input  : username
  returns: home directory path, NULL if username does not exist
           the memory returned is read-only
           stable until the next call to this function
  output : 
  precondition: 
  postcondition:
  */
  
  struct passwd *pass;
  if (username == NULL)
     die("hlr_getHomeDir(NULL)");
  pass = getpwnam(username);
  if (pass == NULL) 
     return NULL;
  return pass->pw_dir;
}

/*
  1999-09-27  klenka: this variable is used for communication between functions
                      hlr_dirScanInit() and dirScanSelect() during execution of
		      hlr_dirScanInit() -- this implementation is NOT
		      multi-thread safe!
 */
static char *gDirScanFilter = NULL ;

#if BIOS_PLATFORM == BIOS_PLATFORM_IRIX || BIOS_PLATFORM == BIOS_PLATFORM_SOLARIS
static int dirScanSelect(struct dirent *de) {
  return (fnmatch(gDirScanFilter, de->d_name, 0) == 0);
}
#endif

#if BIOS_PLATFORM == BIOS_PLATFORM_LINUX
static int dirScanSelect(const struct dirent *de) {
  return (fnmatch(gDirScanFilter, de->d_name, 0) == 0);
}
#endif

static void updatePath(DirScanIter ds) {
  if (ds->indexPath != ds->index) {
    ds->indexPath = ds->index ;

    if (strEqual(ds->dir, "/"))
      stringPrintf(ds->fullPath, "/%s", ds->file) ;
    else
      stringPrintf(ds->fullPath, "%s/%s", ds->dir, ds->file) ;
  }
}

static void updateStat(DirScanIter ds) {
  if (ds->index != ds->indexStat) {
    updatePath(ds) ;
    ds->indexStat = ds->index ;

    if (PLABLA_LSTAT(string(ds->fullPath), &ds->s)) {
      perror(string(ds->fullPath)) ;
      warn("file %s not accessible", string(ds->fullPath)) ;
    }
  }
}

DirScanIter hlr_dirScanInit(char *dirname, char *filter) 
{ /*
  initialize a scan over files in directory dirname matching expression filter
  input: dirname -- name of directory to be scanned, must not be NULL, 
                    must not have a trailing path separator ('/' or '\' 
                    depending on platform) except for the root directory
         filter -- scan only for files matching this pattern, may contain
                   wildcard characters '?' and '*', if NULL all files
  output: -
  returns: an iterator which must be used as an argument to hlr_dirScanNext()
           and other hlr_dirScanXyz() functions or NULL if error
	   to release temporary resources call hlr_dirScanBreak() 
           if iterator is not read to its end 
           (i.e. until hlr_dirScanNext() returns NULL)
  postcondition: hlr_dirScanNext() etc. can be called
  */

  int numEntries = 0 ;
  struct dirent **namelist = ILLADR ;
  DirScanIter ds = NULL ;

  gDirScanFilter = filter ;

#if BIOS_PLATFORM == BIOS_PLATFORM_IRIX || BIOS_PLATFORM == BIOS_PLATFORM_LINUX
  if ((numEntries = scandir(dirname, 
			    &namelist, 
			    filter ? &dirScanSelect : NULL, 
			    &alphasort)) != -1)
#endif
#if BIOS_PLATFORM == BIOS_PLATFORM_SOLARIS
  if ((numEntries = scandir_BSD(dirname, 
			    &namelist, 
			    filter)) != -1)
#endif
  {
    ds = (DirScanIter) hlr_malloc(sizeof(struct DirScanIterStruct));
    ds->numEntries = numEntries ;
    ds->index = -1 ;
    ds->namelist = namelist ;
    ds->dir = hlr_strdup(dirname) ;
    ds->file = NULL ;
    ds->fullPath = stringCreate(256) ;
    ds->indexPath = -1 ;
    ds->indexStat = -1 ;
  }
  return ds ;
}

char *hlr_dirScanNext(DirScanIter ds) 
{ /*
  retrieve next entry from current directory scan; the
  exact order is undefined; if it looks alphabetically this is
  by chance and should not be taken for granted.
  input: ds -- directory scan iterator received from hlr_directoryScanInit()
  output: ds -- updated
  returns: name of next file (, directory, link) from scanned directory
           or NULL if no more filenames
	   only valid until this function returns NULL because iterator is
	   automatically destroyed after final iteration
  precondition: successful call to hlr_dirScanInit()
  postcondition: next call to this function will return next entry;
                 file properties can be queried using other hlr_dirScanXyz() 
                 functions;
                 hlr_dirScanBreak() must be called if scan is cancelled 
                 before this function returned NULL;
		 no other hlr_dirScanXyz() function must be called for this ds
		 after this function has returned NULL
  */

  char *result = NULL ;

  if (++ ds->index < ds->numEntries)
    result = ds->file = ds->namelist[ds->index]->d_name ;
  else
    hlr_dirScanDestroy_func(ds) ;

  return result ;
}

/*
  call this macro if a directory scan is cancelled before hlr_dirScanNext()
  has returned NULL to release temporary resources

  #define hlr_dirScanBreak(this) (hlr_dirScanDestroy_func(this), this=NULL)
*/

void hlr_dirScanDestroy_func(DirScanIter ds) 
{ /*
  destroy directory scan iterator and free resources
  input: ds -- directory scan iterator received from hlr_directoryScanInit()
  precondition: successful call to hlr_dirScanInit()
  postcondition: ds is invalidated, do not call any hlr_dirScanXyz() function 
                 any more for this ds
  note: do not call this function explicitly - use hlr_dirScanBreak() if the scan
        is cancelled before its end. otherwise this function is called automatically
	when the last item has been read.
  */

  for (ds->index=0;ds->index<ds->numEntries;ds->index++)
    free(ds->namelist[ds->index]) ;
#if BIOS_PLATFORM == BIOS_PLATFORM_SOLARIS
  free(ds->namelist);
#endif
  hlr_free(ds->dir) ;
  ds->file = NULL ;
  stringDestroy(ds->fullPath) ;
  hlr_free(ds) ;
}

int hlr_dirScanIsDir(DirScanIter ds) 
{ /*
  find out if current file is a directory
  input: ds -- directory scan iterator received from hlr_directoryScanInit()
  output: -
  returns: 1 if file is directory, 0 else
  precondition: successful call to hlr_dirScanNext()
  */

  updateStat(ds) ;
  return S_ISDIR(ds->s.st_mode) ;
}

char *hlr_dirScanFullPathGet(DirScanIter ds) 
{ /*
  retrieve full path for current file
  input: ds -- directory scan iterator received from hlr_directoryScanInit()
  output: -
  returns: the full path of the last file returned by hlr_dirScanNext()
  precondition: successful call to hlr_dirScanNext()
  */

  updatePath(ds) ;
  return string(ds->fullPath) ;
}

intgr8 hlr_dirScanFileSizeGet(DirScanIter ds) 
{ /*
  retrieve file size of current file
  input: ds -- directory scan iterator received from hlr_directoryScanInit()
  output: -
  returns: the size of the last file returned by hlr_dirScanNext()
  precondition: successful call to hlr_dirScanNext()
  */

  updateStat(ds) ;
  return ds->s.st_size ;
}

int hlr_dirScanLastUpdateGet(DirScanIter ds) 
{ /*
  retrieve last modification date and time (seconds since 01.01.1970) of current file
  input: ds -- directory scan iterator received from hlr_directoryScanInit()
  output: -
  returns: the last modification date and time of the last file returned by hlr_dirScanNext()
  precondition: successful call to hlr_dirScanNext()
  */

  updateStat(ds) ;
  return ds->s.st_mtime ;
}

int hlr_dirScanLastAccessGet(DirScanIter ds) 
{ /*
  retrieve last access date and time (seconds since 01.01.1970) of current file
  input: ds -- directory scan iterator received from hlr_directoryScanInit()
  output: -
  returns: the last access date and time of the last file returned by hlr_dirScanNext()
  precondition: successful call to hlr_dirScanNext()
  */

  updateStat(ds) ;
  return ds->s.st_atime ;
}

/*
  more hlr_dirScanXyz() functions for querying file properties can be added if needed.
  contact axel.klenk@roche.com or detlef.wolf@roche.com
 */

intgr8 hlr_fileSizeGet(char *filename)
{
  /* returns the size in bytes of the file given as argument
     input: filename - the complete path to the file
     returns: the size of the file
  */
  struct PLABLA_STAT s;

  PLABLA_STAT(filename, &s);
  return s.st_size;
}

int hlr_fileLastUpdateGet (char *fileName)
{
  /* returns the last update time (seconds since 1970) of the file
` */
  struct PLABLA_STAT s;

  PLABLA_STAT (fileName,&s);
  return s.st_mtime;
}

char *hlr_getLink(char *filename)
{
  /*
  returns symbolic link info
  input: filename -- name of a file, must not be NULL
  returns: full path of file or dir pointed to if filename is a symbolic link,
           NULL if filename is not a symbolic link or an error occured
  */
  static char buf[MAXPATHLEN+1];
  if(readlink(filename, buf, MAXPATHLEN+1) == -1)
    return NULL ;
  return buf ;
}

#if BIOS_PLATFORM == BIOS_PLATFORM_SOLARIS

static struct dirent * hlr_dirScanCloneDirent(struct dirent *in)
{
  struct dirent *o;
  if (in == NULL)
    return NULL;
  o = (struct dirent *)malloc(in->d_reclen);
  memcpy(o,in,in->d_reclen);
  return o;
}

static int hlr_dirScan_alphasort(struct dirent **d1,struct dirent **d2)
{
  return strcmp((*d1)->d_name,(*d2)->d_name);
}

int scandir_BSD(char *dirname,struct dirent ***in_names,char *filter)
{ /*
  This is the sun version of scandir from BSD (which exists on SGI).
  */
  long int loc;
  DIR *dirp;
  int count = 0;
  int x = 0;
  struct dirent **names;
  struct dirent *out;
  dirp = opendir(dirname);
  if (dirp == NULL) {
    return -1;
  }
  loc = telldir(dirp);
  while (out = readdir(dirp)) {
    if (filter == NULL || dirScanSelect(out))
      ++count;
  }
  seekdir(dirp,loc);
  names = (struct dirent **)malloc(sizeof(struct dirent) * count);
  while (out = readdir(dirp)) {
    if (filter == NULL || dirScanSelect(out)) {
      if (out != NULL)
	names[x] = hlr_dirScanCloneDirent(out);
      ++x;
    }
  }
  qsort(names,x,sizeof(struct dirent *),(int (*)(const void *, const void *)) hlr_dirScan_alphasort);
  *in_names = names;
  closedir(dirp);
  return x;
}

#endif

/*--------------------------------------------DirectoryObject functions */

DirectoryObject dio_create(char *path, char *mask)
{ /*
  input: path -- a directory name
         mask -- filter for file names to import;
                 wildcards allowed ( [], ?, * );
                 NULL means 'all files except for the
                 ones whose name starts with a dot.
  returns: new directory object
           use dio_destroy() after use
  */
  DirScanIter ds;
  char *info;
  Stringa hold;
  int i;
  DirectoryObject theo;
  theo = malloc(sizeof(struct _dirInfo));
  theo->items = textCreate(10);
  theo->root = stringCreate(10);
  theo->place = -1;
  theo->iterator = NULL;
  theo->files = arrayCreate(10,char *);
  theo->dirs = arrayCreate(10,char *);
  hold = stringCreate(20);
  i = strlen(path) -1;
  if (path[i] == '/')
    path[i] = '\0';
  stringCpy(theo->root,path);
  ds = hlr_dirScanInit(path,mask);
  if (ds)
    while ((info = hlr_dirScanNext(ds))) {
      if (!mask && info[0] == '.')
	continue;
      stringPrintf(hold,"%s/%s",string(theo->root),info);
      i = arrayMax(theo->items);
      array(theo->items,arrayMax(theo->items),char *) = hlr_strdup(info);
      if (hlr_queryDir(string(hold)))
	array(theo->files,arrayMax(theo->files),char *) = array(theo->items,i,char *);
      else
	array(theo->dirs,arrayMax(theo->dirs),char *) = array(theo->items,i,char *);
    }
  stringDestroy(hold);
  return theo;
}

void dio_iterInit(DirectoryObject theo,int iter_type)
{ /*
  input: theo -- created by dio_create()
         iter_type -- DIO_FILES_ONLY / DIO_DIRECTORIES_ONLY
                      / DIO_FILES_AND_DIRECTORIES
  */
  if (iter_type == DIO_FILES_ONLY)
    theo->iterator = theo->files;
  else if (iter_type == DIO_DIRECTORIES_ONLY)
    theo->iterator = theo->dirs;
  else if (iter_type == DIO_FILES_AND_DIRECTORIES)
    theo->iterator = theo->items;
  theo->place = 0;
}

char *dio_getNext(DirectoryObject theo)
{ /*
  precondition: dio_iterInit(theo) or dio_getNext(theo)
  postcondition: next call to this function will return next
                 file name from directory
  returns: file name including path; memory managed by this
           routine; read only to user of this routine
  */
  if (theo->place == arrayMax(theo->iterator) || theo->place == -1) 
    return NULL;
  else
    return array(theo->iterator,theo->place++,char *);
}

/*
#define dio_destroy(theo) (dio_destroy_func(theo),theo=NULL)
*/

void dio_destroy_func(DirectoryObject theo)
{ /*
  do not use this function in your program, use dio_destroy()
  */
  if (!theo)
    return;
  arrayDestroy(theo->dirs);
  arrayDestroy(theo->files);
  stringDestroy(theo->root);
  textDestroy(theo->items);
  free(theo);
}


/* ----------------------- begin module hlr_ipcf ---------------------- */
/* 
purpose: transport a string from one process to another via temporary file
         (ipcf = inter process communication via file)
usage: in first process:
         id = hlr_ipcfSet("hello!") ;
       then "somehow" (e.g. as command line argument), pass id to second process.
       in second process:
         char *s = hlr_ipcfGetOnce(id) ;
restrictions: one process can only pass one string. The string can
              be arbitrarily long, but cannot contain '\0'.
*/

static char *gIpcFileNameTemplate = "/tmp/hlr_ipcf_%d.tmp" ;
static int gIpcfCnt = 0 ;

int hlr_ipcfSet(char *s) 
{ /*
  set the string to be communicated
  returns: persistent id assigned to input 's'
  postcondition: hlr_ipcfGetOnce(id) will return 's', even if
                 call happens in a later process
  */
  Stringa fn = stringCreate(40) ;
  FILE *f ;
  int id = (getpid() * (1 << 8)) + ++gIpcfCnt ;
  stringPrintf(fn, gIpcFileNameTemplate, id) ;
  if (f = fopen(string(fn), "wb")) {
    fputs(s, f) ;
    fclose(f) ;
  }
  else
    die("hlr_ipcfSet: cannot open file %s", string(fn)) ;
  stringDestroy(fn) ;
  return id ;
}

char *hlr_ipcfGetOnce(int id) 
{ /*
  receive the string set by hlr_ipcfSet() and free the communication resource
  returns: message -- memory managed by this routine; user may read & write,
           but not free() or realloc();
           if id==0 or there was no hlr_ipcfSet() or some problem occured, 
           an empty string is returned.
  postcondition: next call to hlr_ipcfGetOnce() will return "" (emtpy string).
  */
  Stringa fn = stringCreate(40) ;
  FILE *f ;
  char *problem ;
  int byte ;
  static Stringa msg = NULL ;
  stringCreateClear(msg, 40) ;
  
  if (!id)
    return string(msg) ;

  stringPrintf(fn, gIpcFileNameTemplate, id) ;
  if (f = fopen(string(fn), "rb")) {
    arrayClear(msg) ;                         /* not a Stringa anymore */
    while ((byte = getc(f)) != EOF)
      array(msg, arrayMax(msg), char) = byte ;
    array(msg, arrayMax(msg), char) = '\0' ;  /* convert back into a Stringa */
    fclose(f) ;
    if (problem = hlr_fileRemove(string(fn)))
      stringInsert(msg, 0, problem) ;
  }
  /* if there was no hlr_ipcfSet() or some other problem, return an empty string */

  stringDestroy(fn) ;
  return string(msg) ;
}

/* ----------------------- end module hlr_pers ---------------------- */
