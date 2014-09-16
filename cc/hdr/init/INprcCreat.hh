#ifndef __INPROCCREATE_H
#define __INPROCCREATE_H

// DESCRIPTION:
//	This header file contains the "INprocCreate" and "INprocCreateAck"
//	message class definitions.  The INprocCreate message is used
//	to have INIT start up temporary processes.
//
// NOTES:
//

#include <values.h>
#include <sys/types.h>
#include "hdr/GLtypes.h"
#include "cc/hdr/init/INmtype.hh"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/init/INproctab.hh"	/* IN_NAMEMX, IN_PATHNMMX */


#include <string.h>

// Contents of this structure and of the INprcCreate message should
// be the same.

struct INprocInfo {
	Char 	msgh_name[IN_NAMEMX];	/* Proc's MSGH queue name */
	Char	full_path[IN_PATHNMMX];	/* Proc's executable path name */
	Char	ext_path[IN_EPATHNMMX];	/* Proc's executable path name */
	U_char	priority;		/* Proc's UNIX priority   */
	int	uid;			/* Proc's UNIX UID - can't be 0! */
	U_short	sanity_tmr;		/* Proc's sanity pegging interval */
	U_short rstrt_intvl;		/* Proc's restart/re-init interval*/
	U_short rstrt_max;		/* Proc's restart/re-init threshold */
	IN_STARTSTATE inh_restart;	/* FALSE: attempt to restart proc.  */
  /* TRUE: DON'T restart proc */
	IN_PROC_CATEGORY proc_cat;	/* Process category	*/
	U_short error_threshold;	/* Threshold for escalation on errors */
	U_short error_dec_rate;		/* Decrement rate(/min) of error counts */
	U_short init_complete_timer;	/* Time (sec) for process to finish initialization*/
  /* once PROCINIT has been completed.		*/
	U_short procinit_timer;		/* Time (sec)  for a process to complete PROCINIT */
	Bool	crerror_inh;		/* True if CRERRORS should not be counted	*/
	IN_SOFTCHK    inh_softchk;	/* True if software checks should be inhibited  */
	U_char  run_lvl;		/* Process run level				*/
	Short	msgh_qid;		/* Value of msgh qid				*/
	Char	ofc_path[IN_OPATHNMMX];	/* Proc's official path name 			*/
	U_short	create_timer;		/* Creation time				*/
	Long	q_size;			/* Size of unix queue, if -1 use default	*/
	Bool	print_progress;		/* TRUE if progress messages should print	*/
	Bool 	on_active;		/* TRUE if process should be started on active -1 default */
	Bool 	third_party;		/* TRUE if process is third_party  default FALSE */
	Short	global_queue_timer;	/* Time to respond to global queue		*/
	Short	lv3_timer;		/* Time to respond to lv3 init message 		*/
	int	brevity_low;		/* Brevity control low threshold		*/
	int	brevity_high;		/* Brevity control high threshold		*/
	int	brevity_interval;	/* Brevity control interval			*/
	Short	msg_limit;		/* Message queue limit				*/
	Short	ps;			/* Processor set				*/
	Char	isRT;			/* Real time designation			*/
	int	group_id;		/* Process group id				*/
// take this out at the next retrofit boundary
	Bool	oamleadonly;
	Bool	active_vhost_only;
};

/*
**  "INprocCreate" message class:
**	This class defines the message processes should send to INIT when
**	they want INIT to start temporary processes.  The message type
**	for this message is "INprocCreateTyp" as defined in
**	"cc/hdr/init/mtype.h".
**
**	Note that the "msgh_name" and "full_path" class members must be
**	null-terminated character strings with "msgh_name" being a unique
**	MSGH name and "full_path" referencing an executable file on disk.
*/
class INprocCreate : public MHmsgBase {
public:
  INprocCreate()
     { priType = MHoamPtyp; msgType = INprocCreateTyp;
       msgh_name[0] = '\0';
       full_path[0] = '\0';
       ofc_path[0] = '\0';
       ext_path[0] = '\0';
       priority = 20;
       uid = 0;
       sanity_tmr = SHRT_MAX;

       rstrt_intvl = 0;
       rstrt_max = 0;
       inh_restart = FALSE;
       proc_cat = IN_NON_CRITICAL;
       error_threshold = SHRT_MAX;	
       error_dec_rate = SHRT_MAX;
       init_complete_timer = SHRT_MAX;
       procinit_timer = SHRT_MAX;	

       crerror_inh = FALSE;	
       inh_softchk = FALSE;
       run_lvl = 255;
       msgh_qid = -1;
       create_timer = SHRT_MAX;
       print_progress = TRUE;
       q_size = -1;
       on_active = -1;
       global_queue_timer = SHRT_MAX;
       lv3_timer = SHRT_MAX;
       brevity_low = -1;
       brevity_high = -1;
       brevity_interval = -1;
       msg_limit = -1;
       third_party = FALSE;
       ps = -1;
       isRT = IN_NOTRT;
       group_id = -1;
       oamleadonly = -1;
       active_vhost_only = -1;
     };

	inline	INprocCreate(Char *name, Char *path, U_char prio,
                       int util_id, U_short sanity, U_short rintvl,
                       U_short rmax, Bool inh_flag, 
                       IN_PROC_CATEGORY category = IN_NON_CRITICAL,
                       U_short e_threshold = SHRT_MAX, 
                       U_short e_rate = SHRT_MAX,
                       U_short init_complete_tmr = SHRT_MAX,
                       U_short procinit_tmr = SHRT_MAX,
                       
                       Bool    crerr_inh = FALSE,
                       Bool    softchk = FALSE,
                       U_char r_lvl = 255,
                       Short mh_qid = -1,
                       Char* o_path = NULL,
                       U_short create_tmr = SHRT_MAX,

                       Bool print_progress = TRUE,
                       Long q_size = -1,
                       Bool on_active = -1,

                       Short global_queue_timer = SHRT_MAX,
                       Short lv3_timer = SHRT_MAX,

                       int brevity_low = -1,
                       int brevity_high = -1,
                       int brevity_interval = -1,
                       Short msg_limit = -1,
                       Char* ext_path = NULL,
                       Bool third_party = FALSE,
                       Short ps = -1,Char rt = IN_NOTRT,int gid = -1
                       , Bool oamleadOnly = -1, Bool actvie_vhost_Only = -1
    ); 

	inline GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
	inline GLretVal	send(const Char *name, MHqid fromQid, Long time);
	/*
	 * Note that this method does not require the destination queue
	 * name because this message is always sent TO INIT.
	 */
	inline GLretVal	send(MHqid fromQid, Long time);

	/*
	 *  Data - Processes must initialize these class members before
	 *	   sending the message to INIT
	 */
	Char 	msgh_name[IN_NAMEMX];	/* Proc's MSGH queue name */
	Char	full_path[IN_PATHNMMX];	/* Proc's executable path name */
	U_char	priority;		/* Proc's UNIX priority   */
	int	uid;			/* Proc's UNIX UID - can't be 0! */
	U_short	sanity_tmr;		/* Proc's sanity pegging interval */
	U_short rstrt_intvl;		/* Proc's restart/re-init interval*/
	U_short rstrt_max;		/* Proc's restart/re-init threshold */
	Bool	inh_restart;		/* FALSE: attempt to restart proc.  */
  /* TRUE: DON'T restart proc */
	IN_PROC_CATEGORY proc_cat;	/* Process category	*/
	U_short error_threshold;	/* Threshold for escalation on errors */
	U_short error_dec_rate;		/* Decrement rate(/min) of error counts */
	U_short init_complete_timer;	/* Time (sec) for process to finish initialization*/
  /* once PROCINIT has been completed.		*/
	U_short procinit_timer;		/* Time (sec)  for a process to complete PROCINIT */
	Bool	crerror_inh;		/* True if CRERRORS should not be counted	*/
	Bool    inh_softchk;		/* True if software checks should be inhibited  */
	U_char  run_lvl;		/* Process run level				*/
	Short	msgh_qid;		/* Value of msgh qid, if INfixQ, INIT will pick one */
	Char	ofc_path[IN_OPATHNMMX];	/* Proc's official path name 			*/
	Char	ext_path[IN_EPATHNMMX];	/* Proc's external path name 			*/
	U_short	create_timer;		/* Creation time				*/
	Long	q_size;			/* Size of unix queue, if -1 use default	*/
	Bool	print_progress;		/* TRUE if progress messages should print	*/
	Bool 	on_active;		/* TRUE if process should be started on active -1 default */
	Bool 	third_party;		/* TRUE if third party process, default FALSE	*/
	Short	global_queue_timer;	/* Time to respond to global queue		*/
	Short	lv3_timer;		/* Time to respond to lv3 init message 		*/
	int	brevity_low;		/* Brevity control low threshold		*/
	int	brevity_high;		/* Brevity control high threshold		*/
	int	brevity_interval;	/* Brevity control interval			*/
	Short	msg_limit;		/* Message queue limit				*/
	Short	ps;			/* Processor set				*/
	Char	isRT;			/* Process real time designation		*/
	int	group_id;		/* Process group id				*/
#ifdef __linux
	Bool	oamleadonly;
	Bool	active_vhost_only;
#endif
};

inline
INprocCreate::INprocCreate(Char *name, Char *path, U_char prio, int util_id,
                           U_short sanity, U_short rintvl, U_short rmax, Bool inh_flag,
                           IN_PROC_CATEGORY category, U_short e_threshold, U_short e_rate,
                           U_short init_complete_tmr, U_short procinit_tmr, Bool crerr_inh,
                           Bool softchk, U_char r_lvl, Short mh_qid, Char* o_path, U_short create_tmr,
                           Bool prt_progress, Long q_sz, Bool on_act, Short gqtmr, Short lv3tmr,
                           int brv_low, int brv_high, int brv_interval, Short msg_lmt, Char* e_path, Bool trd_party,
                           Short pset, Char rt, int gid
                           , Bool oamleadOnly, Bool active_vhost_Only
  )
{
	priType = MHoamPtyp;
	msgType = INprocCreateTyp;

	Short len = strlen(name);
	if (len >= IN_NAMEMX) {
		strncpy(msgh_name, name, (IN_NAMEMX-1));
		msgh_name[IN_NAMEMX-1] = (Char)0;
	}
	else {
		strcpy(msgh_name, name);
	}

	len = strlen(path);
	if (len >= IN_PATHNMMX) {
		strncpy(full_path, path, (IN_PATHNMMX-1));
		full_path[IN_NAMEMX-1] = (Char)0;
	}
	else {
		strcpy(full_path, path);
	}

	if(o_path != NULL){
		strncpy(ofc_path, o_path, (IN_OPATHNMMX-1));
	} else {
		ofc_path[0] = '\0';
	}
	
	if(e_path != NULL){
		strncpy(ext_path, e_path, (IN_EPATHNMMX-1));
	} else {
		ext_path[0] = '\0';
	}
	
	priority = prio;
	uid = util_id;
	sanity_tmr = sanity;
	rstrt_intvl = rintvl;
	rstrt_max = rmax;
	inh_restart = inh_flag;
	proc_cat = category;	
	error_threshold = e_threshold;
	error_dec_rate = e_rate;
	init_complete_timer = init_complete_tmr;
	procinit_timer = procinit_tmr;
	create_timer = create_tmr;
	crerror_inh = crerr_inh;
	inh_softchk = softchk;
	run_lvl = r_lvl;
	msgh_qid = mh_qid;
	print_progress = prt_progress;
	q_size = q_sz;
	on_active = on_act;
	global_queue_timer = gqtmr;
	lv3_timer = lv3tmr;
	brevity_low = brv_low;
	brevity_high = brv_high;
	brevity_interval = brv_interval;
	msg_limit = msg_lmt;
	third_party = trd_party;
	ps = pset;
	isRT = rt;
	group_id = gid;
	oamleadonly = oamleadOnly;
	active_vhost_only = active_vhost_Only;
}

inline GLretVal
INprocCreate::send(MHqid toQid, MHqid fromQid, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INprocCreate),
                     time);
}

inline GLretVal
INprocCreate::send(MHqid fromQid, Long time)
{
	char	initname[MHmaxNameLen + 1];
	char	leadname[MHmaxNameLen + 1];

	if(on_active == TRUE){
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
INprocCreate::send(const Char *name, MHqid fromQid, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send(name, (Char *) this, sizeof(INprocCreate),
                     time);
}

/*
**  "INprocCreateAck" message class:
**	This class defines the message INIT will return in response to
**	a "INprocCreate" message when a temporary process has been
**	successfully created.  Its message type is "INprocCreateAckType"
**	as defined in "cc/hdr/init/INmtype.H".
**
**	Note that this message is sent when the process is successfully
**	exec'ed but not before the process has completed its initialization.
*/
class INprocCreateAck : public MHmsgBase {
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
	pid_t	pid;		/* PID of the temporary process */
};


/*
**  "INprocCreateFail" message class:
**	This class defines the message INIT returns in response to a
**	"INprocCreate" message when a temporary process was not
**	successfully created.  Its message type is "INprocCreateFailTyp"
**	as defined in "cc/hdr/init/INmtype.H".
**
**	The error return contained in this message will be set to
**	one of the following values (as defined in "cc/hdr/init/INreturns.H"):
**
**		INDUPMSGNM    -	MSGH name matches that of a process already
**				started by INIT.
**		INNOEXIST     - Executable file does not exist
**		INNOTEXECUT   -	File referenced by "full_path" is not
**				executable.
**		ININVPRIO     -	"priority" exceed maximum allowed UNIX
**				process priority level
**		ININVUID      - "uid" was zero - temporary processes can't
**				run as super-user
**		INFORKFAIL    -	"fork()" failed.
**		ININVPARM     - invalid process parameters
*/
class INprocCreateFail : public MHmsgBase {
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
