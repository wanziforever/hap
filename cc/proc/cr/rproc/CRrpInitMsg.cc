
/*
**      File ID:        @(#): <MID17559 () - 02/19/00, 23.1.1.1>
**
**	File:					MID17559
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

GLretVal
CRrpInitMsg::sendToCproc(const char *cprocName)
{
	priType = MHoamPtyp;
	msgType = CRrpInitMsgTyp;

	Short msgsz = CRrpInitMsgSz;
	return MHmsgBase::send(cprocName, MHnullQ, msgsz, -1);
}


const char *
CRrpInitMsg::getDevName()
{
	return (device);
}


