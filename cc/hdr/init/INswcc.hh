#ifndef __INSWCC_H
#define __INSWCC_H

// DESCRIPTION:
// 	Message class to send messages from SW:CC cep to INIT.
//
// OWNER: Adam Pajerski
//

#include "hdr/GLtypes.h"
#include "hdr/GLmsgs.h"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/init/INmtype.hh"

class INswcc : public MHmsgBase
{
    public:
	Bool	ucl;	/* True if backout is unconditional 	*/
	inline INswcc(Bool ucl_flg = FALSE);
	GLretVal send(MHqid srcQid, Long time);

};

class INswccAck : public MHmsgBase
{
    public:
	GLretVal send(MHqid toQid, MHqid srcQid, Long time);

};

class INswccFail : public MHmsgBase
{
    public:
	GLretVal	retval;
};

inline
INswcc::INswcc(Bool ucl_flg) {
	priType = MHoamPtyp;
	msgType = INswccTyp;

	ucl = ucl_flg;
}

inline GLretVal
INswcc::send(MHqid fromQid, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send("INIT", (Char *) this, sizeof(INswcc),
			   time);
}

#endif
