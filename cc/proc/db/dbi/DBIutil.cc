/*
**	File ID: 	@(#): <MID67393 () - 12/13/03, 2.1.1.1>
**
**	File:			MID67393
**	Release:		2.1.1.1
**	Date:		12/15/03
**	Time:		11:26:33
**	Newest applied delta:	12/13/03 15:12:10
**
** DESCRIPTION:
**	Put some util function here that can be used by both
**	DBI and DBI helpers. The function DBIgetTableName() here
**	is moved from former DBIgetToken() from DBI.C
**
** OWNER:
**	eDB team
**
** History
**	Lucy Liang(11/21/03):  Add for feature 61284
**
** NOTES:
*/


#include "hdr/GLtypes.h"
#include "cc/hdr/db/DBsqlMsg.hh"
#include "cc/hdr/db/DBlimits.hh"
#include "cc/hdr/db/DBsql.hh"
#include "cc/hdr/db/DBretval.hh"
//#include "cc/hdr/db/DBdebug.hh"
//#include "cc/hdr/cr/CRdebugMsg.hh"

#include "hdr/mydebug.h"
#include <string.h>

// get table name from a sql statement
Long 
DBIgetTableName(DBsqlMsg *sqlp, Char *tableName)
{
	Short i, j;
	Long sqlLen = sqlp->sqlSz;
	Char *sql = sqlp->sql;
	Char ch;

	i = 0;
	if (sqlp->tblOffset >= sqlLen)
	{
	    CRERROR("DBIgetTableName(): Wrong table name offset %u. "
	            "SQL statement length = %d", sqlp->tblOffset, sqlLen);
	    return DBFAILURE;
	}

	// get rid of leading blanks 
	for (i = sqlp->tblOffset; i < sqlLen && sql[i] == ' '; i++)
		;

	if (i == sqlLen)
	{
	    CRERROR("DBIgetTableName(): Blank table name in SQL statement(%s)",
	            sql);
	    return DBFAILURE;
	}

	ch = sql[i];
	if (isalpha(ch))
	{
		Char c;
		// an identifier: may containn a-z, A-Z, 0-9, _, $ and # 
		j = 0;
		tableName[j++] = sql[i++];

		for (c = sql[i];
		     (j < DB_TBL_NAME_LEN * 2 + 2) /* include schema */ &&
		     (isalnum(c) || (c == '_') || (c == '$') || (c == '#') || (c == '.'));
		     /* no increment */
		    )
		{
		    tableName[j++] = sql[i++];
		    c = sql[i];
		}

		if (j > DB_TBL_NAME_LEN * 2 + 1)
		{
		    tableName[DB_TBL_NAME_LEN * 2 + 1] = '\0';
		    CRERROR("Schema+table(%s (truncated)) too long", tableName);
		    return(DBFAILURE);
		}

		tableName[j] = '\0';

		// Discount the prefix "<schema>." if there is one.
		Char *p;
		if ((p = strchr(tableName, '.')) != 0)
		{
		    p++;
		    j = strlen(p);
		    CRDEBUG(DBdbOper, ("schema+table(%s), table(%s) j(%d)",
		                       tableName, p, j));
		}
		else
		{
		    p = tableName;
		}

		if ((j <= DB_TBL_NAME_LEN) &&
		    (c == ' ' || c == '\t' || c == '\0'))
		{
		    return(DBSUCCESS);
		}
		else
		{
		    CRERROR("DBIgetTableName(): Table name(%s%c) too long "
		            "or contain invalid character", p, c);
		    return(DBFAILURE);
		}
	}
	else
	{
	    // encounter undesirable character
	    CRERROR("DBIgetTableName(): First character(%c) in table name "
                    "is invalid", ch);
	    return(DBFAILURE);
	}
}


