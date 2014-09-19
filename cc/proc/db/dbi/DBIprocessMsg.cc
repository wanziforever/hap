/*
**	File ID:	@(#): <MID4669 () - 1/11/91, 2.1.1.4>
**
**	File:					MID4669
**	Release:				2.1.1.4
**	Date:					1/17/91
**	Time:					19:14:13
**	Newest applied delta:	06/24/03
**
** DESCRIPTION:
**	Implement the methods of class DBImsgList and 3 global functions, 
**	which manage the global free node list. Every DBImsgList will 
**	allocate nodes from it.
**
**	Routines:
**		DBIinitFreeNode(); // init the free DBImsgNodeTyp node list
**		DBIgetFreeNode(); // get a node from the free node list
**		DBIcleanupFreeNode(); // free all the node in the free node list
**
**		DBImsgList::DBImsgList(); // Constructor. Init the private data
**		DBImsgList::~DBImsgList(); // Destructor. Free the resources.
**		DBImsgList::retFreeNode(); // return a node to the free node list
**		DBImsgList::entWaitLst(); // enter a node into a SQL waiting list
**		DBImsgList::freeLst(); // free all nodes of a list to the free list
**		DBImsgList::fdWaitLst(); // find a waiting SQL waiting list based on srcQue, sid
**		DBImsgList::unLnkWaitLst() // unlink a list from the waiting list 
**		DBImsgList::unLnkRdyLst() // unlink a list from SELECT ready List
**		DBImsgList::entRdyLst()	// enter a node intoready list
**		DBImsgList::getRdyLst()	// return the first ready list
**		DBImsgList::sndSqlToHelper()// send SQL messages to a helper
**
** OWNER:
**	eDB Team
**
** History
**	Yeou H. Hwang
**	Sheng Zhou(06/23/03):Modified for f61286
**	Sheng Zhou(06/23/03):Change name from DBIsql.C to DBIprocessMsg.C
**	Lucy Liang(11/05/03): Change to C++ syntax for feature 61284
** NOTES:
*/


#include "cc/hdr/db/DBsqlMsg.hh"
#include "cc/hdr/db/DBmtype.hh"
#include "cc/hdr/db/DBretval.hh"
//#include "cc/hdr/db/DBdebug.hh"

#include "DBIprocessMsg.hh"
#include "DBIhpr.hh"
#include "DBIsyncMsg.hh"
#include "DBItime.hh"

//#include "cc/hdr/cr/CRdebugMsg.hh"

#include "hdr/mydebug.h"

//free node list, struct defined in DBIprocessMsg.hh
static DBImsgNodeTyp *DBIfreeNodeLst = 0;  
static Short DBInodeNoCount;		// count the number of message nodes
extern Long DBImeasSqlSentNo;
extern MHqid DBImhqid;	// mhqid associated with the helper process

// init the free node list (DBIfreeNodeLst)
// preallocate DBImsgNodeNo nodes
DBRETVAL DBIinitFreeNode()
{
  Short i;
  DBImsgNodeTyp *tmp;

  CRDEBUG(DBinout, ("Enter DBIinitFreeNode()"));
  DBIfreeNodeLst = (DBImsgNodeTyp *)0;
  for (i = 0; i < DBImsgNodeNo ; i++)
  {
    //memory failure will be handled in newHandler function, which is
    //registered in set_new_handler() in procinit().
    if ((tmp = (DBImsgNodeTyp *) new DBImsgNode) == (DBImsgNodeTyp *)0)
    {
      return(DBFAILURE);
    }
    tmp->next = DBIfreeNodeLst;
    DBIfreeNodeLst = tmp;
  }

  DBInodeNoCount = DBImsgNodeNo ; // current message node count
  CRDEBUG(DBinout, ("Exit DBIinitFreeNode()"));
  return(DBSUCCESS);
}


// get a node from DBIfreeNodeLst
// if not enough node, allocate more
DBImsgNodeTyp *
DBIgetFreeNode()
{
  DBImsgNodeTyp *tmp;

  CRDEBUG(DBinout, ("Enter DBIgetFreeNode()"));
  if (DBIfreeNodeLst != (DBImsgNodeTyp *)0)
  {
    tmp = DBIfreeNodeLst;
    DBIfreeNodeLst = DBIfreeNodeLst->next;
  }
  else 
  {
    // need to allocate more node
    // this should rarely happen

    //debug printf
    CRDEBUG(DBlinkList,("DBIgetFreeNode(): Warning, not enough free node"));

    if ((tmp = (DBImsgNodeTyp *) new DBImsgNode) != (DBImsgNodeTyp *)0)
    {
      DBInodeNoCount++;  // extra node
    }
  }

  CRDEBUG(DBinout, ("Exit DBIgetFreeNode()"));
  return tmp;
}


Void DBIcleanupFreeNode()
{
  CRDEBUG(DBinout, ("Enter DBIcleanupFreeNode()"));
  DBImsgNodeTyp *nextnode;

  while (DBIfreeNodeLst != (DBImsgNodeTyp *)0)
  {
    nextnode = DBIfreeNodeLst->next;
    delete DBIfreeNodeLst;
    DBIfreeNodeLst = nextnode;
  }
  CRDEBUG(DBinout, ("Exit DBIcleanupFreeNode()"));
}



//Constructor. Initialize the private data
DBImsgList::DBImsgList()
  : waitLstHdr(0), waitLstTail(0), rdyLstHdr(0), rdyLstTail(0)
{
}


// Destructor. Free all resources
DBImsgList::~DBImsgList()
{
  Short i;
  DBImsgNodeTyp *nod, *list, *nextnode;

  // cleanup select ready list
  // and  waiting list
  for (i=0; i<2; i++)
  {
    if (i == 0) 
       list = rdyLstHdr;
    else
       list= waitLstHdr;

    while (list != (DBImsgNodeTyp *)0)
    {
      nod = list->lst;
      while (nod != (DBImsgNodeTyp *)0)
      {
        nextnode = nod->lst;
        delete nod;
        nod = nextnode;
      }
      nextnode = list->next;
      delete list;
      list = nextnode;
    }
  }

  waitLstHdr = waitLstTail = (DBImsgNodeTyp *)0;
  rdyLstHdr = rdyLstTail = (DBImsgNodeTyp *)0;
}


// retrun a node to the global free node list.
// Try to maintain the DBImsgNodeNo 
Void 
DBImsgList::retFreeNode(DBImsgNodeTyp *retNode) const
{
  if (DBInodeNoCount > DBImsgNodeNo)  // too many nodes around
  {
    delete retNode;
    DBInodeNoCount--;
  }
  else  // return thr DBIfreeNodeLst 
  {
    retNode->next = DBIfreeNodeLst;
    DBIfreeNodeLst = retNode;
  }
}


// enter a node into the waiting list pointed by list
// this node (nod) belonging to the same SQL messages pointed by the list
// in each node, prv, next link all SQL waiting list
// list points to the nodes of the list belonging to a SQL statement
Short 
DBImsgList::entWaitLst(DBImsgNodeTyp *nod)
{
  DBImsgNodeTyp *list;
  DBImsgNodeTyp *tmp;

  CRDEBUG(DBinout, ("Enter entWaitLst()"));
  nod->srcQue = ((DBsqlMsg *)(nod->msgBody))->srcQue;
  nod->sid = ((DBsqlMsg *)(nod->msgBody))->sid;
  nod->lst = (DBImsgNodeTyp *) 0;
    
  list = fdWaitLst(nod->srcQue, nod->sid);

  if (list == (DBImsgNodeTyp *)0) 
  {
    // nod is the first message of a SQL statement
    nod->nmsgs = 1;
    if (waitLstTail == (DBImsgNodeTyp *)0) 
    {
      // waiting list is empty
      nod->next = (DBImsgNodeTyp *)0;
      nod->prv = (DBImsgNodeTyp *)0;
      nod->lst = (DBImsgNodeTyp *)0;
      waitLstTail= waitLstHdr = nod;
    }
    else
    {
      // add to the tail
      nod->next = (DBImsgNodeTyp *)0;
      nod->prv = waitLstTail;
      nod->lst = (DBImsgNodeTyp *)0;
      waitLstTail->next = nod;
      waitLstTail = nod;
    }
  }
  else if(list->nmsgs+1 > DBImaxMsgNo)
  {
    //DBIGENLERROR(DBmsgNoExceed, (DBmsgNoExceedFmt));
    printf("DBmsgNoExceed\n");
    unLnkWaitLst(list); // unlink and free list
    freeLst(list);
    return(DBFAILURE);
  }
  else
  {
    // this nod is not the first message of a SQL
    list->nmsgs++;
    tmp = list;
    while (tmp->lst != (DBImsgNodeTyp *)0)  //goto the end of the list
       tmp = tmp->lst;
    tmp->lst = nod;
    nod->lst = (DBImsgNodeTyp *)0;
  }

  CRDEBUG(DBinout, ("Exit entWaitLst()"));
  return(DBSUCCESS);    
}



// free all nodes pointed by list to the global free node list.
Short 
DBImsgList::freeLst(DBImsgNodeTyp *list)
{
  DBImsgNodeTyp *tmp;
    
  tmp = list;
  while (tmp != (DBImsgNodeTyp *)0)
  {
    list = tmp->lst;
    retFreeNode(tmp);
    tmp = list;
  }
  return(DBSUCCESS);
}




// base on srcQue and sid to the uncomplete SQL message list
DBImsgNodeTyp *
DBImsgList::fdWaitLst(MHqid srcQue, Short sid)
{
  DBImsgNodeTyp *tmp;

  tmp = waitLstHdr;
  while (tmp != (DBImsgNodeTyp *)0)
  {
    if ((tmp->sid == sid) && (tmp->srcQue == srcQue))
       return(tmp);

    tmp = tmp->next;
  }

  return((DBImsgNodeTyp *)0);
}




// unlink a SQL message list from the waiting list
Void 
DBImsgList::unLnkWaitLst(DBImsgNodeTyp *list)
{
  DBImsgNodeTyp *prv, *nxt;

  if (list == (DBImsgNodeTyp *)0)
     return;

  prv = list->prv;
  nxt = list->next;
  if (prv == (DBImsgNodeTyp *)0)
  {
    waitLstHdr = nxt;
  }
  else
  {
    prv -> next = nxt;
  }


  if (nxt == (DBImsgNodeTyp *)0)
  {
    waitLstTail = prv;
  }
  else
  {
    nxt -> prv =  prv;
  }
  list->prv = list->next = (DBImsgNodeTyp *) 0;
}




// Unlink the first ready list from ready queue.
Void
DBImsgList::unLnkRdyLst()
{
  DBImsgNodeTyp *list;

  if ((list = rdyLstHdr) == (DBImsgNodeTyp *)0)
     return;

  rdyLstHdr = list->next;
  if (rdyLstHdr == (DBImsgNodeTyp *)0)
  {
    //The queue is empty after unlink
    rdyLstTail = (DBImsgNodeTyp *)0;
  }
  else
  {
    rdyLstHdr->prv = (DBImsgNodeTyp *)0;
  }

  list->prv = list->next = (DBImsgNodeTyp *) 0;
}



// enter the nod into the list (list)
// enter the list into the Select Ready list (rdyLst)
Void 
DBImsgList::entRdyLst(DBImsgNodeTyp *nod)
{
  DBImsgNodeTyp *list;
  DBImsgNodeTyp *tmp;

  CRDEBUG(DBinout, ("Enter entRdyLst()"));
  nod->srcQue = ((DBsqlMsg *)(nod->msgBody))->srcQue;
  nod->sid = ((DBsqlMsg *)(nod->msgBody))->sid;
  nod->lst = (DBImsgNodeTyp *) 0;
    
  list = fdWaitLst(nod->srcQue, nod->sid);

  // list could be null in the case that this SELECT only contains a nod 
  if (list == (DBImsgNodeTyp *)0)
  {
    CRDEBUG(DBlinkList,("entRdyLst(): Waiting list is empty"));
    if (rdyLstTail == (DBImsgNodeTyp *)0)  // list is empty
    {
      // is first message in the ready list
      nod->next = (DBImsgNodeTyp *)0;
      nod->prv = (DBImsgNodeTyp *)0;
      rdyLstTail= rdyLstHdr = nod;
    }
    else
    {
      nod->next = (DBImsgNodeTyp *)0;
      nod->prv = rdyLstTail;
      rdyLstTail->next = nod;
      rdyLstTail = nod;
    }
  }
  else 
  {
    // nod is not the first message node
    CRDEBUG(DBlinkList,("entRdyLst(): Waiting list is not empty"));
    tmp = list;
    while (tmp->lst != (DBImsgNodeTyp *)0)
       tmp = tmp->lst;
    tmp->lst = nod;

    // link to the rdyLst
    if (rdyLstTail == (DBImsgNodeTyp *)0)
    {
      // rdyLst is 0
      rdyLstHdr = rdyLstTail = list;
      list->prv = list->next = (DBImsgNodeTyp *)0;
    }
    else
    {
      // link to the end of the rdyLst
      list->next = (DBImsgNodeTyp *)0;
      list->prv = rdyLstTail;
      rdyLstTail->next = list;
      rdyLstTail = list;
    }
    unLnkWaitLst(list);
  }
  CRDEBUG(DBinout, ("Exit entRdyLst()"));
}


//
// Send messages in the first ready list to a ready helper.
// The messages will form a complete SQL statement.
//
Short 
DBImsgList::sndSqlToHelper(DBIhelpersStatus &helpers, Short readyHelper)
{
  CRDEBUG(DBinout, ("Enter sndSqlToHelper()"));
  DBIsyncMsg sync;
  Short rtn;    // return number
  DBsqlMsg *sqlp;
  DBImsgNodeTyp *msgList, *msgNode;
  const char * helperName = helpers.getHelperName(readyHelper);
  Short i = 0;

  if (helpers.getHelperErrPeg(readyHelper))    // need to re-sync the helper
  {
    CRDEBUG(DBaudit, ("sndSqlToHelper(): Need to synchronize %s", helperName));

    while ((rtn = sync.send(helpers.getHelperQid(readyHelper), DBImhqid, 0))
           == MHintr && (i++ < DBIsqlAckSendTries));
    if (rtn == GLsuccess)
    {
      helpers.resetHelperErrPeg(readyHelper);
    }
    else
    {
      helpers.increaseHelperErrPeg(readyHelper, rtn);
      CRERROR("sndSqlToHelper(): Failed to synchronize %s. return code(%d)",
              helperName, rtn);
      return(DBFAILURE);
    }
  }

  msgList = msgNode = rdyLstHdr;
  while (msgNode != (DBImsgNodeTyp *) 0)
  {
    sqlp = (DBsqlMsg *) msgNode->msgBody;

    // use sqlp->srcQue as the sending qid, not the DBImhqid
    // time == 0 to avoid interfering with the cyclic timer
    i = 0;
    while ((rtn = sqlp->send(helpers.getHelperQid(readyHelper),
                             sqlp->srcQue, 0)) == MHintr
           && (i++ < DBIsqlAckSendTries))
       ;

    if (rtn != GLsuccess)
    {
      CRERROR("sndSqlToHelper(): Failed to send message to %s. return value is %d.", 
              helperName, rtn);

      helpers.increaseHelperErrPeg(readyHelper, rtn);    // peg the error
      return(DBFAILURE);
    }
    else
    {
      CRDEBUG(DBmsgsent, ("sndSqlToHelper(): Sent to %s the message\n%s",
                          helperName, sqlp->sql));

      msgNode = msgNode->lst; // next node
    }
  }

  helpers.setHelperStatus(DBIhelperWorking, readyHelper);
  helpers.setHelperStartTime(time(0), readyHelper);
  DBImeasSqlSentNo++;

  // The list is no longer needed.
  unLnkRdyLst();
  freeLst(msgList); // free the whole list

  CRDEBUG(DBinout, ("Exit sndSqlToHelper()"));
  return(DBSUCCESS);
}
