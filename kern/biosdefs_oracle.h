#ifndef _ORAENV_H_
#define _ORAENV_H_
/* 
file: biosdefs_oracle.h
this file is read by the C preprocess and by the 'scriptconf' utility
(part of bios/base/utils/).
*/

#define ORACLE_HOME   "/opt/oracle/client/10/run_1"
#define ORA_NLS33     "/opt/oracle/client/10/run_1/nls/data"
#define NLS_LANG "AMERICAN_AMERICA.WE8ISO8859P1"

/* only for bios/exa/rdb_oracle/src/rdbrtest.c : */
#define PRPIDBMSTEST_DBNAME "bia"      /* TWO_TASK */
#define PRPIDBMSTEST_SERVER NULL       /* ORACLE_SID */
#define PRPIDBMSTEST_USER "bid"
#define PRPIDBMSTEST_PASSWORD "xxxxxxx"
#define PRPIDBMSTEST_PASSWORD_ENCODED " \001\000\xxxxxxxxxxxxx"

#endif 
