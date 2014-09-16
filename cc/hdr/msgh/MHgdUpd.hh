#ifndef _MHGDUPD_H
#define _MHGDUPD_H

/*
** File ID:	@(#): <MID42565 () - 08/17/02, 14.1.1.1>
**
** File:		MID42565
** Release:		14.1.1.1
** Date:		08/21/02
** Time:		19:43:42
** Newest applied delta:08/17/02
*/

#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/msgh/MHmsg.hh"
#include "cc/hdr/msgh/MHgd.hh"

class  MHgdUpd : public MHmsgBase {
	public:
	
	MHgdUpd(){
		priType = MHintPtyp;
		msgType = MHgdUpdTyp;
	}; 

	int	m_GdId;		// Id of the GDO
	int	m_Size;		// size of the total message
	int	m_seq;		// Sequence number
	int	m_ackSeq;	// Acknowledged sequence number
};


#endif
