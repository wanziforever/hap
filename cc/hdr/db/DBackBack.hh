#ifndef __DBACKBACK_H
#define __DBACKBACK_H

/*
**
**	File ID:	@(#): <MID4655 () - 10/16/90, 2.1.1.2>
**
**	File:					MID4655
**	Release:				2.1.1.2
**	Date:					1/17/91
**	Time:					19:14:44
**	Newest applied delta:	10/16/90
**
** DESCRIPTION:
**
** 	Definition of message class DBackBack.
**
**	The DBackBack message is used by other subsystems to send an
**	acknowledgement to DBI if this is requested by DBI.
**	When DBI sends a DBselectAck, it may request an ack by setting
**	reqAck to 1 in the DBselectAck.
**
** OWNER:
**	Yeou H. Hwang
**
** NOTES:
*/


#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHpriority.hh"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/db/DBmtype.hh"

class DBackBack : public MHmsgBase
{
	public :
		DBackBack();
		GLretVal send(MHqid toQid, MHqid srcQid, Long time);
	public :
		Short sid;	// statement identifier
};

const Short DBackBackSz = static_cast<Short>(sizeof(DBackBack));
#endif
