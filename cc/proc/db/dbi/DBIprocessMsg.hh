#ifndef __DBISQL_H
#define __DBISQL_H

/*
**
**	File ID:	@(#): <MID7026 () - 08/17/02, 29.1.1.1>
**
**	File:					MID7026
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:21:46
**	Newest applied delta:	06/24/03
**
** DESCRIPTION:This file contains the class definition for DBImsgList class.
**  The class manages two SQL message list. One is waiting list and the other
**  is ready list. The list structure is as follows:
**
**  waitLstHdr-->waitList1<-->waitList2<-->...<-->waitListn<--waitLstTail
**                  |             |                   |
**         waitList1_node2 waitList2_node2 ... waitListn_node2
**                  |             |                   |
**         waitList1_node3 waitList2_node3 ... waitListn_node3
**                  |             |                   |
**                 ...           ...                 ...
**                  |             |                   |
**        waitList1_nodem  waitList2_nodem ... waitListn_nodem
**
**  readyLstHdr-->readyList1<-->readyList2<-->...<-->readyListn<--readyLstTail
**                  |             |                   |
**         readyList1_node2 readyList2_node2 ... readyListn_node2
**                  |             |                   |
**         readyList1_node3 readyList2_node3 ... readyListn_node3
**                  |             |                   |
**                 ...           ...                 ...
**                  |             |                   |
**        readyList1_nodem  readyList2_nodem ... readyListn_nodem
**
**  Messages belonging to an uncomplete SQL statements are stored in the waiting list
**  When the last message of the SQL statement comes, the messages are moved from
**  waiting list to ready list. And then they are sent to Helpers or are concated into
**  one SQL statement.
**
** OWNER:
**	eDB Team
** History
**	Yeou H. Hwang
**	Sheng Zhou(06/24/03):Rename from DBIsql.hh to DBIprocessMsg.hh
**	Lucy Liang (11/05/03): Rewrite in C++ syntax for 61284
**
** NOTES:
*/

#include "cc/hdr/db/DBsqlMsg.hh"
#include "DBIhpr.hh"

typedef struct DBImsgNode
{
	DBImsgNode *next;   // Used only by the list header. Pointing to the
	                    // next list header.
	DBImsgNode *prv;    // Used only by the list header. Pointing to the
	                    // previous list header.
	DBImsgNode *lst;    // Pointing to the next message node in the same
	                    // message list.
	MHqid srcQue;       // source queue id
	Short sid;          // statement id
	Short nmsgs;        // number of messages in the lst pointed by lst
	                    // must be < DBImaxMsgNo
	union
	{
		LongLong align;
		Char msgBody[sizeof(DBsqlMsg)];
	};
} DBImsgNodeTyp;


// Error code in the returned message
// Using the oracle value for backward compatibility 
#define DBI_TABLENAMEINVALID	-903   // table name invalid
#define DBI_SQLTOOLONG	-9000   // SQL statement  too large

#define DBI_MESSAGEINVALID	-100   // sqlSz is invalid in received message

const Short DBImsgNodeNo = 20;	// message nodes that DBI preallocates
const Char  DBImaxMsgNo = 50;	// max number of message for a SQL statement
const Char  DBIbigBufNo = 20;	// big buffer size = DBIbigBufNo*sizeof(DBsqlMsg)

/* IBM skallner 2006/07/05 Changed to Long from Short. No longer fits. */
const Long DBIbigBufSz = DBIbigBufNo *sizeof(DBsqlMsg);

class DBImsgList
{
public:
  DBImsgList();
  ~DBImsgList();
  inline DBImsgNodeTyp *getRdyLst() const {	return(rdyLstHdr);};
  DBImsgNodeTyp *fdWaitLst(MHqid srcQue, Short sid);
  Short entWaitLst(DBImsgNodeTyp *nod);
  Void entRdyLst(DBImsgNodeTyp *nod);
  Short sndSqlToHelper(DBIhelpersStatus &helpers, Short readyHelper);

private:
  Void retFreeNode(DBImsgNodeTyp *retNode) const;
  Short freeLst(DBImsgNodeTyp *lst);
  Void unLnkWaitLst(DBImsgNodeTyp *lst);
  Void unLnkRdyLst();
    	
private:
	DBImsgNodeTyp *waitLstHdr, *waitLstTail;
	DBImsgNodeTyp *rdyLstHdr, *rdyLstTail;
	union
	{
		Long dummy;
		Char sqlBigBuf[DBIbigBufSz];
	};
};

#endif

