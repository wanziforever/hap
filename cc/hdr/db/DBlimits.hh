#ifndef __DBLIMITS_H
#define __DBLIMITS_H

/*
**	File ID: 	@(#): <MID66069 () - 06/11/03, 2.1.1.1>
**
**	File:			MID66069
**	Release:		2.1.1.1
**	Date:			07/15/03
**	Time:			11:08:35
**	Newest applied delta:	06/11/03 18:47:38
**
** FILE NAME: DBlimits.H
**
** PATH: cc/hdr/db
**
** DESCRIPTION:
**	This file defines some constants like length and max numbers
**	for all databases: PostgreSQL, Oracle, RTDB, and TimesTen
**
** NOTES:
**
** OWNER:
**	eDB Team
**
** HISTORY:
**	06/04/03 - Original version by Hon-Wing Cheng for Feature 61286.
*/

#define    DB_TBL_NAME_LEN      30    /* Max table name length         */
#define    DB_COL_NAME_LEN      30    /* Max column name length        */
#define    DB_USR_NAME_LEN      30    /* Max user name/password length */
#define    DB_COL_VALUE_LEN    255    /* Max column value Length       */
#define    DB_COL_NUMBER      1000    /* Max Column numbers in a table */
#define    DB_NAME_LEN          20    /* Max database name length      */
                                      /*  Posssible values are: ORACLE */
                                      /*   PLATDB, and TT              */
#define    DB_DSN_LEN           20    /* Max datastore name length     */
                                      /*   (for TimesTen only)         */
#define    DB_STATUS_LEN        10    /* Max database status length    */
                                      /*  Possible values are: IS,     */
                                      /*   OOS, UNEQ, ALW, INH         */
#define    DB_SQLCMD_LEN     50000    /* Max length of any SQL command */
#define    DB_MAKEMSG_LEN    12000    /* Max length of any string      */
                                      /*  handled by DBmakeMsg         */

//
// Constants defined for DBCN
//
#define    DBCN_MAX_ERRMSG_LEN     200
#ifdef CC
   #define DBCN_MAX_CLIENTS	   32
   #define DBCN_MAX_SUBSCRIPTIONS  1024
   #define DBCN_MAX_TABLES         128
   #define DBCN_MAX_PAGES          5120
#else
   // For EE
   #define DBCN_MAX_CLIENTS        4
   #define DBCN_MAX_SUBSCRIPTIONS  128
   #define DBCN_MAX_TABLES         32
   #define DBCN_MAX_PAGES          100
#endif

#endif
