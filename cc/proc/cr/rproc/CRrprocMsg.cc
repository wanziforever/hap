/*
**      File ID:        @(#): <MID17563 () - 02/19/00, 23.1.1.1>
**
**	File:					MID17563
**	Release:				23.1.1.1
**	Date:					05/13/00
**	Time:					13:27:00
**	Newest applied delta:	02/19/00
**
** DESCRIPTION:
**	This file contains the member functions for the CRrprocMsg and 
**      CRrpHwFailMsg classes.
**	CRrprocMsg is used to send the input received by the RPROC 
**	process to the CPROC process.
**      CRrpHwFailMsg is used to inform the CPROC process of a hardware
**      failure.
** OWNER: 
**	Yash Gupta
**      Roger McKee
** NOTES:
**
*/

#include <sysent.h>
#include <stdio.h>
#include <termio.h>
#include <string.h>
#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "cc/cr/hdr/CRrprocMsg.H"
#include "cc/hdr/cr/CRmtype.H"

extern char CRcprocName[];

CRrprocMsg::CRrprocMsg()
{
	numchars = 0;
	text[0] = '\0';
	rprocPid = (int) getpid();
}

CRrprocMsg::~CRrprocMsg()
{
}

GLretVal
CRrprocMsg::send()
{
	GLretVal ret_val;

	if (mhqid == MHnullQ)
	{
		ret_val = MHmsgh.getMhqid(CRcprocName, mhqid);
		if (ret_val!= GLsuccess)
			return GLfail;
	}

	/* note this priority type is CRUCIAL!
	** The USLI parser will be waiting 
	** for a MSGH message with the following priority and type
	*/	        	
	priType = CRrprocMsgPri;
	msgType = CRrprocMsgTyp;

	if (length() == 0)
		return GLsuccess;

	Short msgsz = CRrprocMsgSz;
	return MHmsgBase::send(mhqid, MHnullQ, msgsz, -1);
}

GLretVal
CRrprocMsg::putMsg(const char *input, short len)
{
	memcpy(text, input, len);
	numchars = len;
	return (GLsuccess);
}


CRrpHwFailMsg::CRrpHwFailMsg()
{
}

CRrpHwFailMsg::~CRrpHwFailMsg()
{
}

GLretVal
CRrpHwFailMsg::send()
{
	/* note this priority type is CRUCIAL!
	** The USLI parser will be waiting 
	** for a MSGH message with the following priority and type
	*/	        	
	priType = CRrprocMsgPri;
	msgType = CRrpHwFailMsgTyp;

	Short msgsz = CRrpHwFailMsgSz;
	return MHmsgBase::send(CRcprocName, MHnullQ, msgsz, -1);
}
