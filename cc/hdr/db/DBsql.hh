#ifndef __DBSQL_H
#define __DBSQL_H

/*
**	File ID: 	%Z%: <%M% (%Q%) - %G%, %I%>
**
**	File:			%M%
**	Release:		%I%
**	Date:			%H%
**	Time:			%T%
**	Newest applied delta:	%G% %U%
**
** FILE NAME: DBsql.hh
**
** PATH: cc/hdr/db
**
** DESCRIPTION:
**	This file contains the prototypes of all functions with embedded
**	SQL code.
**
** NOTES:
**	Any file that calls such functions will need to include this file.
**
** OWNER:
**	eDB Team
**
** HISTORY:
**	06/04/03 - Original version by Hon-Wing Cheng for Feature 61286
*/

#ifdef __linux
/* IBM wyoes 20060705 added luc_compat.h for wrappers */
//#include "luc_compat.h"
#endif

#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHqid.hh"
#include "cc/hdr/db/DBretval.hh"

#define DB_SQLSTATE_LEN   5
#define DB_SQLCLASS_LEN   2
#define DB_SQLERROR_LEN  80

extern Char *sqlstate;
extern int  DBsqlCodeSave;
extern Char DBsqlStateSave[];
extern Char DBsqlErrorSave[];
extern Char DBsqlCmd[];

#define  DBSQLINIT  DBsqlCodeSave = 0;          \
  strcpy(DBsqlStateSave, "00000");              \
  DBsqlErrorSave[0] = '\0';

#define  DBSQLSAVE  DBsqlCodeSave = sqlca.sqlcode;                      \
  strlcpy(DBsqlStateSave, sqlstate, DB_SQLSTATE_LEN + 1);               \
  strlcpy(DBsqlErrorSave, sqlca.sqlerrm.sqlerrmc, sqlca.sqlerrm.sqlerrml + 1);

DBRETVAL DBconnect(const Char *login = "", const Char *passwd = "");
DBRETVAL DBdisconnect(Void);
DBRETVAL DBselectUpdate(const Char *tblName, const Char *where);
DBRETVAL DBlockTable(const Char *tblName, const Char *mode = "ROW EXCLUSIVE");
// EXEC SQL <lock_table_command>
DBRETVAL DBexecSql(const Char *cmd, Bool commit = TRUE);
// EXEC SQL EXECUTE IMMEDIATAE
DBRETVAL DBselectInt(const Char *stmt, int &value, Bool commit = TRUE);
// One integer
DBRETVAL DBselectStr(const Char *stmt, int length, Char *buffer, Bool commit = TRUE);
// One string
DBRETVAL DBselectColumnNames(const Char *tblName, Char *colNames);
DBRETVAL DBcommit();                  // EXEC SQL COMMIT WORK

Long  DBsqlErrorCode();               // sqlca.sqlcode
const Char *DBsqlErrorMsg();          // sqlca.sqlerrm.sqlerrmc
const Char *DBsqlState();             // SQLSTATE (five characters)
const Char *DBsqlClass();             // first two characters of SQLSTATE
Long  DBsqlNumRows();                 // sqlca.sqlerrd[2]

//
// NON_PORTABLE DATABASE INTERFACE
//
// This function is only supported on PostgreSQL.
//
Void DBsetAutoCommit(Bool commit);    // EXEC SQL SET AUTOCOMMIT TO ON/OFF

#endif
