#ifndef __DBISELECT_H
#define __DBISELECT_H

/*
**	File ID: 	@(#): <MID66144 () - 07/22/03, 2.1.1.1>
**
**	File:			MID66144
**	Release:		2.1.1.1
**	Date:			07/29/03
**	Time:			03:10:58
**	Newest applied delta:	07/22/03 03:37:29
**
** DESCRIPTION:
**	define constants for SELECT operation and buffer list
**
** OWNER:
**	eDB team
** HISTORY
**	Yeou H. Hwang
**	Lucy Liang	Changed for Feature 61286
**	Hon-Wing Cheng	Changed for Feature 70490
**
** NOTES:
*/

/* define result of DBIdoFetches()*/
#define DBI_FETCHDONE		1
#define DBI_FETCHFATALERR	-1
#define DBI_FETCHFAIL		0
#define DBI_FETCHNOTDONE	2
#define DBI_NEEDMOREBUFFER	3

/* define the fetch status*/
#define LASTFETCHDONE	0
#define FETCHRECORD		1
#define FETCHRECORDDONE	2
#define FETCHCOLUMN		3
#define FETCHCOLUMNDONE	4

/* define the return value of DBIhprInit.
 */
#define DBI_SUCCESS		0
#define DBI_FAILURE		-20001


#define DBI_TIMEOUT	-1

#define DBI_SNDTIMEOUT	-1
#define DBI_ABORTSELECT	-2
#define DBI_SNDCOMPLETE	1
#define DBI_SNDFAIL	2

#define DBI_SELECTBUFNO	5	/* no of message can be sent at one time */

typedef struct DBIselectBufLst
{
	char *buf;	/* ecah one point to a DBselectAck */
	int len;	/* len of the of real message, <= DBselectAck */
	struct DBIselectBufLst *next;
} DBIselectBufLstTyp;
#endif

