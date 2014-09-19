#ifndef __DBITIME_H
#define __DBITIME_H
/*
**
**	File ID:	@(#): <MID7029 () - 08/17/02, 29.1.1.1>
**
**	File:					MID7029
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:21:47
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**		define timer constants
**
** OWNER:
**	 Yeou H. Hwang
**
** NOTES:
*/

const Short DBIsqlAckSendTries =  2; // Number of attempts to send a
                                        // DBselectAck before aborting
const Short DBIackBackRcvTries    = 30; // Number of attempts to receive a
                                        // DBackBack before aborting

const Long DBItimeDBselectAck = 5000;   // in milliseconds
const Long DBItimeDBackBack   = 1200;   // in milliseconds

#endif
