//
// DESCRIPTION:
//	This file includes the definition of the in_ldata structure used
//	by the INIT library included by each permanent process.
//
// NOTES:
//
#include <sys/types.h>
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/eh/EHhandler.hh"	/* EHEVTYPE definition */

#include "cc/hdr/init/INproctab.hh"
#include "cc/init/proc/INtimers.hh"
#include "cc/init/proc/INlocal.hh"

//#include "cc/hdr/cr/CRomInfo.hh"

/*
 *      This array contains the text strings used when printing out the
 *      process category as part of the OP:INIT report.
 */
 
const char *IN_proccatnm[] = {
                "IN_NON_CRITICAL",
                "IN_INIT_CRITICAL",
                "IN_CP_CRITICAL",
                "IN_PSEUDO_CRITICAL"
};

/*
 *      This array contains text strings representing the
 *      process types.
 */
const char *IN_pstatenm[] = {
	"INVPROC",
	"IN_PERMPROC",
	"IN_TEMPPROC",
};

/*
 *	This array contains text strings representing the
 *	system initialization states.
 */
const char *IN_statenm[] = {
	"INV_INIT",	/* Uninitialized initialization state */
	"INITING",	/* In a system reset/boot */
	"INITING2",
	"IN_CUINTVL",	/* In a timed boot interval after a reset/boot */
	"IN_NOINIT",	/* Steady state, no init. interval in progress */
};

/*
 *	This array contains text strings representing enum values for
 *	IN_SOURCE:
 */
const char *IN_srcnm[] = {
	"IN_INVSRC",	/* Uninitialized 				*/
	"IN_RUNLVL",	/* Init. caused by increase in run level	*/
	"IN_MANUAL",	/* Initialization Invoked Manually 		*/
	"IN_INIT",	/* Initialization caused by INIT proc's death	*/
	"IN_SOFT",	/* Initialization Requested from SN Software 	*/
	"IN_PROC",	/* Init. caused by exceeding proc. restart threshold */
	"IN_TIMEOUT",	/* Init. caused by rstrt timer exp. during sys. reset*/
	"IN_BOOT",	/* Init. caused by a UNIX boot			*/
	"IN_MAXSOURCE",	/* Used for range checking, new source types	*/
};

/* Global data */
IN_SDATA	*IN_sdata = NULL;	/* Ptr to shmem seg. access by INIT's chldren*/
IN_PROCDATA	*IN_procdata = (IN_PROCDATA *) -1;/* Ptr to INIT's "private" shared memory seg.*/

U_long	IN_trace = IN_ALWAYSTR;	/* Global data used for INIT proc's trace flg*/
long	INinit_start_time;

int	INinit_su_idx = -1;		/* Index in INsudata of INIT if it is in the SU,
					** negative number otherwise.
					*/
//IN_SU_PROCINFO INsudata[SU_MAX_OBJ];	/* Data for current SU	*/
Bool	INsupresent = FALSE;		/* TRUE if SU present in the system */

Char		INsupath[IN_PATHNMMX];

#ifdef EES
const Char	INdinitlist[]="cc/init/EEinitlist";
const Char	INinhfile[]="softchk";
const Char	INinitcount[]="initcount";
const Char	INsufile[]="suinfo";
const Char	INinitpath[]="cc/init/proc/EE/EEinit";
const Char	INhistfile[]="./HISTORY";
const Char	INtimeFile[] ="./timestamp";
#else
const Char	INdinitlist[]="/sn/init/initlist";
const Char	INinitcount[]="/sn/init/initcount";
#ifdef __linux
const Char	INinhfile[]="/opt/config/status/softchk";
#else
const Char	INinhfile[]="/sn/init/softchk";
#endif
const Char	INsufile[]="/sn/init/suinfo";
const Char	INinitpath[]="/opt/sn/init/init";
const Char	INhistfile[]="/sn/release/HISTORY";
const Char	INtimeFile[] ="/sn/init/timestamp";
#endif

const Char	INexecdir[]="/sn/core";

Bool	INroot;			/* Flag indicating whether INIT is being */
				/* run with a UID of zero */
Bool	INworkflg;		/* True if sequencing work to do */
Bool	INgqfailover = FALSE;	/* True if gq failover in progress	*/
pid_t	INppid;			/* INIT's parent process id	*/
Bool	INcmd;			/* TRUE if INIT run from command line	*/
Bool	INShut;			/* TRUE if shutdown scripts finished */
Long	INnoenv_vars;		/* keeps track of unset environment variables */
Long	INsys_init_threshold=1000;/* Threshold of rolling initializations */
Long	INmax_boots=0;		/* Threshold of unix boots, 0 = no limit	 */
Long	INvmem_major;		/* Threshold of available virtual memory in K bytes 
				** which will result in generation of major alarm
				*/
Long	INvmem_minor;		/* Threshold of available virtual memory in K bytes 
				** which will result in generation of minor alarm
				*/
Long	INvmem_critical;	/* Threshold of available virtual memory in K bytes 
				** which will result in generation of critical alarm
				*/
CRALARMLVL  INfailover_alarm = POA_INF; /* Alarm level associated with failover message	*/
Bool  INdidFailover = FALSE;

const struct anoninfo * INanoninfo = 0; 
				/* Memmaped pointer to kernel anoninfo variable
				** which keeps track of available swap space.
				** This structure is defined in vm/anon.h.
				*/

INPTMRS	INproctmr[IN_SNPRCMX];	/* Per-proc. timers 			*/

INTMR INinittmr;		/* INIT proc sequencing timer */
INTMR INpolltmr;		/* INIT main loop "polling" timer */
INTMR INarutmr;			/* ARU "keep-alive" timer */
INTMR INaudtmr;			/* INIT audit timer	*/
INTMR INvmemtmr;		/* Timer to generate periodic VMEM OM	*/
INTMR INsanitytmr;		/* Timer to generate periodic sanity peg*/
INTMR INcheckleadtmr;		/* Timer to check lead availablity	*/
INTMR INsoftchktmr;             /* Timer to fire off the alaram if software chekc is inhibited for 
                                   more than 20 min. */ 
INTMR INsetLeadTmr;             /* Timer to inforce the lead in A/A configurations 	*/
INTMR INoamLeadTmr;		/* Cyclic timer for OAM Lead synchronization		*/
INTMR INoamReadyTmr;		/* Timer for OAM Lead ready transition			*/
INTMR INsetActiveVhostTmr;	/* Timer for determining active vhost	 */
INTMR INvhostReadyTmr;		/* Timer for determining active vhost transition	*/

class EHhandler	INevent;		/* INIT event handler */

MHqid	INmsgqid = MHnullQ;	/* INIT MSGH queue ID */
int	INvhostMate = -1;	/* index of the vhost mate		*/

int 	INerr_cnt = 0;		/* Count of INIT errors 		*/
EHEVTYPE INetype = EHTMRONLY;	/* Flag passed to "EHhandler::getEvent()" */
				/* which determines whether or not we're  */
				/* currently accepting MSGH messages	  */
Bool	INmhmutex_cleared = FALSE;	/* TRUE if messaging mutex cleared */
//CRomInfo INcepOmInfo;		/* terminal from which init:proc or       */
				/* init:scn was entered                   */
Bool INuseCepOmInfo;		/* cep OM info is ok                      */

char INvhostMateName[(MHmaxNameLen + 1) * 2];

/* Following are the rules for adding entries to this table:
** 1. All global system parameters have to go at the beginning of the list.
** 2. msgh_name should always start the per process entries.
** 3. user_id should always end the list of required per process entries,
**    i.e. any parameters that become required should be put between 
**    msgh_name and user_id.
** 4. INlocal.H header has to be updated to add an entry for a new parameter.
*/

const char * INparm_names [] = {
	"sys_run_lvl",
	"aru_timer",
	"process_flags",
	"init_flags",
	"sys_error_threshold",
	"sys_error_dec_rate",
	"sys_crerror_inh",
	"sys_init_threshold",
	"vmem_minor",
	"vmem_major",
	"vmem_critical",
	"default_restart_interval",
	"default_sanity_timer",
	"default_restart_threshold",
	"default_init_complete_timer",
	"default_procinit_timer",
	"default_error_threshold",
	"default_error_dec_rate",
	"default_priority",
	"default_create_timer",
	"default_q_size",
	"default_lv3_timer",
	"default_global_queue_timer",
	"default_brevity_low",
	"default_brevity_high",
	"default_brevity_interval",
	"default_msg_limit",
	"first_bootlvl",
	"max_cargo",
	"min_cargo_sz",
	"cargo_tmr",
	"buffered",
	"num256",
	"num1024",
	"num4096",
	"num16384",
	"msgh_ping",
	"percent_load_on_active",
	"shutdown_timer",
	"num_msgh_outgoing",
	"num_lrg_buf",
	"debug_timer",	
	"safe_interval",	
	"core_full_minor",
	"core_full_major",
	"ps000",		/* This is a start of non-mandatory parameters */
	"ps001",
	"ps002",
	"ps003",
	"ps004",
	"ps005",
	"ps006",
	"ps007",
	"ps008",
	"ps009",
	"ps010",
	"ps011",
	"ps012",
	"ps013",
	"ps014",
	"ps015",
	"ps016",
	"ps017",
	"ps018",
	"ps019",
	"ps020",
	"ps021",
	"ps022",
	"ps023",
	"ps024",
	"ps025",
	"ps026",
	"ps027",
	"ps028",
	"ps029",
	"ps030",
	"ps031",
	"ps032",
	"ps033",
	"ps034",
	"ps035",
	"ps036",
	"ps037",
	"ps038",
	"ps039",
	"ps040",
	"ps041",
	"ps042",
	"ps043",
	"ps044",
	"ps045",
	"ps046",
	"ps047",
	"ps048",
	"ps049",
	"ps050",
	"ps051",
	"ps052",
	"ps053",
	"ps054",
	"ps055",
	"ps056",
	"ps057",
	"ps058",
	"ps059",
	"ps060",
	"ps061",
	"ps062",
	"ps063",
	"ps064",
	"ps065",
	"ps066",
	"ps067",
	"ps068",
	"ps069",
	"ps070",
	"ps071",
	"ps072",
	"ps073",
	"ps074",
	"ps075",
	"ps076",
	"ps077",
	"ps078",
	"ps079",
	"ps080",
	"ps081",
	"ps082",
	"ps083",
	"ps084",
	"ps085",
	"ps086",
	"ps087",
	"ps088",
	"ps089",
	"ps090",
	"ps091",
	"ps092",
	"ps093",
	"ps094",
	"ps095",
	"ps096",
	"ps097",
	"ps098",
	"ps099",
	"ps100",
	"ps101",
	"ps102",
	"ps103",
	"ps104",
	"ps105",
	"ps106",
	"ps107",
	"ps108",
	"ps109",
	"ps110",
	"ps111",
	"ps112",
	"ps113",
	"ps114",
	"ps115",
	"ps116",
	"ps117",
	"ps118",
	"ps119",
	"ps120",
	"ps121",
	"ps122",
	"ps123",
	"ps124",
	"ps125",
	"ps126",
	"ps127",
	"active_nodes",
	"resource_groups",
	"oam_lead",
	"oam_other",
	"vhost",
	"vhost_failover_time",
	"max_boots",
	"network_timeout",
	"failover_alarm",
	"msgh_name",		/* Start of per-process parameters	*/
	"run_lvl",
	"path",
	"user_id",
	"group_id",
	"priority",
	"sanity_timer",
	"restart_interval",
	"restart_threshold",
	"inhibit_restart",
	"process_category",
	"init_complete_timer",
	"procinit_timer",
	"error_threshold",
	"error_dec_rate",
	"crerror_inh",
	"inhibit_softchk",
	"msgh_qid",
	"ofc_path",
	"create_timer",
	"q_size",
	"on_active",
	"lv3_timer",
	"global_queue_timer",
	"brevity_low",
	"brevity_high",
	"brevity_interval",
	"msg_limit",
	"ext_path",
	"third_party",
	"rt",
	"ps",
	"oamleadonly",
	"active_vhost_only"
};

const char * INbool_val[] = {
	"NO",
	"YES",
};

const char* INrt_val[] = {
	"NO",
	"RR",
	"FIFO"
};

/* Process category values */
const char * INcategory_vals[] = {
	"NC",		// Non - critical 
	"IC",		// Initialization critical
	"CC",		// Call processing critical
	"PC"		// Pseudo critical
};

#ifdef __sun
/* Structure for obtaining class parameters */
pcinfo_t INpcinfo;
#endif

/* Mapping of AINET priorities to UNIX priorities */
/**************************************************************
** IMPORTANT: if this table is changed, corresponding table in
** cc/lib/init/INcommon.C must also be changed to match.
***************************************************************/
short INprio_map[IN_MAXPRIO] = {
29,	/* 0 = init */
26,
26,
26,
26,	/* 4 = first of call processing priority */
26,
23,
23,
23,
20,
20,
17,
17,	/* 12 = last of call processing priority */
16,	/* 13 = first of high OAM processes */
13,
13,
10,
10,
7,
7,	/* 19 = last of high OAM processes */
0,	/* 20 = regular OAM and default */
-3,
-3,
-6,
-6,	/* below this priority, the process may not escalate above call processing */
-9,
-9,
-12,
-12,
-15,
-15,
-18,
-18,
-21,
-21,
-24,	
-24,	/* 36 = filler class */
-27,	/* 37 = filler class */
-27,	/* 38 = filler class */
-27	/* 39 = filler class */
};

unsigned long	INsanityPeg = 0;
clock_t		INticksMissed = 0;
unsigned long	INwarnMissed = 0;
char	INqInUse[MHmaxQid];
