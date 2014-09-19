/*
**      File ID:        @(#): <MID18629 () - 08/17/02, 29.1.1.1>
**
**	File:					MID18629
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:39:50
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**	This file implements the class that represents MSGH message that
**	is sent to the ROP process to remove or restore the ROP.
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

#include "cc/cr/hdr/CRropMsg.H"
#include "cc/hdr/cr/CRmtype.H"

static const char ROPprocName[] = "ROP";

CRropMsg::CRropMsg(Bool state_value, const CRomInfo* ominf) : 
        state(state_value), omInfo(*ominf)
{
}

CRropMsg::~CRropMsg()
{
}

Bool
CRropMsg::getStateFlag() const
{
	return state;
}

GLretVal
CRropMsg::send()
{
	priType = MHoamPtyp;
	msgType = CRropMsgTyp;

	Short msgsz = CRropMsgSz;
	return MHmsgBase::send(ROPprocName, MHnullQ, msgsz, -1);
}

const CRomInfo*
CRropMsg::getOmInfo() const
{
	return &omInfo;
}
