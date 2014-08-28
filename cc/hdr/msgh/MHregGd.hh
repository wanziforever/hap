#ifndef _MHREGGD_H
#define _MHREGGD_H
/*
** File ID:	@(#): <MID41606 () - 08/20/02, 14.1.1.1>
**
** File:		MID41606
** Release:		14.1.1.1
** Date:		09/12/02
** Time:		10:39:39
** Newest applied delta:08/20/02
*/

#include "hdr/GLreturns.h"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/msgh/MHgd.hh"
#include "cc/hdr/msgh/MHmsg.hh"

class MHregGd : public MHmsgBase {
	public:

	MHregGd(){
		priType = MHintPtyp;
		msgType = MHregGdTyp;
	};

	MHname		m_Name;
	LongLong	m_Size;
	Long		m_bufferSz;
	Long		m_maxUpdSz;
	Long		m_msgBufSz;
	int		m_Permissions;
	uid_t		m_Uid;
	MHgdDist	m_Dist;
	Bool		m_bReplicate;
	Bool		m_bCreate;
	Bool		m_doDelaySend;
	Bool		m_fill1;
	int		m_shmkey;
};

class MHregGdAck : public MHmsgBase {
	public:
	
	MHregGdAck(){
		priType = MHregPtyp;
		msgType = MHregGdAckTyp;
	};
	
	int		m_Shmid;
	GLretVal	m_RetVal;
	Bool		m_bIsNew;
	Bool		m_fill1;
};

class MHrmGd : public MHmsgBase {
	public:

	MHrmGd(){
		priType = MHintPtyp;
		msgType = MHrmGdTyp;
	};

	MHname		m_Name;
};

class MHrmGdAck : public MHmsgBase {
	public:
	
	MHrmGdAck(){
		priType = MHregPtyp;
		msgType = MHrmGdAckTyp;
	};
	
	GLretVal	m_RetVal;
};

class MHaudGd : public MHmsgBase {
	public:

	MHaudGd(){
		priType = MHintPtyp;
		msgType = MHaudGdTyp;
	};

	MHname		m_Name;
	Long		m_Sum;
	int		m_GdId;
	MHgdAudAction	m_Action;
	LongLong	m_Length;
	LongLong	m_Start;
};

class MHaudGdAck : public MHmsgBase {
	public:
	
	MHaudGdAck(){
		priType = MHregPtyp;
		msgType = MHaudGdAckTyp;
	};
	
	MHname		m_Name;
	GLretVal	m_RetVal;
};

#endif
