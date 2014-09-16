#ifndef __INKILLPROC_H
#define __INKILLPROC_H

// DESCRIPTION:
//	This header file contains the following message class definitions:'
//
//		INkillProc    -	Request for INIT to kill a temporary process
//		INkillProcAck - Positive response to a "INkillProc" message
//		INkillProcFail - Negative response to a "INkillProc" message
//
// NOTES:
//

#include <string.h>
#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/msgh/MHpriority.hh"

#include "cc/hdr/init/INproctab.hh"
#include "cc/hdr/init/INmtype.hh"

/*
**  "INkillProc" message class:
**	This class defines the message processes should send to INIT when
**	they want INIT to kill a temporary process.
**	Receipt of this message will cause INIT to kill the specified
**	temporary process and send and "ACK" back to the process requesting
**	the temporary process's death.  Note that the originating process
**	will actually receive TWO messages - the "ACK" to its "INkillProc"
**	message and the "INprocDeath" message which INIT will broadcast
**	when it kills the temporary process.
**
**	The message type for this message is "INkillProcTyp" as defined in
**	"cc/hdr/init/mtype.h".
**
**	If the process passed in this message is a currently active
**	temporary process then INIT will return a "INkillProcAck" message
**	to the sending MSGH queue ID in the original \fIINkillProc\fR
**	message.  Otherwise, INIT will return a "INkillProcFail" message
**	indicating the reason for INIT's rejecting the original request.
*/
class INkillProc : public MHmsgBase {
    public:
		INkillProc()
			 { priType = MHoamPtyp;
			   msgType = INkillProcTyp;
			   send_pdeath = TRUE;
			   bAll = FALSE;
			   bSigterm = FALSE; };
	inline	INkillProc(const Char *name, Bool send_pdeath = TRUE, Bool bAll = FALSE, Bool bSigterm = FALSE);

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
	Char 	msgh_name[IN_NAMEMX];	/* Temp. proc's MSGH queue name 	*/
	Bool	send_pdeath;		/* True if pdeath message is wanted 	*/
	Bool	bAll;			/* True if all instances should be killed */
	Bool    bSigterm;		/* Send SIGTERM instead of SIGKILL	*/
};

inline
INkillProc::INkillProc(const Char *name, Bool pdeath, Bool bAll_flg, Bool bSigterm_flg)
{
	Short len = strlen(name);
	if (len >= IN_NAMEMX) {
		strncpy(msgh_name, name, (IN_NAMEMX-1));
		msgh_name[IN_NAMEMX-1] = (Char)0;
	}
	else {
		strcpy(msgh_name, name);
	}
	send_pdeath = pdeath;
	bAll = bAll_flg;
	bSigterm = bSigterm_flg;
	priType = MHoamPtyp;
	msgType = INkillProcTyp;
}

inline GLretVal
INkillProc::send(MHqid toQid, MHqid fromQid, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INkillProc),
			   time);
}

inline GLretVal
INkillProc::send(MHqid fromQid, Long time)
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
INkillProc::send(const Char *name, MHqid fromQid, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send(name, (Char *) this, sizeof(INkillProc),
			   time);
}


/*
**  "INkillProcAck"
**	This class defines the message INIT will return in response to
**	a "INkillProc" message when a temporary process has been
**	successfully scheduled to be killed.  Its message type is
**	"INkillProcAckTyp" as defined in "cc/hdr/init/INmtype.H".
*/
class INkillProcAck : public MHmsgBase {
    public:
	/*
	 * Since INIT is the only sender for this message, no methods
	 * need to be defined here.  All processes receiving this
	 * message simply need to switch on its message type
	 */
	Char msgh_name[IN_NAMEMX];
};


/*
**  "INkillProcFail" message class:
**	This class defines the message INIT returns in response to a
**	"INkillProc" message when INIT is not able to kill the requested
**	process.  The following possible failure codes (defined in
**	"cc/hdr/init/INreturns.H") may be included in this message:
**
**		INNOPROC   - The process name is not registered with INIT
**		INNOTTEMP  - The process is not a temporary process
**
*
*/
class INkillProcFail : public MHmsgBase {
    public:
	/*
	 * Since INIT is the only sender for this message, no methods
	 * need to be defined here.  All processes receiving this
	 * message simply need to switch on the message type
	 * and cast the message with this class to access the data
	 * members.
	 */

	/* message-specific data */
	GLretVal	ret;	/* error return - 			*/
				/* 	see "cc/hdr/init/INreturns.H"	*/
	Char msgh_name[IN_NAMEMX];
};

#endif
