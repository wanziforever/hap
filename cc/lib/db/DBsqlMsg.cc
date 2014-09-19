/*
**	EMACS_MODES: c, !fill, tabstop=4
**
**	File ID:	%Z%: <%M% (%Q%) - %G%, %I%>
**
**	File:					%M%
**	Release:				%I%
**	Date:					%H%
**	Time:					%T%
**	Newest applied delta:	%G%
**
** DESCRIPTION:
** 	Definition of member functions of the message classes DBsqlMsg and DBsqlAck
**
** OWNER:
**	Yeou H. Hwang
**
** NOTES:
*/

#include "cc/hdr/msgh/MHqid.hh"
#include "cc/hdr/db/DBretval.hh"
#include "cc/hdr/db/DBsqlMsg.hh"


DBsqlMsg::DBsqlMsg()
{
	priType = MHoamPtyp;
	srcQue = MHnullQ;
	sqlSz = 0;
	rowLimit = 0;
	sql[0] = '\0';
}


GLretVal 
DBsqlMsg::send(MHqid toQid, MHqid sQid, Long time)
{
	Short len;

	len = DBsqlMsgHdrSz+sqlSz;
	return (MHmsgBase::send(toQid, sQid, len, time));
}

GLretVal
DBsqlMsg::send(const Char *to, MHqid sQid, Long time)
{
  Short len;

  len = DBsqlMsgHdrSz+sqlSz;
  return (MHmsgBase::send(to, sQid, len, time));
}

DBsqlAck::DBsqlAck()
{
	priType = MHoamPtyp;
}


GLretVal 
DBsqlAck::send(MHqid toQid, MHqid sQid, Long time)
{
  if (ackCode == DBSUCCESS)
  {
    ackMessage[0] = '\0';
  }
  return (MHmsgBase::send(toQid, sQid, DBsqlAckSz, time));
}
