#ifndef _MHGQINIT_H
#define _MHGQINIT_H
/*
** File ID:	@(#): <MID42256 () - 08/17/02, 14.1.1.1>
**
** File:		MID42256
** Release:		14.1.1.1
** Date:		08/21/02
** Time:		19:33:35
** Newest applied delta:08/17/02
*/

#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHmtypes.hh"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"

/*
** MHgqInit message
**	This message is sent to INIT process on a specific machine
**	to handle the transfer of the global queue
*/

class MHgqInit : public MHmsgBase {
	public:
		MHgqInit()
		{
		  priType = MHoamPtyp; 
		  msgType = MHgqInitTyp; 
		  m_realQ = MHnullQ;
		  m_gqid = MHnullQ;
		};
		inline MHgqInit(MHqid realQ, MHqid gqid)
		{
		  priType = MHoamPtyp; 
		  msgType = MHgqInitTyp; 
		  m_realQ = realQ;
		  m_gqid = gqid;
		};

	MHqid 	m_realQ;
	MHqid	m_gqid;
};

class MHgqInitAck : public MHmsgBase {
	public:
		MHgqInitAck(){
		  priType = MHoamPtyp; 
		  msgType = MHgqInitAckTyp; 
		  m_gqid = MHnullQ;
		  m_retVal = GLsuccess;
		};
		inline MHgqInitAck(MHqid gqid, GLretVal retVal){
		  priType = MHoamPtyp; 
		  msgType = MHgqInitAckTyp; 
		  m_gqid = gqid;
		  m_retVal = retVal;
		};

	MHqid		m_gqid;
	GLretVal	m_retVal;
};

#endif
