#ifndef __ININITIALIZE_H
#define __ININITIALIZE_H
/*
**	File ID: 	@(#): <MID42107 () - 09/07/02, 14.1.1.1>
**
**	File:			MID42107
**	Release:		14.1.1.1
**	Date:			09/13/02
**	Time:			11:59:03
**	Newest applied delta:	09/07/02 08:43:48
**
** DESCRIPTION:
**	This header file contains the "INinitialize" and "INinitializeAck"
**	message class definitions.  This message is sent to all processes
**	already running on the ACTIVE side during level 3 initialization
**
** NOTES:
*/

#include <values.h>
#include <sys/types.h>
#include "hdr/GLtypes.h"
#include "cc/hdr/init/INmtype.hh"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/init/INinit.hh"	

class INinitialize : public MHmsgBase {
    public:
		INinitialize()
			 { priType = MHoamPtyp; 
			   msgType = INinitializeTyp;
			   sn_lvl = SN_LV3;
			};

	SN_LVL	sn_lvl;
};

class INinitializeAck : public MHmsgBase {
    public:
		INinitializeAck()
			 { priType = MHoamPtyp; 
			   msgType = INinitializeAckTyp;
			};
    inline GLretVal send(MHqid fromQ);
};

inline GLretVal
INinitializeAck::send(MHqid fromQ)
{
	srcQue = fromQ;
	return MHmsgh.send("INIT", (char*)this, (Long)sizeof(*this), 0);
}

class INfailover : public MHmsgBase {
    public:
		INfailover()
			 { priType = MHoamPtyp; 
			   msgType = INfailoverTyp;
			};

};

class INoamInitialize : public MHmsgBase {
    public:
		INoamInitialize()
			 { 
				priType = MHoamPtyp; 
				msgType = INoamInitializeTyp;
			};

};

class INvhostInitialize : public MHmsgBase {
    public:
		INvhostInitialize(const char* fName)
			 { 
				priType = MHoamPtyp; 
				msgType = INvhostInitializeTyp;
				memset(fromHost, 0x0, sizeof(fromHost));
				strncpy(fromHost, fName, MHmaxNameLen);
			};
		char	fromHost[MHmaxNameLen+1];
};

#endif
