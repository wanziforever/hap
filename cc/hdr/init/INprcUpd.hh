#ifndef __INPROCUPDATE_H
#define __INPROCUPDATE_H

// DESCRIPTION:
//	This header file contains the "INprocUpdate" 
//	message class definition.  The INprocUpdate message is used
//	to have INIT update process parameters for temporary processes.
//
// NOTES:
//

#include "hdr/GLtypes.h"
#include "cc/hdr/init/INmtype.hh"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/init/INproctab.hh"	/* IN_NAMEMX, IN_PATHNMMX */

#include <string.h>

/*
**  "INprocUpdate" message class:
**	This class defines the message processes should send to INIT when
**	they want INIT to modify parameters for temporary processes.  
**	The message type for this message is "INprocUpdateTyp" as defined in
**	"cc/hdr/init/INmtype.h".
**
**	Note that the "msgh_name" class member must be a null-terminated 
**	character string with "msgh_name" being a unique MSGH name.
*/
class INprocUpdate : public MHmsgBase {
    public:
		INprocUpdate()
			 { priType = MHoamPtyp; msgType = INprocUpdateTyp;
			   proc_cat = IN_MAX_CAT; inh_restart = FALSE; bAll = FALSE;
			   on_active = -1;
			   };

	inline	INprocUpdate(Char *name, IN_PROC_CATEGORY category, Bool restart_flg = FALSE, Bool bAll_flg = FALSE, Bool on_active = -1);

	inline GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
	inline GLretVal	send(Char *name, MHqid fromQid, Long time);
	/*
	 * Note that this method does not require the destination queue
	 * name because this message is always sent TO INIT.
	 */
	inline GLretVal	send(MHqid fromQid, Long time);

	/*
	 *  Data - Processes must initialize these class members before
	 *	   sending the message to INIT
	 */
	Char 	msgh_name[IN_NAMEMX];	/* Proc's MSGH queue name 		*/
	IN_PROC_CATEGORY proc_cat;	/* Process category	  		*/
	Bool    inh_restart;            /* FALSE: attempt to restart proc.	*/
					/* TRUE: DON'T restart proc 		*/
	Bool	bAll;			/* Aplly to all copies of this process  */
	Bool	on_active;		/* Change value of on_active field	*/
};

inline
INprocUpdate::INprocUpdate(Char *name, IN_PROC_CATEGORY category, Bool restart_flg,
	Bool bAll_flg, Bool on_act)
{
	priType = MHoamPtyp;
	msgType = INprocUpdateTyp;

	strncpy(msgh_name, name, (IN_NAMEMX-1));
	msgh_name[IN_NAMEMX-1] = (Char)0;

	proc_cat = category;
	inh_restart = restart_flg;
	bAll = bAll_flg;
	on_active = on_act;
}

inline GLretVal
INprocUpdate::send(MHqid toQid, MHqid fromQid, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INprocUpdate),
			   time);
}

inline GLretVal
INprocUpdate::send(MHqid fromQid, Long time)
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
INprocUpdate::send(Char *name, MHqid fromQid, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send(name, (Char *) this, sizeof(INprocUpdate),
			   time);
}

/*
**  "INprocUpdateAck" message class:
**	This class defines the message INIT will return in response to
**	a "INprocUpdate" message when a temporary process has been
**	successfully updated.  Its message type is "INprocUpdateAckType"
**	as defined in "cc/hdr/init/INmtype.H".
**
*/
class INprocUpdateAck : public MHmsgBase {
    public:
	/*
	 * Since INIT is the only sender for this message, no methods
	 * need to be defined here.  All processes receiving this
	 * message simply need to switch on the message type
	 * and cast the message with this class to access the data
	 * members.
	 */

	/* message-specific data */
	Char	msgh_name[IN_NAMEMX];	/* Process's name */
};


/*
**  "INprocUpdateFail" message class:
**	This class defines the message INIT returns in response to a
**	"INprocUpdate" message when a temporary process was not
**	successfully updated.  Its message type is "INprocCreateFailTyp"
**	as defined in "cc/hdr/init/INmtype.H".
**
**	The error return contained in this message will be set to
**	one of the following values (as defined in "cc/hdr/init/INreturns.H"):
**
**		INNOPROC   -	process does not exist
**		INNOTTEMP  - 	process was not temporary
**		INVPARM	   -    invalid parameter values
*/
class INprocUpdateFail : public MHmsgBase {
    public:
	/*
	 * Since INIT is the only sender for this message, no methods
	 * need to be defined here.  All processes receiving this
	 * message simply need to switch on the message type
	 * and cast the message with this class to access the data
	 * members.
	 */

	/* message-specific data */
	Char	msgh_name[IN_NAMEMX];	/* Process's name */
	GLretVal	ret;	/* error return - 			*/
				/* 	see "cc/hdr/init/INreturns.H"	*/
};

#endif
