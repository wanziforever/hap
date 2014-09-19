/*
**      File ID:        @(#): <MID17558 () - 02/19/00, 23.1.1.1>
**
**	File:					MID17558
**	Release:				23.1.1.1
**	Date:					05/13/00
**	Time:					13:26:58
**	Newest applied delta:	02/19/00
**
** DESCRIPTION:
**	This file implements the class that represent MSGH message that
**	is sent in a handshake mode between the RPROC process and
**	the CPROC process. To start with, the RPROC process will 
**	sent this message as a request for device specific info	to
**	the CPROC process. The CPROC process in turn will sent back the 
**	required device specific information to the RPROC process.
**
** OWNER: 
**	Yash Gupta
**
** NOTES:
**
*/

#include <stdio.h>
#include <termio.h>
#include <string.h>
#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "cc/cr/hdr/CRrpInitMsg.H"
#include "cc/hdr/cr/CRmtype.H"
#include "cc/hdr/cr/CRdebugMsg.H"

CRrpInitMsg::CRrpInitMsg()
{
	device[0] = '\0';
}

CRrpInitMsg::~CRrpInitMsg()
{
}

GLretVal
CRrpInitMsg::sendToRproc(const char *rprocName, const char *devName)
{
	
	if (strlen(devName) >= CRmaxDevNameLen || strlen(devName) == 0)
	{
		CRERROR("Incorrect device name. Device = %s", devName);
		return GLfail;
	}

	strcpy(device, devName);
	
	priType = MHoamPtyp;
	msgType = CRrpInitMsgTyp;

	Short msgsz = CRrpInitMsgSz;
	return MHmsgBase::send(rprocName, MHnullQ, msgsz, -1);
}

GLretVal
CRrpInitMsg::sendToRproc(const char *rprocName)
{
	priType = MHoamPtyp;
	msgType = CRrpInitMsgTyp;

	device[0] = '\0';
	Short msgsz = CRrpInitMsgSz;
	return MHmsgBase::send(rprocName, MHnullQ, msgsz, -1);
}



