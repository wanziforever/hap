#ifndef __DBSQLMSG_H
#define __DBSQLMSG_H

/*
**
**	File ID:	@(#): <MID4658 () - 12/11/90, 2.1.1.3>
**
**	File:					MID4658
**	Release:				2.1.1.3
**	Date:					1/17/91
**	Time:					19:14:47
**	Newest applied delta:	12/11/90
**
** DESCRIPTION:
** 	Definition of message classes DBsqlMsg and DBsqlAck
**
** OWNER:
**	Yeou H. Hwang
**
** NOTES:
** A sql statement is identified by a distinct id (sid).  After DBI executes
** the sql statement, it will use the sid to send back the acknowledgements.
** A long sql statement may be sent in multiple messages if the length of the
** statement is larger than DBsqlSz.  The endFlag value is 1 if it is the last
** message of a statement.  Otherwise the endFlag value is 0.
** The sqlSz value indicates the size of the sql text in this message.
** DBI will reassemble the messages into a statement.
** The tblOffset value is the offset of the table name, and whereOffset value
** is the offset of the where clause, from the beginning of the sql statement.
** If there is no where cluse in the sql statement, the whereOffset value is 0.
** If a sql statement is split into multiple messages, only the first message
** needs to set the values of tblOffset and whereOffset.
**
** Note: The tblOffset and whereOffset values are offsets to the whole sql
** statement, not to the sql text in a message.
*/


#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHpriority.hh"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/db/DBmtype.hh"
#include "cc/hdr/db/DBsql.hh"

const Short DBsqlSz = MHmsgSz - MHmsgBaseSz - 14; // 14 is DBsqlMsgHdr size

class DBsqlMsg : public MHmsgBase
{
public :
  DBsqlMsg();
  GLretVal send(MHqid toQid, MHqid srcQid, Long time);
  GLretVal send(const Char *to, MHqid srcQid, Long time);

public :
  Short sid;           // statement id
  U_short sqlSz;       // size of the sql text in this message
  U_short tblOffset;   // offset to the table name
  U_short whereOffset; // offset to the where clause
  U_short rowLimit;    // number of rows to return in DBselectAck;
  // 0 if all rows retrieved

  Char endFlag;        // 1 if is the end of message; 0 otherwise
  Char dummy1;         // not used
  Char dummy2;         // not used
  Char dummy3;         // not used
  Char sql[DBsqlSz];   // sql statement text
};

class DBsqlAck : public MHmsgBase
{
public :
  DBsqlAck();
  GLretVal send(MHqid toQid, MHqid srcQid, Long time);
// Msg format
public :
  Short sid;	// statement id
  Long ackCode;	// ack. code
  Long nrec;	// n records
  Char ackMessage[DB_SQLERROR_LEN];  // error string in case
  // ackCode is not 0
};

// number of bytes in the DBsqlMsg header
const Short DBsqlMsgHdrSz = static_cast<Short>(sizeof(DBsqlMsg) - DBsqlSz);

const Short DBsqlAckSz = static_cast<Short>(sizeof(DBsqlAck));
#endif
