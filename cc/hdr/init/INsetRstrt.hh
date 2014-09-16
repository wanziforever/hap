#ifndef __INSETRSTRT_H
#define __INSETRSTRT_H

// DESCRIPTION:
//	This header file contains the following message class definitions:'
//
//		INsetRstrt    - Enable/disable a process's "inhibit restart"
//				flag.
//		INsetRstrtAck - Successful ACK to "INsetRstrt" message
//		INsetRstrtFail - Error response to "INsetRstrt" message
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
class INsetRstrt : public MHmsgBase {
    public:
		INsetRstrt()
			 { priType = MHoamPtyp; msgType = INsetRstrtTyp; bAll = FALSE; };
	inline INsetRstrt(Char *name, Bool rstrt_flg, Bool ucl_flg, Bool bAll = FALSE);

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
	Bool	inh_restart;		/* TRUE: don't restart proc. if */
					/* 	 dies/misses sanity pegs*/
					/* FALSE: perform "normal" recovery */
					/* 	  step if process dies of */
					/*	  misses sanity pegs	*/
	Bool	ucl;			/* True if action is to be unconditional */
	Bool	bAll;			/* True if all instances of the process are affected */
};

inline
INsetRstrt::INsetRstrt(Char *proc_name, Bool inh_flag, Bool ucl_flg, Bool bAll_flg) {
	priType = MHoamPtyp;
	msgType = INsetRstrtTyp;

	Short len = strlen(proc_name);
	if (len >= IN_NAMEMX) {
		strncpy(msgh_name, proc_name, (IN_NAMEMX-1));
		msgh_name[IN_NAMEMX-1] = (Char)0;
	}
	else {
		strcpy(msgh_name, proc_name);
	}
	inh_restart = inh_flag;
	ucl = ucl_flg;
	bAll = bAll_flg;
}

inline GLretVal
INsetRstrt::send(MHqid toQid, MHqid fromQid, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INsetRstrt),
			   time);
}

inline GLretVal
INsetRstrt::send(MHqid fromQid, Long time)
{
        char    initname[MHmaxNameLen + 1];
        char    leadname[MHmaxNameLen + 1];

        if(bAll == TRUE){
                if(MHmsgh.hostId2Name(MHmsgh.getLeadCC(), leadname) != GLsuccess){
                        return(MHinvHostId);
                }
                sprintf(initname, "%s:INIT", leadname);
        } else { 
                strcpy(initname, "INIT");
        }
        return send(initname, fromQid, time);

}

inline GLretVal
INsetRstrt::send(const Char *name, MHqid fromQid, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send(name, (Char *) this, sizeof(INsetRstrt),
			   time);
}

/*
**  "INsetRstrtAck"
**	This class defines the message INIT will return in response to
**	a "INsetRstrt" message when the process referenced in the
**	"INsetRstrtAck" message is controlled by INIT.
**
**	The message type used for this message is "INsetRstrtAckTyp" as
**	defined in "cc/init/INmtype.H"
*/
class INsetRstrtAck : public MHmsgBase {
    public:
	/*
	 * Since INIT is the only sender for this message, no methods
	 * need to be defined here.  All processes receiving this
	 * message simply need to switch on its message type
	 */
	Char 	msgh_name[IN_NAMEMX];	/* Proc's MSGH queue name */
	Bool	inh_restart;		/* current status of proc's */
					/* "inhibit restart" flag */
};


/*
**  "INsetRstrtFail" message class:
**	This class defines the message INIT returns in response to a
**	"INsetRstrt" message when INIT is not currently controlling
**	the process referenced in the "INsetRstrt" message.  Error
**	return values for which may be included in this message
**	(defined in "cc/hdr/init/INreturns.H") include:
**
**		INNOPROC    - The process is not known to INIT
**		INNORESCHED - The process is currently scheduled to be
**			      killed and, therefore, it's "inhibit restart"
**			      status cannot be reset.
**
**
**	The message type used in this message is "INsetRstrtFailTyp"
**	as defined in "cc/hdr/init/INmtype.H".
*/
class INsetRstrtFail : public MHmsgBase {
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
					/* "INsetRstrt" message */
	GLretVal	ret;	/* error return - 			*/
				/* 	see "cc/hdr/init/INreturns.H"	*/
};

#endif
