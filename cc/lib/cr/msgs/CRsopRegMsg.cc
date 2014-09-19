/*
**      File ID:        @(#): <MID20431 () - 08/17/02, 29.1.1.1>
**
**	File:					MID20431
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:39:51
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**	This file defines the class used by a SOP to register with CSOP.
**
** OWNER: 
**	Roger McKee
**
*/
#include <string.h>
#include "cc/hdr/cr/CRsopRegMsg.H"
#include "cc/hdr/cr/CRdebugMsg.H"

CRsopRegMsg::CRsopRegMsg()
{
	priType = MHoamPtyp;
	msgType = CRsopRegMsgTyp;
	sopMhname[0] = '\0';
}

GLretVal
CRsopRegMsg::send(Bool regFlag)
{
	return send(CRprocname, regFlag);
}

GLretVal
CRsopRegMsg::send(const char* destName, Bool regFlag)
{
	registerFlag = regFlag;
	strcpy(sopMhname, destName);

	Short msgsz = CRsopRegMsgSz;
	return MHmsgBase::send("CSOP", MHnullQ, msgsz, 0);
}
