/*
**
**	File ID:	@(#): <MID7019 () - 08/17/02, 29.1.1.1>
**
**	File:					MID7019
**	Release:				29.1.1.1
**	Date:					06/02/03
**	Time:					14:01:50
**	Newest applied delta:	06/23/03
**
** DESCRIPTION:
** 	define DBIqryStat message
**
** OWNER:
##	eDB Team
**
** History
**	Yeou H. Hwang
**	Sheng Zhous(06/23/03): Modifed for f61286.
** NOTES:
*/

#include <stdio.h>
#include "cc/hdr/msgh/MHqid.hh"
#include "cc/hdr/msgh/MHpriority.hh"
#include "DBIqryStat.hh"

DBIqryStat::DBIqryStat()
{
	priType = MHoamPtyp;
	msgType = DBIqryStatMsgTyp;
	srcQue = MHnullQ;
}

GLretVal
DBIqryStat::send(MHqid toQid, MHqid sQid, Long time)
{
	return (MHmsgBase::send(toQid, sQid, sizeof(*this), time));
}
