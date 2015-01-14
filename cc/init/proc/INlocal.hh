/*
**	File ID: 	@(#): <MID12300 () - 05/12/03, 29.1.1.2>
**
**	File:			MID12300
**	Release:		29.1.1.2
**	Date:			06/18/03
**	Time:			19:19:17
**	Newest applied delta:	05/12/03 16:42:30
**
** DESCRIPTION:
** 	This header file contains general external and const declarations
**	needed to build the INIT process.
**
** NOTES:
*/

#include <signal.h>
#include <values.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/eh/EHhandler.hh"
//#include "cc/hdr/cr/CRdebugMsg.hh"
#include "cc/hdr/cr/CRomInfo.hh"
#include "cc/hdr/cr/CRprmMsg.hh"
#include "cc/hdr/init/INreturns.hh"
//#include "cc/hdr/su/SUapplyMsg.hh"
#include "cc/hdr/init/INproctab.hh"
#include "cc/init/proc/INtimers.hh"
#include <thread.h>
#include <pthread.h>


#define MAX_TOKEN_LEN   512             /* Maximum length of initlist token 	*/
#define INIT_INDEX	-1		/* INIT process table index		*/
#define IN_MAX_INIT_ERRORS 20		/* Maximum number of init errors	*/
#define IN_NO_RESTART	MAXSHORT	/* Process should not be restarted	*/
#define IN_PROGRESS_CHECK_MAX  	7	/* Maximum number of missed progress checks */
#define IN_MIN_STACK	64000		/* Minimum stack size for INIT threads	*/

/* Flags identifying missing environment info to be ORed int INnoenv_vars*/
#define IN_NO_TZ	0x00000001	/* TZ not set		*/
#define IN_NO_LANG	0x00000002	/* LC_TIME not set	*/

#define IN_MSGH_RLVL	10		/* Run level of MSGH process		*/

extern Long INnoenv_vars;
extern Long INvmem_minor;
extern Long INvmem_major;
extern Long INvmem_critical;
extern CRALARMLVL INfailover_alarm;
extern Bool  INdidFailover;
#ifdef __sun
extern const struct anoninfo * INanoninfo;
#endif

#define INmaxPrioRT	79

/*
 * These macros are accessed within trace statements.  They verify that
 * enumerated typedefs are in range before translating them into
 * text strings for trace output:
 */
#define IN_PROCSTNM(indx) (((unsigned int)indx < IN_MAXSTATE) ? IN_procstnm[indx] : "INV INDX")

#define IN_SQSTEPNM(indx) (((unsigned int)indx < IN_MAXSTEP) ? IN_sqstepnm[indx] : "INV INDX")

#define IN_PSTATENM(indx) (((unsigned int)indx < IN_MAXPTYPE) ? IN_pstatenm[indx] : "INV INDX")

#define IN_STATENM(indx) (((unsigned int)indx < IN_MAXINIT) ? IN_statenm[indx] : "INV INDX")

#define IN_SNLVLNM(indx) (((unsigned int)indx < IN_MAXSNLVL) ? IN_snlvlnm[indx] : "INV INDX")

#define IN_SNSRCNM(indx) (((unsigned int)indx < IN_MAXSOURCE) ? IN_srcnm[indx] : "INV INDX")

#define IN_PROCCATNM(indx) (((unsigned int)indx < IN_MAX_CAT) ? IN_proccatnm[indx] : "INV INDX")

// Definition of process parameters controlled from initlist file.
// Use of the fields is documented in that file.

struct IN_PROC_PARMS {
	char		msgh_name[IN_NAMEMX];
	char		path[IN_PATHNMMX];
	char		ofc_path[IN_OPATHNMMX];
	char		ext_path[IN_EPATHNMMX];
	int		user_id;		
	int		group_id;		
	U_char 		run_lvl;		
	IN_STARTSTATE	inhibit_restart;
	IN_SOFTCHK	inh_softchk;
	Bool		crerror_inh;	
	IN_PROC_CATEGORY proc_category;
	U_short		error_threshold;
	U_short		error_dec_rate;
	U_short		init_complete_timer;
	U_short		procinit_timer;
	U_short		restart_threshold;
	U_char		priority;
	U_short		sanity_timer;
	U_short		restart_interval;
	Short		msgh_qid;
	U_short		create_timer;
	Long		q_size;
	U_short		lv3_timer;
	U_short		global_queue_timer;
	Bool		on_active;
	Bool		oamleadonly;
	Bool		active_vhost_only;
	int		brevity_low;
	int		brevity_high;
	int		brevity_interval;
	Short		msg_limit;
	Bool		third_party;
	Bool		isRT;
	Short		ps;
};

#define INmaxProcessors	32
// Definition of system parameters controlled from initlist file.
// Use of the fields is documented in that file.

struct IN_SYS_PARMS {
	U_char 		sys_run_lvl;
	Bool		sys_crerror_inh;
	U_short		aru_timer;
	U_short		sys_error_threshold;
	U_short		sys_error_dec_rate;
	U_long		init_flags;
	U_long		process_flags;
	U_short		default_restart_interval;
	U_short		default_restart_threshold;
	U_short		default_sanity_timer;
	U_short		default_init_complete_timer;
	U_short		default_procinit_timer;
	U_short		default_error_threshold;
	U_short		default_error_dec_rate;
	U_short		default_priority;
	U_short		default_create_timer;
	Long		default_q_size;
	U_short		default_lv3_timer;
	U_short		default_global_queue_timer;
	int		default_brevity_low;
	int		default_brevity_high;
	int		default_brevity_interval;
	Short		default_msg_limit;
	U_char		first_runlvl;
	int		maxCargo;
	int		minCargoSz;
	int		cargoTmr;
	int		buffered;
	int		num256;
	int		num1024;
	int		num4096;
	int		num16384;
	int		msgh_ping;
	char            percent_load_on_active;
	int		shutdown_timer;
	int             num_msgh_outgoing;
	int		num_lrg_buf;
	int		debug_timer;
	int		safe_interval;
	char		core_full_minor;
	char		core_full_major;
	char  		resource_groups[INmaxResourceGroups]; 
	char    	active_nodes[INmaxResourceGroups][INmaxNodeName+1];
	char    	oam_lead[INmaxResourceGroups][INmaxNodeName+1];
	char    	oam_other[INmaxResourceGroups][INmaxNodeName+1];
	short		network_timeout;
	char		vhost[INmaxVhosts][INmaxNodeName+1];
	char		vhostfailover_time;

#ifdef __sun
	processorid_t	pset[INmaxPsets][INmaxProcessors];
#else
	/* Linux currently does not support processor sets, however
	** most of this code will be compiled on Linux in case it
	** processor sets are supported in the future.
	** So, pset is defined in all environments.
	*/
	int		pset[INmaxPsets][INmaxProcessors];
#endif
};

// Enums corresponding to indexes of parameter strings in INparm_names array.

typedef enum {
	IN_SYS_RUN_LVL,
	IN_ARU_TIMER,
	IN_PROCESS_FLAGS,
	IN_INIT_FLAGS,
	IN_SYS_ERROR_THRESHOLD,
	IN_SYS_ERROR_DEC_RATE,
	IN_SYS_CRERROR_INH,
	IN_SYS_INIT_THRESHOLD,
	IN_VMEM_MINOR,
	IN_VMEM_MAJOR,
	IN_VMEM_CRITICAL,
	IN_DEFAULT_RESTART_INTERVAL,
	IN_DEFAULT_SANITY_TIMER,
	IN_DEFAULT_RESTART_THRESHOLD,
	IN_DEFAULT_INIT_COMPLETE_TIMER,
	IN_DEFAULT_PROCINIT_TIMER,
	IN_DEFAULT_ERROR_THRESHOLD,
	IN_DEFAULT_ERROR_DEC_RATE,
	IN_DEFAULT_PRIORITY,
	IN_DEFAULT_CREATE_TIMER,
	IN_DEFAULT_Q_SIZE,
	IN_DEFAULT_LV3_TIMER,
	IN_DEFAULT_GLOBAL_QUEUE_TIMER,
	IN_DEFAULT_BREVITY_LOW,
	IN_DEFAULT_BREVITY_HIGH,
	IN_DEFAULT_BREVITY_INTERVAL,
	IN_DEFAULT_MSG_LIMIT,
	IN_FIRST_RUNLVL,
	IN_MAX_CARGO,
	IN_MIN_CARGO_SZ,
	IN_CARGO_TMR,
	IN_BUFFERED,
	IN_NUM256,
	IN_NUM1024,
	IN_NUM4096,
	IN_NUM16384,
	IN_MSGH_PING,
	IN_PERCENT_LOAD_ON_ACTIVE,
	IN_SHUTDOWN_TIMER,
	IN_NUM_MSGH_OUTGOING,
	IN_NUM_LRG_BUF,
	IN_DEBUG_TIMER,
	IN_SAFE_INTERVAL,
	IN_CORE_FULL_MINOR,
	IN_CORE_FULL_MAJOR,
	IN_PS000,		/* Insert new non-optional system parameters before this one 	*/
	IN_PS127 = IN_PS000 + 127, /* Insert optional system parameters below			*/
	IN_ACTIVE_NODES,
	IN_RESOURCE_GROUPS,
	IN_OAM_LEAD,
	IN_OAM_OTHER,
	IN_VHOST,
	IN_VHOST_FAILOVER_TIME,
	IN_MAX_BOOTS,
	IN_NETWORK_TIMEOUT,
	IN_FAILOVER_ALARM,
	IN_MSGH_NAME,
	IN_RUN_LVL,
	IN_PATH,
	IN_USER_ID,
	IN_GROUP_ID,
	IN_PRIORITY,
	IN_SANITY_TIMER,
	IN_RESTART_INTERVAL,
	IN_RESTART_THRESHOLD,
	IN_INHIBIT_RESTART,
	IN_PROCESS_CATEGORY,
	IN_INIT_COMPLETE_TIMER,
	IN_PROCINIT_TIMER,
	IN_ERROR_THRESHOLD,
	IN_ERROR_DEC_RATE,
	IN_CRERROR_INH,
	IN_INHIBIT_SOFTCHK,
	IN_MSGH_QID,
	IN_OFC_PATH,
	IN_CREATE_TIMER,
	IN_Q_SIZE,
	IN_ON_ACTIVE,
	IN_LV3_TIMER,
	IN_GLOBAL_QUEUE_TIMER,
	IN_BREVITY_LOW,
	IN_BREVITY_HIGH,
	IN_BREVITY_INTERVAL,
	IN_MSG_LIMIT,
	IN_EXT_PATH,
	IN_THIRD_PARTY,
	IN_RT,
	IN_PS,
	IN_OAMLEADONLY,
	IN_ACTIVE_VHOST_ONLY,
	IN_PARM_MAX		/* This should always be the last value */
} IN_PARM_INDEX;

typedef enum {
	IN_NO,
	IN_YES,
	IN_MAX_BOOL
} IN_BOOL_VAL;

#define IN_ISACTIVE(state)	(((state) == S_ACT) || ((state) == S_LEADACT))
#define IN_ISSTBY(state)	((state) == S_STBY)

#define IN_SLEEPN(sec, nsec)	{                 \
    struct timespec tsleep;                     \
    tsleep.tv_sec = sec;                        \
    tsleep.tv_nsec = nsec;                      \
    nanosleep(&tsleep,NULL);                    \
  }					
#define IN_SLEEP(time)		IN_SLEEPN(time,0)

/* Minimum values for process parameters */
#define IN_MIN_SANITY		12	/* Minimum sanity timer			*/
#define IN_MIN_RESTART		10	/* Minimum restart interval		*/
#define IN_MIN_RESTART_THRESHOLD 2	/* Minimum restart threshold		*/
#define IN_MIN_INITCMPL		5	/* Minimum init complete timer 		*/
#define IN_MIN_PROCCMPL		5	/* Minimum procinit complete timer 	*/
#define IN_MIN_ERROR_THRESHOLD	10	/* Minimum error threshold		*/
#define IN_MIN_ERROR_DEC_RATE	3	/* Minimum error decrement rate		*/
#define IN_MIN_INIT_THRESHOLD	2	/* Minimum number of system inits	*/


typedef enum {		
	INSU_APPLY,
	INSU_BKOUT,
	INSU_COMMIT
} INSUACTION;

///* This structure contains information describing current SU */
//typedef struct {
//  char  obj_path[IN_PATHNMMX];    /* Path of the object file      */
//  SN_LVL  sn_lvl;                 /* Initialization level at which the
//                                  ** SU for this process should occur
//                                  */
//	Bool	kill;			/* Use kill -9 instead of SUexitTyp */
//	Bool	applied;		/* TRUE if associated files already applied
//                    ** for this object.  This is necessary when
//                    ** more than one process uses the same
//                    ** object file, i.e. lmt.
//                    */
//	Bool	changed;		/* TRUE if the process is actually changed
//                    ** as part of the SU instead of having to
//                    ** be reinitialized as a result of changes
//                    ** to other files.
//                    */ 
//	Bool	new_obj;		/* TRUE if this is a new object file	*/
//  char  file_path[SU_MAX_OFILES][IN_PATHNMMX];
//  /* Path to other files associated with this
//  ** object file that must be maintained 
//  ** consistent during SU.  Usually those
//  ** files will represent images, however
//  ** INIT does not care what they are.
//  */
//	Bool  new_file[SU_MAX_OFILES];	/* TRUE if this a new file added in this SU.
//                                  ** Files added in an SU are marked with
//                                  ** a "+" in front of the path name.
//                                  */
//        
//} IN_SU_PROCINFO;

//extern int INinit_su_idx;               /* Index in INsudata of INIT if it is in the SU,
//                                        ** negative number otherwise.
//                                        */
//extern IN_SU_PROCINFO INsudata[];	/* Current SU information	*/
extern Bool INsupresent;		/* True if SU present in the system */

/*
 * This macro was used in the INIT code when it was originally ported and
 * has been updated to use the more-recently developed USLI trace and
 * error macros.  Note that, to turn on INIT trace, the "INinitTrace" flag
 * must be set.
 */
//const	CRdbflag INinitTrace = 32;	/* Enable INIT tracing */

extern int	INerr_cnt;
extern long	INinit_start_time;

#define INIT_ERROR(out_data)                                \
	CRERROR out_data ;                                        \
	if(++INerr_cnt > IN_MAX_INIT_ERRORS){                     \
		INescalate(SN_LV0,INPROCERR_THRESH,IN_SOFT,INIT_INDEX); \
	}                                                         \

#define INIT_DEBUG(type,out_data)               \
	if (type & IN_trace) {                        \
		CR_PRM out_data	;                           \
	}


const U_short	IN_MAXDEATHS = 2;	/* Max number of permanent proc */
/* deaths/re-inits allowed in a */
/* timed intrvl before escalation */


const U_short INMAXARU = 600;	/* Maximum ARU polling interval */

#ifdef EES
// IBM thomharr 20060908 - need type.
const int INNUMCORE = 10;	/* Number of core files which may be stored for a */
/* process -- note that this value should not */
/* exceed 26 as the suffixes added to the core */
/* files range from "A" through "Z"...'A' + 27 is */
/* '[' in ASCII which is not such a good suffix! */
#else
const int INNUMCORE = 5;
#endif

/*
 * 11/18/91:
 * The following value is the delay passed in all calls to
 * "MHinfoExt::broadcast()".  It is the time, in milliseconds, which
 * the MSGH library will allow us to be blocked (per destination queue)
 * while attempting to send messages.  Experience has shown that it is
 * relatively important that the "INpDeath" message not get dropped -
 * SPMAN and the SCHs take important state change/resource deallocation
 * actions when they receive "INpDeath" messages.  Therefore, the potential
 * extra delay (and, perhaps, additional burden on message queue resources
 * caused by being more persistent when sending messages) is probably
 * worth it...future tuning, of course, cannot be ruled out!
 */
const Long	INbrdcast_delay = 20;	/* 20 milliseconds/destination queue */

/*
 * The following is used for manipulating indices into INIT's local
 * message queue and INIT's recovery queue.  It evaluates to the value
 * of the next valid index into the queue beyond that index used as its
 * parameter.
 */
#define IN_NXTINDX(i, qsize)	((i < (qsize - 1)) ? (i+1) : 0)

extern U_long		IN_trace;	/* INIT proc's trace flags */
extern const Char	INdinitlist[];
extern const Char	INinhfile[];
extern const Char      	INinitcount[];	/* initcount file name		*/
extern const Char	INexecdir[];	/* First part of path used to set */
/* working directory for children */
/* procs.			  */
extern const Char	INsufile[];	/* File containing SU information */
extern Char INsupath[];			/* Path to the location of the current SU */
extern const Char	INinitpath[];	/* Path to INIT executable 	*/
extern const Char	INhistfile[];	/* Path to SU HISTORY  file 	*/
extern const Char	INapp_res_name[]; /* Name of application resource*/
extern const Char	INtimeFile[];	/* Path to timestamp file of last activity */
extern Bool INroot;			/* "TRUE" if INIT's UID is zero */
extern Bool INworkflg;			/* TRUE if sequencing work to do */
extern Bool INgqfailover;		/* TRUE if gq failover occured	*/
extern Long INsys_init_threshold;	/* Threshold of rolling inits	*/
extern Long INmax_boots;		/* Threshold of Unix boots 	*/
extern Bool INShut;			/* TRUE if shutdown script completed */
extern Bool INmhmutex_cleared;		/* TRUE if messaging mutex was cleared */
extern Bool INissimplex;

#define INscriptsStart		0
#define INscriptsStop		1
#define INscriptsFailover	2

#define INscriptsNone		0
#define INscriptsRunning	1
#define INscriptsFinished	2
#define INscriptsFailed		3

extern int  INscriptsState;

extern "C" void* INrunScriptList(void*);


#define ETC_INIT_PID	(pid_t)1	/* pid of /etc/init		*/
extern pid_t  INppid;			/* parent pid of INIT		*/
extern Bool  INcmd;			/* TRUE if INIT run from command line */

extern MHqid	INmsgqid;		/* INIT MSGH queue ID */

extern EHEVTYPE INetype;	/* Flag used to determine whether we're   */
/* receiving messages from MSGH or not    */
extern char	INmyPeerHostName[MHmaxNameLen + 1]; /* Name of this node from peer perspective 	*/
extern Short	INmyPeerHostId;			    /* Hostid of this node from peer perspective */ 
extern Bool	INcanBeOamLead;			    /* True if the node can be OA&M lead	*/
extern int	INnoOamLeadMsg;			    /* Count of times no OAM lead msg received	*/

extern char	INvhostMateName[];

extern short INprio_map[IN_MAXPRIO];   /* Priority mapping array */
#ifdef __sun
extern pcinfo_t INpcinfo;
#endif

/*
 * MSGH utility routine to remove all the UNIX queues:
 */

extern Bool 	INlevel4;
/*
 * Routines found in INmain.C
 */
extern Void	INfreeres(Bool release_init=TRUE);
extern Void	INinit();
extern Void	INinitptab(U_short);
extern Void	INdef_parm_init(IN_PROC_PARMS *,IN_SYS_PARMS *);
extern "C" Void	INsigcld(int);
extern pid_t	INcheck_zombie();
extern "C" Void	INsigterm(int, siginfo_t*, void*);
extern "C" Void	INsigint(int);
extern "C" Void	INsigsu(int);
extern "C" Void	INsigsnstop(int, siginfo_t*, void*);
extern Void	INprocinit();
extern Void	INdump(Char *);
extern Void	INprintHistory(Void);
extern Void	INprtDetailProcInfo(Char* msgh_name);
extern Void	INprtGeneralProcInfos(Void);
extern int	INcountprocs();
extern Void 	INmain_init();
extern Long	INlv4_count(Bool, Bool update = TRUE);
extern GLretVal INkernel_map();
extern Void	INsigalrm();
extern Void	INsanset(Long);
extern Void	INsanstrobe();
extern "C" Void*INmhMutexCheck(void*);

/*
 * Routines found in INcreate.C
 */
extern GLretVal	INcreate(U_short);
extern GLretVal INcheck_image(U_short);

/*
 * Routines found in INrdinls.C
 */
extern Short	INrdinls(Bool,Bool);
extern GLretVal	INgettoken(char *,char *&,int &);
extern GLretVal	INgetpath(char *, Bool, Bool check_dir = FALSE);
extern Short 	INmatch_string(char **,char *,Short);
extern Bool 	INis_numeric(char *,char *,CRALARMLVL,int);
extern int 	INconvparm(char *,char *,Bool,int,int);
extern int 	INfindproc(const char *);

/*
 * Routines found in INsync.C
 */
extern Void	INsequence();
extern Void	INgqsequence();
extern GLretVal	INscanstep(IN_SYNCSTEP);
extern GLretVal	INscanstate(IN_PROCSTATE);
extern Void	INinitover();
extern GLretVal	INsync(U_short, IN_SYNCSTEP, MHqid realQ = MHnullQ, MHqid gqid = MHnullQ);
extern Void	INpkill(U_short,IN_SYNCSTEP,IN_SYNCSTEP);
extern Void	INnext_rlvl();
extern GLretVal	INmvsufiles(U_short,SN_LVL &,INSUACTION);
extern GLretVal	INmvsuinit(INSUACTION);
extern Bool	INgetRealName(char*, char*);

/*
 * Routines found in INrestart.C
 */
extern Void	INgrimreaper();
extern Void 	INdeath(U_short);
extern GLretVal INreqinit(SN_LVL, U_short, GLretVal, IN_SOURCE, const char *);
extern GLretVal	INsetrstrt(SN_LVL,U_short,IN_SOURCE);
extern Void 	INdeadproc(U_short,Bool);
extern Void	INkillprocs(Bool sendSigterm = TRUE);
extern Void	INautobkout(Bool, Bool);
extern Void	INfreeshmem(U_short,Bool);
extern GLretVal INrun_bkout(Bool);
extern "C" Void	INalarm(int);
extern Void	INsys_bk();
extern GLretVal	INfinish_bkout();
extern int	INsudata_find(char *);
extern Void	INfreesem(U_short);
extern int	INkill(U_short indx, int sig);

/*
**  Routines found in INmachdep.C
*/


extern Void	INchkcore(const Char *);
extern Long	INcheckDirSize(Void);

/*
 * Routine found in INtimerproc.C
 */
extern Void	INtimerproc(U_short);
extern Void	INsettmr(INTMR &, Long, U_short,  Bool, Bool);
extern Void	INarumsg(Void);
extern Void	INchecklead(Void);

/*
 * Routine found in INrcvmsg.C
 */
extern Void	INrcvmsg(Char *, int);
extern GLretVal INgetsudata(Char *,Bool);
extern Bool 	INisdupfile(Char *);
extern Void 	INgetMateCC(Char *, Bool getMyName = TRUE);

/*
 * Routines found in INaudit.C:
 */
extern GLretVal	INaudit(Bool);
extern Void	INescalate(SN_LVL, GLretVal, IN_SOURCE, Short);
extern Void	INsysreset(SN_LVL, GLretVal, IN_SOURCE, Short);
extern Void	INsysboot(GLretVal, Bool bAllNodes = FALSE, Bool printPRM = TRUE);
extern Void	INcheck_progress();
extern Void	INwait_exit();
extern Bool	INic_crit_up(Short);
extern Void	INcheck_sanity();
extern Void	INvmem_check(Bool,Bool);
extern Void	INswitchVhost();

/* Routines found in INrm_thread.C */
extern Void	INrm_check_state(char);
extern char	INgetaltstate(void);
extern CRALARMLVL INadjustAlarm(CRALARMLVL);

#ifndef EES
extern "C" void*INshutOracle(void*);
#endif

/*
 * Enum-to-text-string mappings not extern'ed in "cc/hdr/INinit.h"
 */
extern const char	*IN_pstatenm[];
extern const char	*IN_statenm[];
extern const char	*IN_snlvlnm[];
extern const char	*IN_srcnm[];
extern const char	*IN_proccatnm[];
 
extern unsigned long	INsanityPeg;
extern unsigned long	INwarnMissed;
extern clock_t		INticksMissed;
extern int		INvhostMate;


#define INmaxWdPorts	MAX_LINK
#define INmaxClusters	4
#define INsanityTimeout	120		// INIT mainloop maximum timeout
#define INsanityWarnTimeout 10		// Timeout duration before warning issued
#define IN_LDCURSTATE		IN_NODE_STATE[IN_MYNODEID]

extern char INqInUse[MHmaxQid];
