#ifndef __DBUTIL2_H
#define __DBUTIL2_H

/*
** FILE NAME: DButil2.H
**
** PATH: cc/hdr/db
**
** DESCRIPTION:
**
** NOTES:
**
** OWNER:
**	eDB Team
**
** HISTORY:
**	12/15/03 - Original version by Hon-Wing Cheng for Feature 61284
*/

#include "hdr/GLtypes.h"

enum DBdbms
{
   DB_PG,       // PostgreSQL
   DB_ORACLE,   // Oracle
   DB_TT,       // TimesTen
   DB_RTDB,     // Real-Time database
   DB_UNKNOWN
};

Bool DBisSpaTable(const Char *tblName, int &dbms);

#endif
