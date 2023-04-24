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
/** @file rdbu.c
    @brief Relational database utilites, handling database login info.
    Module prefix rdbu_
*/
#include "log.h"
#include "format.h"
#include "arg.h"
#include "rdbu.h"

static Stringa user = NULL;
static Stringa password = NULL;
static Stringa database = NULL;

static void readUserInfoFile (char *filename) {
  char line[200];
  char *u;
  char *p;
  char *db;
  char *slashp;
  char *atp;
  char *cp;
  char *fgetsRes;

  FILE *f = fopen (filename,"r");
  if (f == NULL)
    return;
  fgetsRes = fgets (line,sizeof (line),f);
  if (fgetsRes == NULL) {
	  error("fgets error.");
  }
  fclose (f);
  // user/password@instance
  // - in each field: unknown
  cp = strchr(line, '\n');
  if (*cp != '\0')
    *cp = '\0';
  slashp = strchr (line,'/');
  atp = strchr (line,'@');
  if (slashp == line || slashp == NULL || slashp >= atp-1 ||
      atp == NULL || *(atp+1) == '\0') {
    warn ("cannot understand contents of file %s. ignoring contents.",
          filename);
    return;
  }
  u = line;
  p = slashp+1;
  db = atp+1;
  *slashp = '\0';
  *atp = '\0';
  if (strDiffer (u,"-"))
    stringCpy (user,u);
  if (strDiffer (p,"-"))
    stringCpy (password,p);
  if (strDiffer (db,"-"))
    stringCpy (database,db);
}

void rdbu_initLoginInfo (char *filename) {
  /**
     Looks in the following places for user/password\@database
     info needed for logging into database in this order
     1. a file named .login_db in the user's home directory
     2. a file named .login_db in the current directory
     3. a file named 'filename' (optional input parameter)
     4. arguments 'dbuser', 'dbpassword', 'dbname' present  the command line
     Syntax of the files:
     - only the first line is read
     - this line must have the form user/password\@dbname
     - each of these fields can be '-'; in this case the value
       of this field is not changed. E.g. if $HOME/.login_db contains
       -/-\@testdb
      and 'filename' contains
      scott/-@-
      and on the command line there is an argument
      -dbpassword tiger
      then username will be scott, password will be tiger and
      database will be testdb
      optional: if arg_init() was called before then the command line will be
                considered; else the command line is ignored
      Postcondition: rdbu_user() etc can be called
  */
  Stringa fn;
  char *home = getenv ("HOME");
  if (user != NULL)
    die ("rdbu_getLoginInfo() twice");
  user = stringCreate(10);
  password = stringCreate(10);
  database = stringCreate(10);
  if (home != NULL) {
    fn = stringCreate (100);
    stringPrintf (fn,"%s/.login_db",home);
    readUserInfoFile (string (fn));
    stringDestroy (fn);
  }
  readUserInfoFile (".login_db");
  if (filename != NULL && *filename != '\0')
    readUserInfoFile (filename);
  if (arg_isInit ()) {
    if (arg_present ("dbuser"))
      stringCpy (user,arg_get ("dbuser"));
    if (arg_present ("dbpassword"))
      stringCpy (password,arg_get ("dbpassword"));
    if (arg_present ("dbname"))
      stringCpy (database,arg_get ("dbname"));
  }
}

char *rdbu_user (void) {
  /**
     Retrieves the user logged in
  */
  if (user == NULL)
    die ("rdbu_initLoginInfo() missing");
  return string (user);
}

char *rdbu_password (void) {
  /**
     Retrieves the password of the user
  */
  if (password == NULL)
    die ("rdbu_initLoginInfo() missing");
  return string (password);
}

char *rdbu_database (void) {
  /**
     Retrieves the database name
  */
  if (database == NULL)
    die ("rdbu_initLoginInfo() missing");
  return string (database);
}

void rdbu_loginFunc (char* (*rdbr_login)(char *server,char *dbname,
                                         char *user,char *passwd)) {
  /**
     Tries to login into the database based on the procedure described
     in rdbu_initLoginInfo(). In addition, if there is a command line
     argument "-dbfile filename", then parameters are also looked for
     in 'filename'.
     If the login fails, there is a terse message 'PROBLEM: oracle logon
     failed' on order not to show the password to the user of the program.
     At debugging time however it might be desirable to know the login
     parameters. In this case establish the environment variable 'RDBU_DEBUG'
     to get a detailed message.<br>
     Postcondition: rdbr_logout() and other rdbr_ routines can be called.<br>
     Note: do not invoke this function in your program; use rdbu_login()
     instead
  */
  /*
    implementation:
    why did we use the function pointer? :
    because we wanted to keep this module independent from linking with
    ORACLE or PostgreSQL; so rdbr.o can go into libkern.a
  */
  char *msg;
  char *filename = NULL;
  static int first = 1;
  if (first) {
    if (arg_isInit () && arg_present ("dbfile"))
      filename = arg_get ("dbfile");
    rdbu_initLoginInfo (filename);
    first = 0;
  }
  if ((msg = (*rdbr_login)(rdbu_database (),NULL,
                           rdbu_user (),rdbu_password ())) != NULL) {
    warn (msg);
    if (getenv ("RDBU_DEBUG") != NULL)
      romsg ("rdbu_login failed for %s/%s@%s",rdbu_user (),rdbu_password (),
             rdbu_database ());
    return;
  }
}



