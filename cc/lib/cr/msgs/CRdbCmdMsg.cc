/*
**      File ID:        @(#): <MID18628 () - 09/06/02, 29.1.1.1>
**
**	File:					MID18628
**	Release:				29.1.1.1
**	Date:					09/12/02
**	Time:					10:39:18
**	Newest applied delta:	09/06/02
**
** DESCRIPTION:
**	This file contains the member function for the class CRdbCmdMsg
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**	These functions were previously "inline".
*/

#include "cc/hdr/cr/CRdbCmdMsg.H"
#include "cc/hdr/cr/CRdebugMsg.H"

CRdbCmdMsg::CRdbCmdMsg()
{
        priType = MHoamPtyp;
        msgType = CRdbCmdMsgTyp;
        srcQue = MHnullQ;
	isDebugOn = NO;
}
 
Bool
CRdbCmdMsg::setMap(const char* flagName)
{
	if (strcmp(flagName, "off") == 0 || strcmp(flagName, "OFF") == 0)
		isDebugOn = NO;
	else
		isDebugOn = YES;
	
	return bitmap.setMap(flagName);
}

Bool
CRdbCmdMsg::setMap(short flagValue)
{
	if (flagValue < 0)
	{
		return NO;
	}
	isDebugOn = YES;
	bitmap.setBit(flagValue);
	return YES;
}

GLretVal
CRdbCmdMsg::send(MHqid toQid, MHqid sQid, Long time)
{
	return MHmsgBase::send(toQid, sQid, (Long) sizeof(CRdbCmdMsg), time);
}
 
GLretVal
CRdbCmdMsg::send(const Char *name, MHqid sQid, Long time)
{
	return MHmsgBase::send(name, sQid, (Long) sizeof(CRdbCmdMsg), time);
}

void
CRdbCmdMsg::unload() const
{
	/*fprintf(stderr, "before unloading\n");
	CRtraceFlags.dump();*/
	
	CRtraceFlags.copy(isDebugOn, bitmap);

	/*fprintf(stderr, "after unloading\n");
	CRtraceFlags.dump();*/
}

const unsigned char*
CRdbCmdMsg::ssnStart(CRsubsys bitnum)
{
	return bitmap.getByteStart(bitnum);
}

