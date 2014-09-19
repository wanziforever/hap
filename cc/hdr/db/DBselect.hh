#ifndef __DBSELECT_H
#define __DBSELECT_H

/*
**
**	File ID:	@(#): <MID4661 () - 12/11/90, 2.1.1.3>
**
**	File:					MID4661
**	Release:				2.1.1.3
**	Date:					1/17/91
**	Time:					19:14:50
**	Newest applied delta:	12/11/90
**
** DESCRIPTION:
** 	Definition of message classes DBselect and DBselectAck.
**
**	DBselect is used to send a SELECT statement to DBI.
**
**	DBselectAck is used to send the results of a SELECT statement.
**	The retrieved values are returned as name-value pairs.  Each name-value
**	pair has four fields: the length of column name, the name itself,
**	the length of the value and the value.  The reqAck field is used to
**	indicate that DBI requests the receiving process to send back an Ack
**	(DBackBack).
**
** OWNER:
**	Yeou H. Hwang
**
** NOTES:
*/


#include "cc/hdr/msgh/MHqid.hh"
#include "cc/hdr/db/DBlimits.hh"
#include "cc/hdr/db/DBsqlMsg.hh"

const Short DBdataSz = MHmsgLimit - MHmsgBaseSz - DB_SQLERROR_LEN - 12;


class DBselect : public DBsqlMsg
{
	public :
		DBselect();
};

class DBselectAck : public MHmsgBase
{
	public :
		DBselectAck();
		GLretVal send(MHqid toQid, MHqid srcQid, Short len, Long time);

		const Char *dump() const;  // returns a string representation

	public :
		Long  ackCode;  // ack. code from SELECT operation
		Short sid;	// statement id
		Short npairs;   // number of name-value pairs in this message
		Char endFlag;   // is this message the end of a statement
		Char reqAck;	// request ack flag (for flow control)
		Char dummy1;	// not used
		Char dummy2;	// not used
		Char ackMessage[DB_SQLERROR_LEN]; // error string if ackCode
		                                  // is not 0
		Char nvlist[DBdataSz];  // name-value list
};

const Short DBselectAckHdrSz = static_cast<Short>(sizeof(DBselectAck) - DBdataSz);
#endif

