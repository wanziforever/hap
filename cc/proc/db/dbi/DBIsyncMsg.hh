#ifndef __DBISYNCMSG_H
#define __DBISYNCMSG_H
/*
**
**	File ID:	@(#): <MID7028 () - 08/17/02, 29.1.1.1>
**
**	File:					MID7028
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:21:47
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**		Message between DBI main and DBIhelpers.
**		DBI main want to "synchronize" the DBI helper.
**		To inform DBIhelpers a new start of SELECT messages to be coming
**		and helper will forget the old sql messages.
**		This is because DBI may not be able to send all message belonging to
**		a SELECT to a helper due to unexpected reason. The DBI helper need to
**		be "sync" when DBI main tries to send a next statement to it.
**		Otherwise, the DBI helper may still expect the messages from last statement.
**
** OWNER:
**	 Yeou H. Hwang
**
** NOTES:
*/

#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/db/DBmtype.hh"
 
  
class DBIsyncMsg : public MHmsgBase {
public :
		DBIsyncMsg();
		GLretVal send(MHqid toQid, MHqid srcQid, Long time);
		// Msg format
};

#endif
