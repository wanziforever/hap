#ifndef __INSETSOFTCHK_H
#define __INSETSOFTCHK_H

// DESCRIPTION:
//	This header file contains the following message class definitions:'
//
//		INsetSoftChk    - Enable/disable a process's "inhibit restart"
//				flag.
//		INsetSoftChkAck - Successful ACK to "INsetRstrt" message
//		INsetSoftChkFail - Error response to "INsetRstrt" message
//
// NOTES:
//
#include <string.h>
#include "hdr/GLtypes.h"

#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/msgh/MHpriority.hh"
#include "cc/hdr/init/INmtype.hh"
#include "cc/hdr/init/INproctab.hh"	/* IN_NAMEMX */

/*
**  "INsetRstrt" message class:
**	This class defines the message processes should send to INIT when
**	they wish to enable or disable the "inhibit restart" flag associated
**	with a process.  When a process's "inhibit restart" flag is
**	enabled it will NOT be restarted/re-init'ed if it dies or
**	fails to peg its sanity flag.
**
**	If the process referenced in this message is currently under
**	INIT's control than INIT will set it "inhibit restart" status
**	accordingly and return a "INsetRstrtAck" message to the sending
**	MSGH queue in the original "INsetRstrt" message.  Otherwise
**	a "INsetRstrtFail" message is returned.
**
**	The message type used for this message is "INsetRstrtTyp" as
**	defined in "cc/hdr/init/INmtype.H"
*/
class INsetSoftChk : public MHmsgBase {
    public:
		INsetSoftChk()
			 { priType = MHoamPtyp; msgType = INsetSoftChkTyp;};
	inline INsetSoftChk(Char *name, Bool inh_flg);

	inline GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
	inline GLretVal	send(const Char *name, MHqid fromQid, Long time);

	/*
	 * Note that this method does not require the destination queue
	 * name because this message is always sent TO INIT:
	 */
	inline GLretVal	send(MHqid fromQid, Long time);

	/*
	 *  Data - Processes must initialize these class members before
	 *	   sending the message to INIT
	 */
	Char 	msgh_name[IN_NAMEMX];	/* Proc's MSGH queue name */
					/* If MSGH is null, action applies to system */
	Bool	inh_softchk;		/* TRUE: don't restart proc. if */
					/* 	 dies/misses sanity pegs*/
					/* FALSE: perform "normal" recovery */
					/* 	  step if process dies of */
					/*	  misses sanity pegs	*/
};

inline
INsetSoftChk::INsetSoftChk(Char *proc_name, Bool inh_flag) {
	priType = MHoamPtyp;
	msgType = INsetSoftChkTyp;

	Short len = strlen(proc_name);
	if (len >= IN_NAMEMX) {
		strncpy(msgh_name, proc_name, (IN_NAMEMX-1));
		msgh_name[IN_NAMEMX-1] = (Char)0;
	}
	else {
		strcpy(msgh_name, proc_name);
	}
	inh_softchk = inh_flag;
}

inline GLretVal
INsetSoftChk::send(MHqid toQid, MHqid fromQid, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INsetSoftChk),
			   time);
}

inline GLretVal
INsetSoftChk::send(MHqid fromQid, Long time)
{
	return send("INIT", fromQid, time);
}

inline GLretVal
INsetSoftChk::send(const Char *name, MHqid fromQid, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send(name, (Char *) this, sizeof(INsetSoftChk),
			   time);
}

/*
**  "INsetSoftChkAck"
**	This class defines the message INIT will return in response to
**	a "INsetSoftChk" message when the process referenced in the
**	"INsetSoftChkAck" message is controlled by INIT.
**
**	The message type used for this message is "INsetSoftChkAckTyp" as
**	defined in "cc/init/INmtype.H"
*/
class INsetSoftChkAck : public MHmsgBase {
    public:
	/*
	 * Since INIT is the only sender for this message, no methods
	 * need to be defined here.  All processes receiving this
	 * message simply need to switch on its message type
	 */
	Char 	msgh_name[IN_NAMEMX];	/* Proc's MSGH queue name */
	Bool	inh_softchk;		/* current status of proc's */
					/* "inhibit softchk" flag */
};


/*
**  "INsetSoftChkFail" message class:
**	This class defines the message INIT returns in response to a
**	"INsetSoftChk" message when INIT is not currently controlling
**	the process referenced in the "INsetSoftChk" message.  Error
**	return values for which may be included in this message
**	(defined in "cc/hdr/init/INreturns.H") include:
**
**		INNOPROC    - The process is not known to INIT
**
**
**	The message type used in this message is "INsetSoftChkFailTyp"
**	as defined in "cc/hdr/init/INmtype.H".
*/
class INsetSoftChkFail : public MHmsgBase {
    public:
	/*
	 * Since INIT is the only sender for this message, no methods
	 * need to be defined here.  All processes receiving this
	 * message simply need to switch on the message type
	 * and cast the message with this class to access the data
	 * members.
	 */

	/* message-specific data */
	Char 	msgh_name[IN_NAMEMX];	/* MSGH name included in original */
					/* "INsetSoftChk" message */
	GLretVal	ret;	/* error return - 			*/
				/* 	see "cc/hdr/init/INreturns.H"	*/
};

#endif
