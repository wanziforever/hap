#ifndef __DBRECONNECTMSG_H
#define __DBRECONNECTMSG_H

/*
**	File ID: 	@(#): <MID66065 () - 06/05/03, 2.1.1.1>
**
**	File:			MID66065
**	Release:		2.1.1.1
**	Date:			07/15/03
**	Time:			11:08:37
**	Newest applied delta:	06/05/03 23:49:13
**
** FILE NAME: DBreconnectMsg.hh
**
** PATH: cc/hdr/db
**
** DESCRIPTION:
**	This file defines the DBreconnectMsg class that replaces the
**	DBdiscOracle message class.  With PostgreSQL and Oracle
**	optionally on the platform, this new message makes it more
**	generic.
**
**	On receiving this message, a process is supposed to disconnect
**	from the database and then reconnect.  The purpose is to
**	make sure that all locks are released.  This message is sent
**	by the CLR:DBLOCK,PROC=x CEP.
**
** NOTES:
**
** OWNER:
**	eDB Team
**
** HISTORY:
**	06/04/03 - Original version by Hon-Wing Cheng for Feature 61286
*/

#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/db/DBmtype.hh"

class DBreconnectMsg : public MHmsgBase
{
    public:
        DBreconnectMsg()
        {
           msgType = DBreconnectTyp;
           priType = MHoamPtyp;
           srcQue = MHnullQ;   /* To be changed by sender */
        }

        GLretVal send(const Char *name, MHqid fromQid, Long time)
        {
           return (MHmsgBase::send(name, fromQid, sizeof(*this), time));
        }
};

#endif
