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

#ifndef DEF_rofutil_H
#define DEF_rofutil_H
#include <sys/stat.h>
#include "plabla.h"
#include "format.h"

/* -----------------------------------------------------------
   do not refer to anything inside these structs from your programs
   */

typedef struct DirScanIterStruct {
  int numEntries ;
  int index ;
  struct dirent **namelist ;
  char *dir ;
  char *file ;
  int indexPath ;
  Stringa fullPath ;
  int indexStat ;
  struct PLABLA_STAT s ;
} *DirScanIter ;


typedef struct _dirInfo{
  Texta items;   /*   of char * */
  Array dirs;   /* char *;indices of directories */
  Array files;   /* char *;indices of files */
  Array iterator; /* stores type of iteration; points to dirs, files or items */
  Stringa root; /* root path */
  int place; /* place holder for last access */
}*DirectoryObject;

/* ------------------------------------- public functions */

extern DirScanIter hlr_dirScanInit(char *dirname, char *filter) ;
extern char *hlr_dirScanNext(DirScanIter ds) ;
#define hlr_dirScanBreak(this) (hlr_dirScanDestroy_func(this), this=NULL)
extern int hlr_dirScanIsDir(DirScanIter ds) ;
extern char *hlr_dirScanFullPathGet(DirScanIter ds) ;
extern intgr8 hlr_dirScanFileSizeGet(DirScanIter ds) ;
extern int hlr_dirScanLastAccessGet(DirScanIter ds) ;
extern int hlr_dirScanLastUpdateGet(DirScanIter ds) ;

extern char* hlr_queryPlainFile(char *filename) ;
extern char* hlr_queryDir(char *dirname) ;
extern char *hlr_tail(char *fileNameWithPath) ; /* :t modifier from csh */
extern char *hlr_getDir(char *fileNameWithPath) ; /* opposite of hlr_tail() */
extern FILE *hlr_fopenRead(char *filename) ;
extern FILE *hlr_fopenWrite(char *filename) ;
extern FILE *hlr_fopenAppend(char *filename) ;
extern char *hlr_getUser(void) ;
extern int hlr_isAbsolutePath(char *path);
extern char *hlr_extension(char *filename);
extern int hlr_fileLock(char *filename) ;
extern int hlr_fileLockNB(char *filename) ;
extern void hlr_fileUnlock(int fildes) ;

extern char *hlr_fileRead(char *filename, int isText, int *len);
extern int hlr_fileEqual(char *fn1, char *fn2) ;
extern char *hlr_fileMove(char *fn1, char *fn2) ;
extern char *hlr_fileCopy(char *fn1, char *fn2) ;
extern char *hlr_fileRemove(char *fn) ;
extern char *hlr_mkDir(char *dirname) ;
extern char *hlr_getHomeDir(char *username) ;
extern intgr8 hlr_fileSizeGet (char *filename) ;
extern int hlr_fileLastUpdateGet (char *fileName);
extern char *hlr_getLink(char *filename) ;

extern void hlr_setCoredumpsize(int size) ;

/* ----------------------- begin module hlr_ipcf ---------------------- */
/* (ipcf = inter process communication via file) */
extern int hlr_ipcfSet(char *s) ;
extern char *hlr_ipcfGetOnce(int id) ;

/*------------------------------------- DirectoryObject functions */
extern DirectoryObject dio_create(char *path,char *mask);
extern void dio_iterInit(DirectoryObject theo,int iter_type);
extern char * dio_getNext(DirectoryObject theo);
#define DIO_FILES_ONLY 1
#define DIO_DIRECTORIES_ONLY 2
#define DIO_FILES_AND_DIRECTORIES 3

/* do not call the following in your programs */
/* private */ extern void dio_destroy_func(DirectoryObject theo);
#define dio_destroy(theo) (dio_destroy_func(theo),theo=NULL)

/* ------------------------------------- private functions */
/* do not call in your programs: */
/* private */ extern void hlr_dirScanDestroy_func(DirScanIter ds) ;


#endif
