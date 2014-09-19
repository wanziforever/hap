/*
**	File ID: 	@(#): <MID66149 () - 12/02/03, 2.1.1.2>
**
**	File:			MID66149
**	Release:		2.1.1.2
**	Date:			12/15/03
**	Time:			11:26:32
**	Newest applied delta:	12/02/03 19:00:31
**
** DESCRIPTION: This file implement member functions of class DBImsgBuffer
**              The class encapsulates the functions in former DBIsMsgBuf.C
**
** OWNER:
**	eDB Team
**
** History:
**	Yeou H. Hwang
**	Lucy Liang (05/14/03): created for feature 61286
**
** NOTES:
*/

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "cc/hdr/msgh/MHinfoExt.hh"

// for INIT
#include "cc/hdr/init/INusrinit.hh"

#include "cc/hdr/db/DBsql.hh"
#include "cc/hdr/db/DBsqlMsg.hh"
#include "cc/hdr/db/DBselect.hh"
#include "cc/hdr/db/DBackBack.hh"
//#include "cc/hdr/db/DBassert.hh"
//#include "cc/hdr/db/DBdebug.hh"
//#include "cc/hdr/cr/CRdbCmdMsg.hh"
//#include "cc/hdr/su/SUexitMsg.hh"

#include "DBIqryStat.hh"
#include "DBIselect.hh" // define DbselectAck message buffer list
#include "DBItime.hh"
#include "DBImsgBuffer.hh"

#include "hdr/mydebug.h"

static Short timerStat;

extern MHqid DBImhqid;	// mhqid associated with the helper process
extern Void DBIhprWorking();
extern Short DBIunRegister(const Char *);
extern Char DBImyName[];


// catch a timer, set timeout flag
Void sigproc(int sig)
{
  sig=sig; // to remove compiling warning
  timerStat = DBI_TIMEOUT;
}

// Constructor
DBImsgBuffer::DBImsgBuffer()
{
  msgLstHdr = (DBIselectBufLstTyp *) 0;
  msgLstTail = (DBIselectBufLstTyp *) 0;
  size = 0;
}

// Destructor. It's the former DBIsMsgBufCleanup.
DBImsgBuffer::~DBImsgBuffer()
{
  DBIselectBufLstTyp *lst , *nxt;

  lst = msgLstHdr;
  while (lst != (DBIselectBufLstTyp *) 0)
  {
    nxt = lst->next;
    if (lst->buf != 0)
       delete ((DBselectAck *) lst->buf);
    delete ((DBIselectBufLstTyp *) lst);
    lst =nxt;
  }

  msgLstHdr = (DBIselectBufLstTyp *) 0;

}

// Add buffer to the buffer list. Corresponding pointers
// will be updated also.
Short DBImsgBuffer::addNode()
{
  DBIselectBufLstTyp *lst;
  DBselectAck *msgptr;

  if ((lst = (DBIselectBufLstTyp *) new DBIselectBufLst) == NULL)
     return(DBFAILURE);

  if ( (msgptr = new DBselectAck) == NULL)
  {
    delete lst;
    return(DBFAILURE);
  }

  lst->buf = (Char *) msgptr;
  lst->len = 0;
  lst->next = 0;
  if (msgLstHdr == (DBIselectBufLstTyp *) 0)
  {
    msgLstHdr = lst;
    msgLstTail = lst;
  }
  else
  {
    msgLstTail->next = lst;
    msgLstTail = lst;
  }
  size++;
  return(DBSUCCESS);
}

// preallocate DBI_SELECTBUFNO numbers of DBselectAck messages
Short DBImsgBuffer::init()
{
  Short i;
  for (i=0; i<DBI_SELECTBUFNO; i++)
  {
    if (addNode() == DBFAILURE)
       return(DBFAILURE);
  }
    
  currentMsg = msgLstHdr;
  numBufferUsed = 1;
  return(DBSUCCESS);
}




// check buffer size, try to keep the size = DBI_SELECTBUFNO
Void DBImsgBuffer::reset()
{
  Short count = 0;
  DBIselectBufLstTyp *lst, *last, *nxt;

  currentMsg = msgLstHdr;

  if (size <= DBI_SELECTBUFNO )
  {
    numBufferUsed = 1;
    return;
  }
    
  lst = msgLstHdr;
  for (;count < DBI_SELECTBUFNO-1; count++)
  {
    lst = lst->next;
  }
  last = lst;
  msgLstTail = last;
  lst = lst->next;
  while (lst != 0)
  {
    nxt = lst->next;
    delete ((DBselectAck *) lst->buf);
    delete ((DBIselectBufLstTyp *) lst);
    lst = nxt;
  }
  last->next = 0;
  size = DBI_SELECTBUFNO;
  numBufferUsed = 1;
}
        

// fill in the header information into a DBselectAck
// msg points to a DBselectAck
// msgHdr points to a DBselect statement

Short 
DBImsgBuffer::fillHdrInfo(DBIselectBufLstTyp *lstPtr, const DBsqlMsg *selHdr, 
                          Long ackCode, Char endFlag, U_char reqAck, 
                          Short npairs)
{
  DBselectAck *ptr = (DBselectAck *)lstPtr->buf;

  ptr->msgType = DBselectAckTyp;
  ptr->sid = selHdr->sid;  // copy the sid
  ptr->ackCode = ackCode;
  ptr->ackMessage[0] = '\0';
  ptr->endFlag = endFlag;
  ptr->reqAck = reqAck;
  ptr->npairs = npairs;
  CRDEBUG(DBinout, ("Exit fillHdrInfo(). sid=%d, ackCode=%d, endFlag=%d,\n"
                    "reqAck=%d, npairs=%d", ptr->sid, ptr->ackCode, ptr->endFlag, 
                    ptr->reqAck, ptr->npairs));
  //dump(ptr);
  return(DBSUCCESS);
}


// Get the first buffer in the buffer list.
DBIselectBufLstTyp* DBImsgBuffer::getHead() const
{
  return(msgLstHdr);
}


/* Get the next buffer in the buffer list.                             *
 * If all the buffer has been used then alloc new buffer               *
 * using addNode()                                                     */

DBIselectBufLstTyp* DBImsgBuffer::getNext()
{
  if  (currentMsg == msgLstTail)  // used all available messages
  {
    if (addNode() != DBSUCCESS)
    {
      return (DBIselectBufLstTyp *) 0;
    }
  }
  currentMsg = currentMsg->next;
  numBufferUsed++;
  return(currentMsg);
}



/* Send nmsg of DBselectAck.                                           *
 * If reqAck flag = TRUE, then reqFlag of last message is set to TRUE. *
 * A timer is set for DBackBack or DBabortSelect message.              *
 * Cannot wait for the message forever.  We have to abort the message  *
 * if timeout occurs for DBIackBackRcvTries attempts.                  */

Short DBImsgBuffer::sndSelectAck(Short nmsg, Char reqAck, const DBsqlMsg *selHdr)
{
  static Bool noReqAckSent = FALSE;
  static Bool lastReqAck = FALSE;
  DBIselectBufLstTyp *lst;
  DBselectAck *msgp;
  int cnt;
  Short len;
  Short msgsz;
  Char tmpBuf[MHmsgSz];
  DBackBack *ackp = (DBackBack *) tmpBuf;
  int rtn;
  Short tryno = 0;
  Long  tryTime = 0;
  Short rcv_tryno = 0;
  Char proc_name[40] = "UNKNOWN";

  cnt =1;
  lst = msgLstHdr;

  // do we request a DBackBack from last time
  if (lastReqAck==TRUE)
  {
    // loop for a DBackBack message or DBabortSelect
    while (rcv_tryno < DBIackBackRcvTries)
    {
      // peg INIT sanity count
      IN_SANPEG();
      msgsz = sizeof(tmpBuf);
      if ((rtn=MHmsgh.receive(DBImhqid, tmpBuf, msgsz, 
                              0, DBItimeDBselectAck)) == GLsuccess)
      {
        // peg INIT snity count
        IN_SANPEG();

        // get a message
        // check the message type and sid
        if ((ackp->msgType == DBackBackTyp || 
             ackp->msgType == DBabortSelectTyp) && 
            (ackp->sid == selHdr->sid) &&
            (ackp->srcQue == selHdr->srcQue))
        {
          //cancell timer
          alarm(0);
          break;  // break the while loop
        }
        //else if (ackp->msgType == CRdbCmdMsgTyp)
        //{
        //  CRdbCmdMsg *cr = (CRdbCmdMsg *)ackp;
        //  cr->unload();
        //}
        else if (ackp->msgType == DBIqryStatMsgTyp)
        {
          DBIhprWorking();
        }
        //else if (ackp->msgType == SUexitTyp)
        //{
        //  DBIunRegister(DBImyName);
        //  DBdisconnect();
        //  exit(0);
        //}
        else
        {
          CRDEBUG(DBmsgreceived,("Helper received an unknown message "
                                 "(type = %d, srcQue = %s).", 
                                 ackp->msgType,ackp->srcQue.display()));
        }
      }
      else 
      {
        if (MHmsgh.getName(selHdr->srcQue, proc_name) != GLsuccess)
        {
          CRERROR("Failed to receive from Qid %s "
                  "(rtn = %d: Process not found)",
                  selHdr->srcQue.display(), rtn);
          rcv_tryno = DBIackBackRcvTries;
          break;
        }        
        if (rtn != MHtimeOut)
        {
          rcv_tryno = DBIackBackRcvTries; 
          break;
        }
        else
        {
          rcv_tryno++;    
        }
      }
    }
    if (rcv_tryno >= DBIackBackRcvTries)
    {
      // encounter time out, reset lastReqAck
      lastReqAck = FALSE;
      CRERROR("Failed to receive DBackBack from Qid=%s, process=%s "
              "after %d attempts.  Transaction aborted, rtn=%d.",
              selHdr->srcQue.display(), proc_name, rcv_tryno, rtn);
      return(DBI_SNDTIMEOUT);
    }
    // Abort of transaction requested
    if (ackp->msgType == DBabortSelectTyp) {
      CRDEBUG(DBdbOper,("sndSelectAck() - Abort current "
                        "select transaction requested"));
      // encounter time out, reset lastReqAck
      lastReqAck = FALSE;
      return(DBI_ABORTSELECT);
    }

  }
    
  while (cnt <= nmsg)
  {
    msgp = (DBselectAck *) lst->buf;

    len = DBselectAckHdrSz + lst->len;
/*
  if ((cnt == nmsg) && (reqAck==TRUE))
  msgp->reqAck = TRUE;
  else
  msgp->reqAck = FALSE;
*/
    // should we use -1, there is no deadlock concern 
    // try couple times
    tryno = 0;
    tryTime = 0;

    CRDEBUG(DBmsgsent, ("sndSelectAck(): srcQue=%s, sid %d, npairs %d,"
                        "ackCode %d, reqAck %d, len=%d.", selHdr->srcQue.display(),
                        msgp->sid,msgp->npairs, msgp->ackCode, msgp->reqAck,len));
    //dump(msgp);

    while (1)
    {
      if ((!msgp->endFlag && !msgp->reqAck) || noReqAckSent)
      {
        CRDEBUG(DBclientItfc, ("sndSelectAck(): srcQue=%s, nmsg %d, "
                               "sid %d, npairs %d, ackCode %d, endFlag %d, reqAck %d,"
                               " lst->len %d", selHdr->srcQue.display(), nmsg,
                               msgp->sid, msgp->npairs, msgp->endFlag, msgp->ackCode,
                               msgp->reqAck, lst->len));
      }

      rtn = msgp->send(selHdr->srcQue, DBImhqid, len,tryTime);
      if (rtn == GLsuccess)
      {
        noReqAckSent = (!msgp->endFlag && !msgp->reqAck);
        cnt++;
        lst = lst->next;
        break; // break the inner loop 
      }
      else
      {
        tryno++;
        if (tryno >= DBIsqlAckSendTries)
        {
          CRERROR("Send to qid=%s failed with error=%d, nvlist=%.100s",
                  selHdr->srcQue.display(), rtn, msgp->nvlist);
          // encounter error, reset lastReqAck
          lastReqAck = FALSE;
          return(DBI_SNDFAIL);
        }
        else
        {
          // should we sleep for a while, how long should we sleep
          tryTime = tryno * DBItimeDBselectAck;
          IN_SANPEG();
        }
      }
    }
  }

  // set flag to indicate a DBackBack is expected or not
  lastReqAck = msgp->reqAck;

  return(DBI_SNDCOMPLETE);
}



// send a error message
Short DBImsgBuffer::sndErrMsg(Long errCode, const Char *errMsg, const DBsqlMsg *selHdr)
{
  DBselectAck *ptr = (DBselectAck *)msgLstHdr->buf;

  // endFlag is true 

  ptr->msgType = DBselectAckTyp;
  ptr->sid = selHdr->sid;  // copy the sid
  ptr->ackCode = errCode;
  strcpy(ptr->ackMessage, errMsg);
  ptr->endFlag = TRUE;
  ptr->reqAck = FALSE;
  ptr->npairs = 0;

  msgLstHdr->len = 0;

  // send it
  CRDEBUG(DBclientItfc, ("sndErrMsg(): sid(%d)", ptr->sid));
  return(sndSelectAck((Short)1, (Char)FALSE, selHdr));
}

