/*
**	File ID: 	@(#): <MID66138 () - 12/13/03, 2.1.1.2>
**
**	File:			MID66138
**	Release:		2.1.1.2
**	Date:			12/15/03
**	Time:			11:26:31
**	Newest applied delta:	12/13/03 15:11:23
**
** DESCRIPTION:
** 	This file contains the three rountines required by INIT process
**	cleanup, procinit, sysinit. It also implements the class
**	DBIhprMsg, and the functions to send out ready or working messages.
**
** OWNER:
**	eDB Team
**
** History:
**	Yeou H. Hwang
**	Lucy Liang (05/14/03): Create by merging mainhpr.C, 
**		DBIhprInit.C, DBIhprMsg.C and DBIhprRdy.C for f61286
**	Lucy Liang (11/03/03): Update for f61284
**
** NOTES:
*/

#include <stdio.h>
#include <String.h>
#include <stdlib.h>
//#include <new.h>
#include "hdr/GLtypes.h"
#include "cc/hdr/misc/GLreturns.h"      // for GLnoMemory
#include "cc/hdr/eh/EHhandler.hh"
#include "cc/hdr/db/DBsql.hh"
//#include "cc/hdr/db/DBassert.hh"
#include "cc/hdr/db/DBselect.hh"
//#include "cc/hdr/db/DBdebug.hh"
#include "cc/hdr/db/DBlimits.hh"

#include "DBI.hh"
#include "DBIhpr.hh"
#include "DBImsgBuffer.hh"
#include "DBIhprMsg.hh"
#include "DBIselect.hh"
#include "DBItime.hh"

#include "cc/hdr/init/INusrinit.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/msgh/MHpriority.hh"
//#include "cc/hdr/cr/CRdebugMsg.hh"
 
#include "hdr/mydebug.h"
#include <string.h>

MHqid DBImainQid;			// the MSGH qid passed by the DBI main
DBImsgBuffer msgBuffer;
Char DBImyName[MHmaxNameLen+1];  // DBI helper name
extern MHqid DBImhqid;		// MSGH Qid of the helper
int DBIselectAckDataSz = MHmsgSz - MHmsgBaseSz - DB_SQLERROR_LEN - 12;

extern "C" Void DBIsigterm(int);
Void newHandler();
Long DBIprocessNoSelect(DBsqlMsg *, Long*, Char*);
extern Short DBIhprGetSql(DBsqlMsg *&);
extern Void  DBIhprRdy();
extern Short DBIregister(const Char *, MHregisterTyp);
extern Short DBIunRegister(const Char *);
extern Void DBItimerInit();
extern int DBIhprInit();
extern Void DBIhprCleanup();
extern int DBIdoFetches(Char*, int, Char*, int, int*, int*, Char, int*, Char*);
extern Void DBIconvertSpecialChar(Char *, Char *);
extern Long DBIgetTableName(DBsqlMsg *, Char *);
extern Short DBIrtnMsg(DBsqlMsg *, Long, const Char *);


// The callback function for SIGTERM
Void DBIsigterm(int)
{
	// disconnect from Database.
	CRDEBUG(DBinout, ("Got SIGTERM and Enter DBIsigterm()."));
	DBIhprCleanup();
	DBdisconnect();
	CRDEBUG(DBinout, ("Exit DBIsigterm()."));
	exit(1);
}

Void process(int, Char**, SN_LVL, U_char)
{
	DBsqlMsg *DBIsqlMsgPtr = 0;
	DBIselectBufLstTyp *ptlst;
	Char *ptbuf;
	Char bLastBuffer = FALSE;
	std::string debugMsg;
	U_short rowLimit;
	int npairs = 0;
	int alen = 0;       // accumulated length of the name-value pairs
	int rtn;
	Long ackCode = 0;
	Char ackMsg[DB_SQLERROR_LEN];

	CRDEBUG(DBinout,("Enter process()."));

	CRDEBUG(DBinitFlg, ("Using dataSz(%d)", DBIselectAckDataSz));

	while (1)
	{
		//Send a ready message to the DBI main process.
		//Always send ready mesage to DBI even DBIgetSel failed.
		DBIhprRdy();

		if (DBIhprGetSql(DBIsqlMsgPtr) == DBSUCCESS){
			CRDEBUG(DBdbOper,("process(): Got SQL:(%s), srcQue=%s,"
                        "sid=%d,rowLimit=%d", DBIsqlMsgPtr->sql,
                        DBIsqlMsgPtr->srcQue.display(),DBIsqlMsgPtr->sid,
                        DBIsqlMsgPtr->rowLimit));
			if (DBIsqlMsgPtr->msgType == DBinsertTyp ||
			    DBIsqlMsgPtr->msgType == DBdeleteTyp ||
			    DBIsqlMsgPtr->msgType == DBupdateTyp)
			{
				DBIprocessNoSelect(DBIsqlMsgPtr,&ackCode,ackMsg);
				DBIrtnMsg(DBIsqlMsgPtr, ackCode, ackMsg);
				break;
			}

			ptlst = msgBuffer.getHead();
			ptbuf = ptlst->buf + DBselectAckHdrSz;    // point to buffer
			rowLimit = DBIsqlMsgPtr->rowLimit;

			rtn = DBIdoFetches(DBIsqlMsgPtr->sql, rowLimit, ptbuf, DBIselectAckDataSz, &npairs, &alen, bLastBuffer, &ackCode, ackMsg);
			while (rtn == DBI_FETCHNOTDONE || rtn ==DBI_NEEDMOREBUFFER)
			{
				if (rtn == DBI_NEEDMOREBUFFER)
				{
					msgBuffer.fillHdrInfo(ptlst, DBIsqlMsgPtr, 0, FALSE, FALSE, npairs);
					ptlst->len = alen;      // length of the name-value pairs

					// Use the next message.
					ptlst = msgBuffer.getNext();
					if(ptlst == (DBIselectBufLstTyp *) 0)
					{
						// Memory is used up.
						CRERROR("DBIdoFetches(): failed to alloc more message buffer");
						DBIhprCleanup();
						msgBuffer.sndErrMsg(DBI_FETCHFATALERR, "Not enough memory space for select result.", DBIsqlMsgPtr);
						break;
					}
					ptbuf = ptlst->buf + DBselectAckHdrSz;
					if(msgBuffer.getNumBufferUsed() > DBI_SELECTBUFNO)
					{
						bLastBuffer = TRUE;
					}
					else
					{
						bLastBuffer = FALSE;
					}
				}
				else
				{
					msgBuffer.fillHdrInfo(ptlst, DBIsqlMsgPtr, 0, FALSE, TRUE, npairs);
					ptlst->len = alen;      // length of the name-value pairs

					if (msgBuffer.sndSelectAck(msgBuffer.getNumBufferUsed(), TRUE, DBIsqlMsgPtr) != DBI_SNDCOMPLETE)
					{
						CRERROR("DBI Helper process(): failed to send selectAck message back.");
						DBIhprCleanup();
						break;  // stop processing this statement
					}
					
					msgBuffer.reset();
					ptlst = msgBuffer.getHead();
					ptbuf = ptlst->buf + DBselectAckHdrSz;
					bLastBuffer = FALSE;
				}
		
				rtn = DBIdoFetches(DBIsqlMsgPtr->sql, rowLimit, ptbuf, DBIselectAckDataSz, &npairs, &alen, bLastBuffer, &ackCode, ackMsg);
			}

			if (rtn == DBI_FETCHDONE)
			{
				msgBuffer.fillHdrInfo(ptlst, DBIsqlMsgPtr, 0, TRUE, FALSE, npairs);
				ptlst->len = alen;    // length of the name-value pairs

				msgBuffer.sndSelectAck(msgBuffer.getNumBufferUsed(), FALSE, DBIsqlMsgPtr);
			}
			else if (rtn == DBI_FETCHFAIL ||
			         rtn == DBI_FETCHFATALERR) 
			{
				msgBuffer.sndErrMsg(ackCode,ackMsg,DBIsqlMsgPtr);
			}

			msgBuffer.reset();
		}
	}
}


// rountines required by INIT process
Short
sysinit(int , Char **, SN_LVL , U_char )
{
	return(GLsuccess);
}


// rountines required by INIT process
Short
procinit(int , Char *argv[], SN_LVL sn_lvl , U_char )
{
	//set_new_handler(newHandler);
	CRERRINIT(argv[0]);

	// arg1 is the name of the helper process
	strcpy(DBImyName, argv[0]);

	// register with MSGH
	// the main DBIhelper is called DBIhelper
	if (DBIregister(argv[0], MH_LOCAL) == DBFAILURE)
	{
		//DBIGENLERROR(DBrgstMSGHFail, (DBrgstMSGHFailFmt ) );
    printf("DBrgstMSGHFail\n");
		return(GLfail);
	}

	if (DBconnect() == DBFAILURE)
	{
		if (sn_lvl == SN_LV5 || sn_lvl == SN_LV3) {
			// DB may not be up yet
			// try until INIT kills me
			while (1) {
				//DBIGENLERROR(DBconnDbTry, (DBconnDbTryFmt ) );
        printf("DBconnDbTry\n");
				sleep(5);
				if (DBconnect() == DBFAILURE)
           continue;
				else
           break;
			}
		}
		else {
			//DBIGENLERROR(DBconnDbFail, (DBconnDbFailFmt) );
      printf("DBconnDbFail\n");
			return(GLfail);
		}
	}

	// Allocate necessary resource to handle SQL statement
	if (DBIhprInit() == DBI_FAILURE)
	{
		CRERROR("Helper fail in initialization DB data structures.");
		return(GLfail);
	}

	// initialize message node storage
	if (msgBuffer.init() == DBFAILURE)
	{
		//DBIGENLERROR(DBhprMSGinitFail, ( DBhprMSGinitFailFmt ) );
    printf("DBhprMSGinitFail\n");
		return(GLfail);
	}

	//Initiate timer used by EHhandler
	DBItimerInit();

	signal(SIGTERM, DBIsigterm);

	//
	// DBIhelper by default sends short DBselectAck messsages of size
	// MHmsgSz (3800).  If File "/sysdata1/.DBselectAck.long" is present,
	// then it sends long DBselect messages of size MHmsgLimit (32752).
	//
	if (access("/sysdata1/.DBselectAck.long", F_OK) == 0)
	{
		DBIselectAckDataSz = DBdataSz;
	}

	// get DBI qid
	while (MHmsgh.getMhqid((const Char *)"DBI", DBImainQid) != GLsuccess)
	{
		sleep(1);
	}

	return(GLsuccess);
}


// rountines required by INIT process
Short
cleanup(int , Char *argv[], SN_LVL , U_char )
{
	CRDEBUG(DBinout, ("Enter cleanup()"));

	// clean up and disconnect from DB
	DBIhprCleanup();
	DBdisconnect();

	// remove process name
	if (DBIunRegister(argv[0]) == DBFAILURE)
	{
		CRERROR("Exit cleanup() abnormally. Fail to un-register DBI helper from MSGH");
		return(GLfail);
	}

	CRDEBUG(DBinout, ("Exit cleanup() normally."));
	return(GLsuccess);
}


//
// handler for memory exhaustion exception
//
Void newHandler()
{
  // DBI helper process will die and get reinit'ed
  CRERROR("OUT OF MEMORY exception caught");
  INITREQ(SN_LV0, GLnoMemory, "OUT OF MEMORY", IN_EXIT);
}


// Send DBIhprMsg to DBI. Telling DBI that it's ready
Void DBIhprRdy()
{
	CRDEBUG(DBinout, ("Enter DBIhprRdy()"));
	DBIhprMsg msg;
	Short rtn, i = 0;

	msg.state =  DBIhelperRdy;

	strcpy(msg.name, DBImyName);
	CRDEBUG(DBmsgsent, ("Helper %s sends ready message to DBI %s", 
                      DBImhqid.display(), DBImainQid.display()));
	while ((rtn = msg.send(DBImainQid, DBImhqid,-1)) != GLsuccess && 
	       i++<DBIsqlAckSendTries)
	{	
    ;
	}
	if (rtn != GLsuccess)
	{
    //DBIGENLERROR(DBHprSndRdyFail, (DBHprSndRdyFailFmt) );
    printf("DBHprSndRdyFail\n");
	}
	else
	{
    CRDEBUG(DBinout, ("Exit DBIhprRdy(). Send ready message successfully."));
	}
}

// Send DBIhprMsg to DBI. Telling DBI that it's working on a query.
Void DBIhprWorking()
{
	CRDEBUG(DBinout, ("Enter DBIhprWorking()"));
	DBIhprMsg msg;
	Short rtn, i = 0;

	msg.state =  DBIhelperWorking;

	strcpy(msg.name, DBImyName);
	CRDEBUG(DBmsgsent, ("Helper %s sends working message to DBI %s", 
                      DBImhqid.display(), DBImainQid.display()));
	while ((rtn = msg.send(DBImainQid, DBImhqid,-1)) != GLsuccess && 
	       i++<DBIsqlAckSendTries)
	{	
    ;
	}
	if (rtn != GLsuccess)
	{
    //DBIGENLERROR(DBHprSndRdyFail, (DBHprSndRdyFailFmt) );
    printf("DBHprSndRdyFail");
	}
	else
	{
    CRDEBUG(DBinout, ("Exit DBIhprWorking(). Send working message successfully."));
	}
}

Void DBIcrDebug(Short flag, Char *crmsg)
{
  // prepare a debug message and send it to CSOP
  CRDEBUG(flag, ("%s", crmsg) );
}

Void DBIcrError(Char *crmsg)
{
  // prepare a error message and send it to CSOP
  CRERROR( ("%s", crmsg) );
}


Long
DBIprocessNoSelect(DBsqlMsg *sqlp, Long *ackCode, Char *ackMsg)
{
	Short sqlType = sqlp->msgType;
	Char *where;
	U_short whereOffset = sqlp->whereOffset;
	Char *sqlStatement = sqlp->sql;
	Char tblName[DB_TBL_NAME_LEN  * 2 + 2]; /* include "<schema>." */
	Char convertedSql[2*DBsqlSz+1];

	// setp 1, test parse the command get the table name
	// table name is required
	DBIgetTableName(sqlp, tblName);

  // DBlockTable() and DBselectUpdate() are commented out below for
  // a lock problem found in nc_cr8358. The lock of table meas_mstr
  // block us from reading the view usr_tab_columns later, which cause
  // the NDB RTDB not come up. Commenting out these two functions is
  // safe because the update and delete operation will automatically
  // aquire the proper lock by the design of Oracle and PostgreSQL.

	// step 2: lock table in row exclusive mode
/*
	if (DBlockTable(tblName)==DBFAILURE)
	{
  CRDEBUG(DBdbOper,("DBIprocessNoSelect: DBlockTable failed, Error code: %d, Error message: %s", DBsqlErrorCode(), DBsqlErrorMsg()));
  return DBFAILURE;
	}
*/
	// if the cmd is update or delete, lock the affected rows first
	// use SELECT FOR UPDATE command
	// we need to know a column name

	if ((sqlType == DBupdateTyp) || (sqlType == DBdeleteTyp))
	{
		if ((whereOffset > 0) && (whereOffset < (Short) strlen(sqlStatement)) )
       where = &sqlStatement[whereOffset];
		else
       where = (Char *) 0;

/*
  if (DBselectUpdate(tblName, where) == DBFAILURE)
  {
  CRDEBUG(DBdbOper,("DBIprocessNoSelect:DBselectUpdate: failed. Error code: %d, Error message:%s", DBsqlErrorCode(), DBsqlErrorMsg()));
  return DBFAILURE;
  }
*/
	}
	else
	{
		DBIconvertSpecialChar(convertedSql, sqlStatement);
		sqlStatement = convertedSql;
	}

	// call lib function execute statement
	if(DBexecSql(sqlStatement, FALSE) == DBFAILURE)
	{
		*ackCode = DBsqlErrorCode();
		strcpy(ackMsg, DBsqlErrorMsg());
		CRDEBUG(DBdbOper,("DBIprocessNoSelect: DBexecSql failed. Error code: %d, Error message: %s", ackCode, ackMsg));

		/* we need to use COMMIT here to release the row locks */
		DBcommit();
		return DBFAILURE;
	};


	// important, need to commit
	if(DBcommit() == DBFAILURE)
	{
		*ackCode = DBsqlErrorCode();
		strcpy(ackMsg, DBsqlErrorMsg());
		CRDEBUG(DBdbOper,("DBIprocessNoSelect:DBcommit: failed: erro code: %d, error message: %s", ackCode, ackMsg));
		return DBFAILURE;
	}

	return DBSUCCESS;
}

