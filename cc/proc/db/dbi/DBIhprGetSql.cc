/*
**	File ID: 	@(#): <MID67392 () - 12/13/03, 2.1.1.1>
**
**	File:			MID67392
**	Release:		2.1.1.1
**	Date:			12/15/03
**	Time:			11:26:31
**	Newest applied delta:	12/13/03 15:11:51
**
** DESCRIPTION:
** 	DBIgetSel, called by DBI DBIhelpers to get a SQL statement
**
** OWNER:
** 	eDB team
**
** History:
**	Yeou H Hwang
**	Lucy Liang (05/14/03): Update for f61286. Use global library to connect Database.
**	Lucy Liang (11/21/03): Update for f61284. Handle INSERT, DELETE and UPDATE also.
**
** NOTES:
*/

#include <stdio.h>
#include <string.h>
#include <String.h>
#include <signal.h>
#include <stdlib.h>

//#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/eh/EHhandler.hh"
#include "cc/hdr/tim/TMtmrExp.hh"
#include "cc/hdr/msgh/MHgq.hh"

//#include "cc/hdr/db/DBassert.hh"

// for INIT
#include "cc/hdr/init/INusrinit.hh"
#include "cc/hdr/init/INinitialize.hh"

#include "cc/hdr/db/DBsqlMsg.hh"
#include "cc/hdr/db/DBsql.hh"
#include "cc/hdr/db/DBretval.hh"
//#include "cc/hdr/db/DBdebug.hh"

#include "DBI.hh"
#include "DBIsyncMsg.hh"
//#include "cc/hdr/cr/CRdebugMsg.hh"
//#include "cc/hdr/cr/CRdbCmdMsg.hh"
//#include "cc/hdr/su/SUexitMsg.hh"
#include "cc/hdr/db/DBreconnectMsg.hh"

#include "hdr/mydebug.h"

// predefined global buffer, should be big enough for most SELECT
// The union is used to avoid possible alignment problem.
static union
{
  Long dummy;
  Char sqlBuf[sizeof(DBsqlMsg)+1];
};

extern MHqid DBImhqid;    // mhqid associated with the helper process
extern MHqid DBIglbMhqid;
// Use event handler to handle both MSGH messages and timer
extern EHhandler DBIeventHandler;

extern Void DBIhprRdy();
extern Void DBIhprWorking();
extern Short DBIunRegister(const Char *);
extern Char DBImyName[];

Short DBIhprGetSql(DBsqlMsg *&msgHdr)
{
  CRDEBUG(DBinout, ("Enter DBIhprGetSel()"));
  // The temp buffer is used to receive more messages
  // If a SELECT statement is too long.
  // The union is used to avoid possible alignment problem.
  union
  {
    Long dummy;
    Char tmpBuf[sizeof(DBsqlMsg)+1];
  };
  Short bufSz;
  Short msgsz = sizeof(DBsqlMsg);
  Short currentSz;
  DBsqlMsg *hdrPtr = (DBsqlMsg *)sqlBuf;
  DBsqlMsg *msgPtr;
  Short rtn;    // return value
  extern Void DBIsigChkParent(int );

  // free the storage if it is a dynamic storage
  if (((Char *)msgHdr != sqlBuf) && (msgHdr != 0))
     delete ((Char *)msgHdr);
  bufSz = sizeof(sqlBuf);
    
  while (1)
  {
    // read first message, should be a SELECT message, otherwise discard the msg
    while (1)
    {
      // set alarm to check whether parent process exist
      msgsz = sizeof(DBsqlMsg);
      rtn=DBIeventHandler.getEvent(DBImhqid, sqlBuf, msgsz);

      switch( rtn )
      {
      case MHintr:
      case MHtimeOut:
        continue;

      case GLsuccess:
        break;

        // If something has gone wrong with the message queue, then 
        // no further retries can be made -- exit the process so that 
        // a recovery can occur
      case MHnoQue:
      case MHidRm:
      case MHbadQid:
      case MHbadName:
         {
           Char * nameptr[1];
           CRERROR("DBIgetSel: MHmsgh.receive failed due to queue problems Error=%d\n", rtn);
        
           cleanup(7, nameptr, (SN_LVL)7 , (U_char)7);
         }
        
         INITREQ(SN_LV0, rtn, "FAILED to ACK MHgqRcv", IN_EXIT);
        
      default :
        CRERROR("DBIhprGetSql, MHmsgh.receive failed %d", rtn);
        return(DBFAILURE);
      }


      if (hdrPtr->msgType == DBselectTyp ||
          hdrPtr->msgType == DBinsertTyp ||
          hdrPtr->msgType == DBdeleteTyp ||
          hdrPtr->msgType == DBupdateTyp)  // check msg type
      {
        hdrPtr->sql[hdrPtr->sqlSz] = '\0';  // append null char
        currentSz = DBsqlMsgHdrSz + hdrPtr->sqlSz;

        DBsqlMsg *sqlp = (DBsqlMsg*) hdrPtr;
        CRDEBUG(DBmsgreceived, ("Got sql message. srcQue %s, sid %d, "
                                "endFlag %d, sqlsz %d, sql %s, tblOffset %d, whereOffset %d",
                                sqlp->srcQue.display(), sqlp->sid, sqlp->endFlag, sqlp->sqlSz,
                                sqlp->sql, sqlp->tblOffset, sqlp->whereOffset));
        break;   //break the while loop
      }

      //if (hdrPtr->msgType == SUexitTyp)
      //{
      //  DBIunRegister(DBImyName);
      //  DBdisconnect();
      //  exit(0);
      //}

      if (hdrPtr->msgType == DBIsyncMsgTyp)  
         continue;    // new start message, to the end of the while loop

      if (hdrPtr->msgType == DBIqryStatMsgTyp)
      {
        CRDEBUG(DBmsgreceived, ("Got qryStatMsgTyp. srcQue %s", 
                                hdrPtr->srcQue.display()));
        DBIhprRdy();
        continue;
      }

      if (hdrPtr->msgType == TMtmrExpTyp)
      {
        TMtmrExp *timerp = (TMtmrExp*)hdrPtr;
        U_long tmrTyp = timerp->tmrTag;

        if (tmrTyp != DBIsanityTmrTyp)
        {
          CRERROR("Bad timer type(%d) encountered", tmrTyp);
          continue;
        }

        // peg INIT sanity count
        CRDEBUG(DBmsgreceived, ("Time to peg sanity"));
        IN_SANPEG();
      }

      if (hdrPtr->msgType == MHgqRcvTyp)
      {
        CRDEBUG(DBmsgreceived, ("Got MHgqRcvTyp message"));
        MHgqRcv *p = (MHgqRcv *)sqlBuf;
        MHgqRcvAck ack(p->gqid);
        if ((rtn = ack.send(p->srcQue)) != GLsuccess)
        {
          CRERROR("Failed to send MHgqRcvAck: rtn=%d", rtn);
          INITREQ(SN_LV0, rtn, "FAILED to ACK MHgqRcv", IN_EXIT);
        }
        continue;
      }
                
      if (hdrPtr->msgType == INinitializeTyp)
      {
        INinitializeAck ack;
        if ((rtn = ack.send(DBIglbMhqid)) != GLsuccess)
        {
          CRERROR("Failed to send INinitializedAck: rtn=%d", rtn);
        }
        continue;
      }

      //if (hdrPtr->msgType == CRdbCmdMsgTyp)
      //{
      //  CRdbCmdMsg *cr = (CRdbCmdMsg *)hdrPtr;
      //  cr->unload();
      //  continue;
      //}

      if (hdrPtr->msgType == DBreconnectTyp)
      {
        DBdisconnect();
        if (DBconnect() == DBFAILURE)
        {
          Short z;
          for (z = 0; z < 4; z++)
          {
            //DBIGENLERROR(DBconnDbTry, (DBconnDbTryFmt) );
            printf("DBconnDbTry\n");
            if (DBconnect() == DBSUCCESS)
            {
              break;
            }
          }
          if (z >= 4)
          {
            //DBIGENLERROR(DBconnDbFail, (DBconnDbFailFmt) );
            printf("DBconnDbFail\n");
            exit(1);
          }
        }
      }
      else
      {
        CRDEBUG(DBmsgreceived, ("DBIhprGetSel(): get  message type:%d.Ignore it.", hdrPtr->msgType));
      }
    }

    // now we received a SQL message

    if (hdrPtr->endFlag == TRUE) // ready to return
    {
      msgHdr = hdrPtr;
      CRDEBUG(DBinout, ("Exit DBIhprGetSql() normally."));
      return(DBSUCCESS);
    }

    // otherwise, loop for the end message of the statement.
    msgPtr = (DBsqlMsg *) tmpBuf;
    while (1)
    {
      // read another message
      msgsz = sizeof(DBsqlMsg);
      if ((rtn = MHmsgh.receive(DBImhqid, tmpBuf, msgsz, 0, -1))
          != GLsuccess)
      {
        IN_SANPEG();
        if (rtn == MHintr)
           continue;

        CRERROR("DBIgetSel, MHmsgh.receive failed with return code %d", rtn);
        return(DBFAILURE);
      }

      // is this a sync message (indicate to forget previously received messages
      // peg INIT snity count 
      IN_SANPEG();

      //if (msgPtr->msgType == SUexitTyp)
      //{
      //  DBIunRegister(DBImyName);
      //  DBdisconnect();
      //  exit(0);
      //}

      if (msgPtr->msgType == DBIsyncMsgTyp)
         break;    // break this loop, go back to the first loop

      if (msgPtr->msgType == DBIqryStatMsgTyp)
      {
        CRDEBUG(DBmsgreceived, ("Got qryStatMsgTyp. srcQue %s", 
                                msgPtr->srcQue.display()));
        DBIhprWorking();
        continue;
      }

      //if (msgPtr->msgType == CRdbCmdMsgTyp)
      //{
      //  CRdbCmdMsg *cr = (CRdbCmdMsg *)hdrPtr;
      //  cr->unload();
      //  continue;
      //}

      if (msgPtr->msgType == DBreconnectTyp)
      {
        DBdisconnect();
        if (DBconnect() == DBFAILURE)
        {
          Short z;
          for (z = 0; z < 4; z++)
          {
            //DBIGENLERROR(DBconnDbTry, (DBconnDbTryFmt) );
            printf("DBconnDbTry\n");
            if (DBconnect() == DBSUCCESS)
            {
              break;
            }
          }
          if (z >= 4)
          {
            //DBIGENLERROR(DBconnDbFail, (DBconnDbFailFmt) );
            printf("DBconnDbFail\n");
            exit(1);
          }
        }
      }

      if (msgPtr->msgType != DBselectTyp &&
          msgPtr->msgType != DBinsertTyp &&
          msgPtr->msgType != DBdeleteTyp &&
          msgPtr->msgType != DBupdateTyp)  // check msg type
      {
        CRDEBUG(DBmsgreceived, ("DBIhprGetSel(): get  message type:%d."
                                "Ignore it.", msgPtr->msgType));
        continue;
      }

      // this is a right message
      msgPtr->sql[msgPtr->sqlSz] = '\0';
      CRDEBUG(DBmsgreceived, ("Got sql message:srcQue=%s, sid=%d, "
                              "endFlag=%d, sqlsz=%d, sql=%s", msgPtr->srcQue.display(), 
                              msgPtr->sid, msgPtr->endFlag, msgPtr->sqlSz, msgPtr->sql));

      if ((msgPtr->msgType != hdrPtr->msgType) ||
          (msgPtr->sid != hdrPtr->sid) ||
          (msgPtr->srcQue != hdrPtr->srcQue))
      {
        CRDEBUG(DBmsgreceived, ("discard the message. msgType=%d,"
                                "first msgType=%d; sid=%d, first sid=%d; srcQue=%s, "
                                "first srcQue=%s", msgPtr->msgType, hdrPtr->msgType, 
                                msgPtr->sid, hdrPtr->sid,
                                msgPtr->srcQue.display(), hdrPtr->srcQue.display()));
        continue;  // discard this message, if is not a correct message
      }


      if ( currentSz + msgPtr->sqlSz >= bufSz)
      {
        // buffer is not big enough
        // need to reallocate buffer space
        // this code should not be executed usually
        Char *tmp;
        Short size;

        bufSz = currentSz + 3*sizeof(DBsqlMsg);
        if ( (tmp= new Char[bufSz]) == 0)
        {
          //DBIGENLERROR(DBImallocFail, (DBImallocFailFmt) );
          printf("DBImallocFail\n");
          return(DBFAILURE);
        }
        memcpy(tmp, (Char *)hdrPtr, currentSz); // copy content
        tmp[currentSz] = '\0';

        // free the storage if it is a dynamic storage
        if ((Char *)hdrPtr != sqlBuf)
           delete ((Char *)hdrPtr);
                
        hdrPtr = (DBsqlMsg *) tmp;
      }

      // concatenate the sql text
      strcat(hdrPtr->sql, msgPtr->sql);
      currentSz += msgPtr->sqlSz;

      if (msgPtr->endFlag == TRUE) // ready to return
      {
        msgHdr = hdrPtr;
        CRDEBUG(DBinout, ("Exit DBIhprGetSql() normally."));
        return(DBSUCCESS);
      }
    }
  }
}
