/*
**      File ID:        @(#): <MID18637 () - 08/17/02, 29.1.1.1>
**
**	File:					MID18637
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:39:53
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**      This file contains the definition of the CRomInfo class.
**      This class is used by USLI to hold information needed to properly
**      route an output message.
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

#include <string.h>
#include "cc/hdr/cr/CRomInfo.hh"

CRomInfo::CRomInfo()
{
	init();
}

CRomInfo::CRomInfo(CRALARMLVL a, short msghqid)
{
	init(a, "", msghqid);
}

CRomInfo::CRomInfo(CRALARMLVL a, const char* usliName)
{
	init(a, usliName);
}

void
CRomInfo::init(CRALARMLVL a, const char* usliName, short msghqid)
{
	alarmLevel = a;
	strcpy(usliProcName, usliName);
	usliMsghqid = msghqid;
}

CRALARMLVL
CRomInfo::getAlarmLevel() const
{
	return alarmLevel;
}

const char*
CRomInfo::getUSLIname() const
{
	return usliProcName;
}
