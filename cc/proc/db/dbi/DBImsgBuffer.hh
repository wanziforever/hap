#ifndef _DBIMSGBUFFER_H
#define _DBIMSGBUFFER_H

/*
**	File ID: 	@(#): <MID66148 () - 12/02/03, 2.1.1.2>
**
**	File:			MID66148
**	Release:		2.1.1.2
**	Date:			12/15/03
**	Time:			11:26:32
**	Newest applied delta:	12/02/03 18:59:58
**
** DESCRIPTION: This file contains the class definition for DBImsgBuffer
**              class. The class encapsulate the functions in former
**              DBIsMsgBuf.C
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
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/db/DBsqlMsg.hh"
#include "cc/hdr/db/DBselect.hh"
#include "DBIselect.hh"

class DBImsgBuffer {
    public:
        DBImsgBuffer();
        ~DBImsgBuffer();
        Short init();
        Void reset();
        Short fillHdrInfo(DBIselectBufLstTyp *lstPtr, const DBsqlMsg *selHdr, 
		                  Long ackCode, Char endFlag, U_char reqAck, Short npairs);
        DBIselectBufLstTyp *getNext();
        DBIselectBufLstTyp *getHead() const;
        Short sndSelectAck(Short nmsg, Char reqAck, const DBsqlMsg *selHdr);
        Short sndErrMsg(Long errCode, const Char *errMsg, const DBsqlMsg *selHdr);
        inline Short getSize(){ return size; }
        inline Short getNumBufferUsed() { return numBufferUsed; }

    private:
        Short addNode();

    private:
        DBIselectBufLstTyp *msgLstHdr, *msgLstTail, *currentMsg;
        Short size, numBufferUsed;

};
#endif

