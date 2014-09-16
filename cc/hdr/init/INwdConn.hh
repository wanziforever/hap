#ifndef __INWDCONN_H
#define __INWDCONN_H

// DESCRIPTION:
//	This header file contains the "INwdSend" and INwdRcv
//	message class definition.  This message is used by any process to
//	send a message to the watchdog.  
//
// NOTES:
//

#include "hdr/GLtypes.h"
#include "cc/hdr/init/INmtype.hh"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"

class INwdSend : public MHmsgBase {
    public:
	INwdSend() { 
		priType = MHoamPtyp; 
	  	msgType = INwdSendTyp;
		seq = -1;
		};

	inline	GLretVal	send(MHqid fromQid, Long time, int mseq = -1);

	int	seq;		// End user id
	char	wdmsg[1];	// Contains formatted, CR and null terminated WD message
};

inline GLretVal
INwdSend::send(MHqid fromQid, Long time , int mseq)
{
	priType = MHoamPtyp; 
  	msgType = INwdSendTyp;
	seq = mseq;
	srcQue = fromQid;
	return MHmsgh.send("INIT", (Char*) this, strlen(wdmsg) + sizeof(INwdSend), time);
}

class INwdRcv : public MHmsgBase {
    public:
        INwdRcv() {
                priType = MHoamPtyp;
                msgType = INwdRcvTyp;
		seq = -1;
        };
 
        inline  GLretVal        send(MHqid from, MHqid to, char* msg, int nSeq);
	int	seq; 		// Correlated user id
        char    wdmsg[1];
};
 
inline GLretVal
INwdRcv::send(MHqid from, MHqid to, char* msg, int nSeq)
{
        priType = MHoamPtyp;
        msgType = INwdRcvTyp;
	srcQue = from;
	seq = nSeq;
	strcpy(wdmsg, msg);
        return MHmsgh.send(to, (Char*) this, strlen(wdmsg) + sizeof(INwdRcv), 0L);
}
#endif
