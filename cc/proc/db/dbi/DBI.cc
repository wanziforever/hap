/*
**	File ID: 	@(#): <MID66180 () - 01/18/04, 2.1.1.2>
**
**	File:			MID66180
**	Release:		2.1.1.2
**	Date:		01/18/04
**	Time:		05:30:38
**	Newest applied delta:	01/18/04 05:25:26
**
** DESCRIPTION:
** 	DBI main process
**	DBI is used to receive other processes' db operation
**	request, decide which DBMS the request should go and
**	pass the request messages to the corresponding helper.
**
** OWNER:
**	eDB team
**
** History
**	Yeou H. Hwang
**	Sheng Zhou(06/23/03):Modified for f61286
**	Lucy Liang(11/13/03):Modified for f61284. Make DBI a 
**	    message dispatcher and talk to both platdb and 
**	    Oracle helpers.
**
** NOTES:
*/

//#include <new.h>

#include "string.h"
#include "hdr/GLtypes.h"
#include "cc/hdr/misc/GLreturns.h"      // for GLnoMemory
#include "cc/hdr/eh/EHhandler.hh"
#include "cc/hdr/tim/TMtmrExp.hh"
#include "cc/hdr/msgh/MHgq.hh"
//#include "cc/hdr/su/SUexitMsg.hh"

#include "cc/hdr/db/DBsqlMsg.hh"
#include "cc/hdr/db/DBreconnectMsg.hh"
#include "cc/hdr/db/DBmtype.hh"
//#include "cc/hdr/db/DBassert.hh"
#include "cc/hdr/db/DBlimits.hh"
//#include "cc/hdr/db/DBdebug.hh"
#include "cc/hdr/db/DButil2.hh"

//#include "cc/hdr/cr/CRdebugMsg.hh"
//#include "cc/hdr/cr/CRdbCmdMsg.hh"

#include "DBI.hh"
#include "DBIsyncMsg.hh"
#include "DBIprocessMsg.hh"
#include "DBIhpr.hh"
#include "DBIhprMsg.hh"
#include "DBItime.hh"

// for INIT
#include "cc/hdr/init/INusrinit.hh"
#include "cc/hdr/init/INinitialize.hh"


Long DBImeasSqlMsgNo;
Long DBImeasSqlSentNo;
extern MHqid DBImhqid;	// mhqid associated with the helper process
extern MHqid DBIglbMhqid;
extern EHhandler DBIeventHandler;

extern "C" Void DBIterminate(int);
void newHandler();
Long DBIgetTableName(DBsqlMsg *, Char *);
Void DBIprocessSqlMsg(DBImsgNodeTyp*&, DBImsgList&, DBIhelpersStatus&);
Void DBIsendSqlToHelper(DBImsgList&, DBIhelpersStatus&);
void DBIsendToHelpers(MHmsgBase*, DBIhelpersStatus&);

extern Short DBIrtnMsg(DBsqlMsg*, Long, const Char*); //return a sqlAck message 
extern Short DBIregister(const Char *, MHregisterTyp);
extern Short DBIglobalReg(const Char *);
extern Short DBIunRegister(const Char *);
extern Void DBItimerInit();
extern DBRETVAL DBIinitFreeNode();
extern DBImsgNodeTyp *DBIgetFreeNode();
extern Void DBIcleanupFreeNode();

Short
sysinit(Long, Char **, SN_LVL , U_char)
{
  printf("------dbi sysinit enter--");
  return(GLsuccess);
}


Short
procinit(Long, Char *argv[], SN_LVL sn_lvl, U_char)
{
  //set_new_handler(newHandler);

  //CRERRINIT(argv[0]);

  // register with MSGH 
  // the main process is called "DBI" 
  // local Queue name - "_DBI"
  // global queue name - "DBI".
  std::string localQname = "_";
  localQname += argv[0];

  if (DBIregister(localQname.c_str(), MH_GLOBAL) == DBFAILURE)
  {
    //DBIGENLERROR(DBrgstMSGHFail, (DBrgstMSGHFailFmt));
    printf("DBrgstMSGHFail\n");
    return(GLfail);
  }

  // initialize message node storage 
  if (DBIinitFreeNode() != DBSUCCESS)
  {
    //DBIGENLERROR(DBinitnodeFail, (DBinitnodeFailFmt));
    printf("DBinitnodeFail\n");
    return(GLfail);
  }
  // register for Global Queue 
  if (DBIglobalReg(argv[0]) != DBSUCCESS)
  {
    //DBIGENLERROR(DBrgstMSGHFail, (DBrgstMSGHFailFmt));
    printf("DBrgstMSGHFail\n");
    return(GLfail);
  }

  //Initiate timer for DBI. 
  DBItimerInit();

  DBImeasSqlMsgNo = 0;
  DBImeasSqlSentNo = 0;

  return(GLsuccess);
}


// main routine to process the incoming messages
// messages could be SQL messages or DBIhprMsg (helper ready)
// A SQL message represents a complete SQL UPDATE, INSERT and DELETE will
// be processed right away.
// An incomplete SQL message (with endFlag == 0) will be enter into waiting 
// list. A complete SQL will be sent to a ready helper or moved to the ready
// list when no helper is ready.
Void 
process(Long, Char *argv[], SN_LVL, U_char )
{
  DBIhelpersStatus platdbHelpers("DBIhpr"), oracleHelpers("DBIoraHpr");
  DBImsgList platdbMsgList, oracleMsgList;
  MHmsgBase *msgp;
  DBImsgNodeTyp * msgNode = (DBImsgNodeTyp *)0;
  static Short msgsz;
  GLretVal rtn=0;
  MHqid SPMANmhQid = MHnullQ;

  //INIT will not call cleanup(). We will have to catch the 
  //SIGTERM and call the cleanup() manually.
  signal(SIGTERM, DBIterminate);


  while (1)
  {
    if (msgNode == (DBImsgNodeTyp *)0)
    {
      if ((msgNode = DBIgetFreeNode()) == (DBImsgNodeTyp *)0)
      {
        //DBIGENLERROR(DBmemoryOut, (DBmemroyOutFmt) );
        printf("DBIENLERROR\n");
        sleep(2);
        continue;
      }
    }

    msgp = (MHmsgBase *) (msgNode->msgBody);

    msgsz = MHmsgSz;  // reset every time
    rtn = DBIeventHandler.getEvent(DBImhqid, (Char *)msgNode->msgBody, msgsz);

    switch( rtn )
    {
    case MHintr:
    case MHtimeOut:
      continue;
    case GLsuccess:
      break;
            
      // If something has gone wrong with the message queue, then no 
      // further retries can be made -- exit the process so that 
      // proper recovery can occur
    case MHnoQue:
    case MHidRm:
    case MHbadQid:
    case MHbadName:
       {
         //CRERROR("process(): MHmsgh.receive failed due to queue problems Error=%d", rtn);
         printf("process(): MHmsgh.receive failed due to queue problems Error=%d\n");

         (Void) cleanup(7, argv, (SN_LVL)7 , (U_char)7);
         INITREQ(SN_LV0, rtn, "Message Queue Error", IN_EXIT);
         // Not falling through due to INITREQ().
       }
    case MH2Big:
       {
         //CRERROR("process(): msgbody[] is greater than msgsz, "
         //        "srcQue(%s), msgType(%d), msgSz(%d), expected(%d)",
         //        msgp->srcQue.display(), msgp->msgType, msgp->msgSz, msgsz);
         printf("process(): msgbody[] is greater than msgsz, "
                 "srcQue(%s), msgType(%d), msgSz(%d), expected(%d)\n",
                 msgp->srcQue.display(), msgp->msgType, msgp->msgSz, msgsz);
         continue;
       }
    default :
       {
         //CRERROR("process(), MHmsgh.receive failed. Return value:%d, "
         //        "msgSize:%d, srcQue:%s", rtn, msgsz, msgp->srcQue.display());
         printf("process(), MHmsgh.receive failed. Return value:%d, "
                 "msgSize:%d, srcQue:%s\n", rtn, msgsz, msgp->srcQue.display());
         continue;
       }
    }

    switch (msgp->msgType) {
    case DBupdateTyp :
    case DBinsertTyp :
    case DBdeleteTyp :
    case DBselectTyp :
       {
         DBImeasSqlMsgNo++;
         DBsqlMsg *sqlp = (DBsqlMsg*) msgp;
         Long sqlLen = sqlp->sqlSz;
    
         if ((sqlLen <= 0) || (sqlLen >= DBsqlSz))
         {
           //CRERROR("process(): Wrong SQL message size %d",sqlLen);
           printf("process(): Wrong SQL message size %d\n",sqlLen);
           DBIrtnMsg(sqlp, DBI_MESSAGEINVALID, "Wrong SQL length. Ignored.");
           continue;
         }

         sqlp->sql[sqlLen] = '\0';
         //CRDEBUG(DBmsgreceived,("process(): rcv msg srcQue %s, sid %d, endFlag %d,"
         //                       " sqlsz %d: %*s, tblOffset %d, whereOffset %d", 
         //                       sqlp->srcQue.display(), sqlp->sid, sqlp->endFlag, 
         //                       sqlp->sqlSz, sqlp->sqlSz, sqlp->sql, sqlp->tblOffset, 
         //                       sqlp->whereOffset));
         printf("process(): rcv msg srcQue %s, sid %d, endFlag %d,"
                " sqlsz %d: %*s, tblOffset %d, whereOffset %d\n", 
                sqlp->srcQue.display(), sqlp->sid, sqlp->endFlag, 
                sqlp->sqlSz, sqlp->sqlSz, sqlp->sql, sqlp->tblOffset, 
                sqlp->whereOffset);

         Char tblName[DB_TBL_NAME_LEN * 2 + 2];  /* include "<schema>." */
         if ((DBIgetTableName(sqlp, tblName) == DBFAILURE))
         {
           if (platdbMsgList.fdWaitLst(sqlp->srcQue,sqlp->sid)!=NULL)
           {
             DBIprocessSqlMsg(msgNode, platdbMsgList, platdbHelpers);
           }
           else if(oracleMsgList.fdWaitLst(sqlp->srcQue,sqlp->sid)!=NULL)
           {
             DBIprocessSqlMsg(msgNode, oracleMsgList, oracleHelpers);
           }
           else
           {
             //invalid sql or invalid table name
             //Send error message back
             DBIrtnMsg(sqlp, DBI_TABLENAMEINVALID, "Table name invalid");
           }
         }
         else
         {
           int dbms = DB_UNKNOWN;
           Bool isSpaTable = false;
           if (DBIeventHandler.getMhqid("SPMAN", SPMANmhQid) == GLsuccess)
           {
             //isSpaTable = DBisSpaTable(tblName, dbms);
             isSpaTable = FALSE;
           }

           if (isSpaTable && dbms == DB_PG || !isSpaTable)
              DBIprocessSqlMsg(msgNode, platdbMsgList, platdbHelpers);
           else if (dbms == DB_ORACLE)
              DBIprocessSqlMsg(msgNode, oracleMsgList, oracleHelpers);
           else
              DBIrtnMsg(sqlp, DBI_TABLENAMEINVALID, "Table not found in either platdb or Oracle");
         }

         break;
       }

    case DBIhprMsgTyp:
       {
         DBIhprMsg *hprMsgp = (DBIhprMsg*)msgp;
         //CRDEBUG(DBmsgreceived,("process(): rcv msg srcQue %s, helper name %s,"
         //                       " helper status: %d", hprMsgp->srcQue.display(), 
         //                       hprMsgp->name, hprMsgp->state));
         printf("process(): rcv msg srcQue %s, helper name %s,"
                " helper status: %d\n", hprMsgp->srcQue.display(), 
                hprMsgp->name, hprMsgp->state);
    
         if(strncmp(hprMsgp->name, "DBIhpr", 6) == 0)
         {
           platdbHelpers.setHelperStatus(hprMsgp);
           DBIsendSqlToHelper(platdbMsgList, platdbHelpers);
         }
         else if(strncmp(hprMsgp->name, "DBIoraHpr", 9) == 0)
         {
           oracleHelpers.setHelperStatus(hprMsgp);
           DBIsendSqlToHelper(oracleMsgList, oracleHelpers);
         }
         break;
       }

    case TMtmrExpTyp:
       {
         TMtmrExp *timerp = (TMtmrExp*)msgp;
         U_long tmrTyp = timerp->tmrTag;

         if (tmrTyp != DBIsanityTmrTyp)
         {
           //CRERROR("process(): Bad timer type(%d) encountered", tmrTyp);
           printf("process(): Bad timer type(%d) encountered\n");
           continue;
         }

         IN_SANPEG();

         //CRDEBUG(DBaudit, ("Received total: %d, sent total: %d",
         //                  DBImeasSqlMsgNo, DBImeasSqlSentNo));
         printf("Received total: %d, sent total: %d\n",
                DBImeasSqlMsgNo, DBImeasSqlSentNo);

         platdbHelpers.audit();
         oracleHelpers.audit();

         break;
       }
//    case SUexitTyp:
//       {
//         (Void) cleanup(7, argv, (SN_LVL)7 , (U_char)7);
//
//         exit(0);
//         break;
//       }
//
//    case CRdbCmdMsgTyp:
//       {
//         CRdbCmdMsg *cr = (CRdbCmdMsg *)msgp;
//         cr->unload();
//         //Continue to broadcast the message to all the helpers
//       }
    case DBreconnectTyp:
       {
         //Broadcast the message to all the helpers
         DBIsendToHelpers(msgp, platdbHelpers);
         DBIsendToHelpers(msgp, oracleHelpers);
         break;
       }
        
    case MHgqRcvTyp:
       {
         MHgqRcv *p = (MHgqRcv *) msgp;
         MHgqRcvAck ack(p->gqid);
         if ((rtn = ack.send(p->srcQue)) != GLsuccess)
         {
           //CRERROR("process(): Failed to send MHgqRcvAck: rtn=%d", rtn);
           printf("process(): Failed to send MHgqRcvAck: rtn=%d\n", rtn);
           INITREQ(SN_LV0, rtn, "FAILED to ACK MHgqRcv", IN_EXIT);
         }
         break;
       }
                
    case INinitializeTyp:
       {
         INinitializeAck ack;
         if ((rtn = ack.send(DBIglbMhqid)) != GLsuccess)
         {
           //CRERROR("process(): Failed to send INinitializedAck: rtn=%d", rtn);
           printf("process(): Failed to send INinitializedAck: rtn=%d\n");
         }
         break;
       }

    default:
       {
         //CRDEBUG(DBmsgreceived,("process(): received unknown type message,msgType: %d",msgp->msgType));
         printf("process(): received unknown type message,msgType: %d\n",msgp->msgType);
         break;
       }
    }
  }
}


//init will not call cleanup. We have to catch the SIGTERM
//and do some cleanup manually
Void DBIterminate(int)
{
  DBIcleanupFreeNode();
}


//
// handler for memory exhaustion exception
//
Void newHandler()
{
  DBIcleanupFreeNode();
  // DBI process will die and get reinit'ed
  //CRERROR("OUT OF MEMORY exception caught");
  printf("OUT OF MEMORY exception caught\n");
  INITREQ(SN_LV0, GLnoMemory, "OUT OF MEMORY", IN_EXIT);
}


Short
cleanup(Long, Char *argv[], SN_LVL, U_char)
{
  // remove process name
  if (DBIunRegister(argv[0]) == DBFAILURE)
  {
    return(GLfail);
  }
  return(GLsuccess);
}


Void DBIprocessSqlMsg(DBImsgNodeTyp *&msgNode, DBImsgList &msgList, DBIhelpersStatus &helpers)
{
  //CRDEBUG(DBinout, ("Enter DBIprocessSqlMsg()"));
  printf("Enter DBIprocessSqlMsg()\n");
  DBsqlMsg * sqlp = (DBsqlMsg*)msgNode->msgBody;
    
  if (sqlp->endFlag == TRUE)
  {
    //Received a complete SQL message. 
    //Put in ready list and send to helpers.
    msgList.entRdyLst(msgNode);
    msgNode = (DBImsgNodeTyp *)0;

    Short helperIndex;
    while (msgList.getRdyLst() != (DBImsgNodeTyp *)0 &&
           (helperIndex = helpers.getReadyHelper()) != -1)
                   
    {
      msgList.sndSqlToHelper(helpers, helperIndex);
    }

    if (msgList.getRdyLst() == (DBImsgNodeTyp *)0)
    {
      //CRDEBUG(DBaudit, ("DBIprocessSqlMsg(): No more message to be sent"));
      printf("DBIprocessSqlMsg(): No more message to be sent\n");
    }
    else if (helperIndex == -1)
    {
      //CRDEBUG(DBaudit, ("DBIprocessSqlMsg(): No more helper is available"));
      printf("DBIprocessSqlMsg(): No more helper is available\n");
    }
  }
  else 
  {
    if (msgList.entWaitLst(msgNode) == DBFAILURE)
    {
      //CRERROR("DBIprocessSqlMsg(): SQL too large. Exceed the max message nodes.");
      printf("DBIprocessSqlMsg(): SQL too large. Exceed the max message nodes.\n");
      DBIrtnMsg(sqlp, DBI_SQLTOOLONG, "SQL too large.");
    }
    else
    {
      // we need a new node, reset msgNode
      msgNode = (DBImsgNodeTyp *)0;
    }
  }
  //CRDEBUG(DBinout, ("Exit DBIprocessSqlMsg()"));
  printf("Exit DBIprocessSqlMsg()\n");
}


Void DBIsendSqlToHelper(DBImsgList &msgList, DBIhelpersStatus &helpers)
{
  //CRDEBUG(DBinout, ("Enter DBIsendSqlToHelper()"));
  printf("Enter DBIsendSqlToHelper()\n");
  Short helperIndex;
  while (msgList.getRdyLst() != (DBImsgNodeTyp *)0 && 
         (helperIndex=helpers.getReadyHelper()) != -1)
               
  {
    msgList.sndSqlToHelper(helpers, helperIndex);
  }
  //CRDEBUG(DBinout, ("Exit DBIsendSqlToHelper()"));
  printf("Exit DBIsendSqlToHelper()\n");
}

void DBIsendToHelpers(MHmsgBase *msgp, DBIhelpersStatus &helpers)
{
  MHqid helperQid;
  Long  msgLen;
  Short helperIndex, i;

  switch (msgp->msgType)
  {
//  case CRdbCmdMsgTyp:
//    msgLen = sizeof(CRdbCmdMsg);
//    break;
  case DBreconnectTyp:
    msgLen = sizeof(DBreconnectMsg);
    break;
  default:
    //CRERROR("DBIsendToHelpers(): Wrong message type");
    printf("DBIsendToHelpers(): Wrong message type\n");
    return;
  }

  for (helperIndex=0; helperIndex<helpers.getTotalNumber(); helperIndex++)
  {
    helperQid = helpers.getHelperQid(helperIndex);
    if (helperQid != MHnullQ)
    {
      i = 0;
      while ((msgp->send(helperQid, DBImhqid, msgLen, 0)) == MHintr && 
             (i++ < DBIsqlAckSendTries))
         ;
    }
  }
}

