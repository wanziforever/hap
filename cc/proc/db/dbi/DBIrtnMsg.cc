/*
**
**	File ID:	@(#): <MID7022 () - 08/17/02, 29.1.1.1>
**
**	File:					MID7022
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:21:46
**	Newest applied delta:	06/23/03
**
** DESCRIPTION:
**		DBIrtnMsg:
**	 	Send result (DBsqlAck) from the execution of DBinsert, DBupdate, DBdelete and DBselect
**
** OWNER:
**	eDB Team
**
** History
**	Yeou H. Hwang
**	Sheng Zhou(06/23/03):Modified for f61286
**	Lucy Liang (11/19/03): Modified for f61284
** NOTES:
*/
  
#include <stdio.h>
#include <string.h>
#include "cc/hdr/msgh/MHqid.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
//#include "cc/hdr/db/DBassert.hh"
#include "cc/hdr/db/DBsqlMsg.hh"
#include "cc/hdr/db/DBselect.hh"
#include "DBItime.hh"
//#include "cc/hdr/cr/CRdebugMsg.hh"
#include "cc/hdr/db/DBretval.hh"
//#include "cc/hdr/db/DBdebug.hh"
  
#include "hdr/mydebug.h"

extern MHqid DBImhqid; // mhqid associate with teh DBI process

Short DBIrtnSelMsg(DBsqlMsg *, Long, const Char *);

Short 
DBIrtnMsg(DBsqlMsg *sqlp, Long ackCode, const Char *ackMessage)
{
  Short sqlType = sqlp->msgType;
	DBsqlAck ackMsg;
	Short rtn;
	Short i = 0;

	if (sqlType == DBselectTyp)
	{
		return(DBIrtnSelMsg(sqlp, ackCode, ackMessage));
	}
	else if (sqlType == DBinsertTyp)
	{
		ackMsg.msgType = DBinsertAckTyp;
	}
	else if (sqlType == DBupdateTyp)
	{
		ackMsg.msgType = DBupdateAckTyp;
	}
	else if (sqlType == DBdeleteTyp)
	{
		ackMsg.msgType =  DBdeleteAckTyp;
	}
	else
	{
		CRERROR("DBIrtnMsg(): unknown sqlType(%d), srcQue(%s), sid(%d),"
            "endFlag(%d)", sqlType, sqlp->srcQue.display(),
		        sqlp->sid, sqlp->endFlag);
		return(DBFAILURE);
	}
	ackMsg.nrec = 0;
	ackMsg.ackCode = ackCode;
	strcpy(ackMsg.ackMessage, ackMessage);
	ackMsg.sid = sqlp->sid;

	while (((rtn = ackMsg.send(sqlp->srcQue, DBImhqid, 0)) == MHintr) &&
	       (i++ < DBIsqlAckSendTries))
	{
		;
	}

	if (rtn == GLsuccess)
	{
		CRDEBUG(DBmsgsent,
		        ("DBIrtnMsg(): sent message, code(%d), srcQue(%s)",
		         ackCode, sqlp->srcQue.display()));  
		return(DBSUCCESS);
	}
	else
	{
    //DBIGENLERROR(DBIrtnMsgFail, (DBIrtnMsgFailFmt, rtn));
    printf("DBIrtnMsgFail\n");
		return(DBFAILURE);
	}
}


Short 
DBIrtnSelMsg(DBsqlMsg *sqlp, Long ackCode, const Char *ackMessage)
{
	DBselectAck ackMsg;
	Short rtn;
	Short i = 0;

	ackMsg.msgType = DBselectAckTyp;
	ackMsg.ackCode = ackCode;
	strcpy(ackMsg.ackMessage, ackMessage);
	ackMsg.sid = sqlp->sid;
	ackMsg.endFlag = TRUE;
	ackMsg.reqAck = FALSE;
	ackMsg.npairs = 0;

	// try DBIsqlAckSendTries times 
	while (((rtn = ackMsg.send(sqlp->srcQue, DBImhqid, DBselectAckHdrSz, 0))
          == MHintr) &&
	       (i++ < DBIsqlAckSendTries))
	{
		;
	}

	if (rtn == GLsuccess)
	{
		return(DBSUCCESS);
	}
	else
	{
		//DBIGENLERROR(DBIrtnSelMsgFail, (DBIrtnSelMsgFailFmt, rtn));
    printf("DBIrtnSelMsgFail\n");
		return(DBFAILURE);
	}
}
