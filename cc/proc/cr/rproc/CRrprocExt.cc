/*
**      File ID:        @(#): <MID17562 () - 02/19/00, 23.1.1.1>
**
**	File:					MID17562
**	Release:				23.1.1.1
**	Date:					05/13/00
**	Time:					13:27:00
**	Newest applied delta:	02/19/00
**
** DESCRIPTION:
**	This file contains member functions for the CRrprocMsg class.
**	This class is used to sent the input received by the RPROC 
**	process to the CPROC process.
** OWNER: 
**	Yash Gupta
**
** NOTES:
**
*/
#include "hdr/GLtypes.h"
#include "cc/cr/hdr/CRrprocMsg.H"

MHqid CRrprocMsg::mhqid = MHnullQ;

const char *
CRrprocMsg::getMsg() const
{
	return text;
}

short
CRrprocMsg::length() const
{
	return numchars;
}
