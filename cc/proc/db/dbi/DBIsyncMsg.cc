
/*
**
**	File ID:	@(#): <MID7027 () - 08/17/02, 29.1.1.1>
**
**	File:					MID7027
**	Release:				29.1.1.1
**	Date:					06/02/03
**	Time:					13:57:34
**	Newest applied delta:	06/23/03
**
** DESCRIPTION:
** 	define DBIsyncMsg
**
** OWNER:
**	eDB Team
**
** History
**	Yeou H. Hwang
**	Sheng Zhous(06/23/03):Modified for f61286
**
** NOTES:
*/

#include <stdio.h>
#include "cc/hdr/msgh/MHpriority.hh"
#include "DBIsyncMsg.hh"

DBIsyncMsg::DBIsyncMsg()
{
	priType = MHoamPtyp;
	msgType = DBIsyncMsgTyp;
	srcQue = MHnullQ;
}

GLretVal
DBIsyncMsg::send(MHqid toQid, MHqid sQid, Long time)
{
	return (MHmsgBase::send(toQid, sQid, sizeof(*this), time));
}

