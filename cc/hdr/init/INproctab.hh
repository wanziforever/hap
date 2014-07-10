#ifndef __INPROCTAB_H
#define __INPROCTAB_H

#include <stdio.h>
#include <sys/types.h>
#include <pthread.h>

#include "hdr/GLreturns.h"
#include "cc/hdr/init/INinit.hh"
#include "cc/hdr/msgh/MHnames.hh"
#include "cc/hdr/msgh/MHqid.hh"
#include "cc/hdr/msgh/MHdefs.hh"

#define INPDATA (INMSGBASE+0xff) // Key for IN_PROCDATA's shared memory seq
// IMPORTANT: IN_SHM_VER contains shared memory version information to
// prevent system corruption in the case of incompatible processes.
// This variable must be updated every time shared memory layout
// changes in any way. The version should be updated by adding +1 before
// the ) terminating #define. A comment should be added explaining
// the change and load changes was made in.
#define IN_SHM_VER (0x0a0a0000 + 1)

#define IN_SHM_ERR 254

#define IN_PATHNMMX 164 // Max Characters in Pathname
#define IN_EPATHNMMX 124 // Max Characters in Ext Pathname
#define IN_OPATHNMMX 80 // Max Characters in Ofc Pathname

#define IN_NAMEMX (MHmaxNameLen+1) // Max space for characters in process name
#define IN_NUMINITS 10 // Max number of system-wide inits, for
                       // which information will be kept
#define IN_FREEPID (pid_t)(-1) // No PID Specified

#define IN_MAXPRIO  40  // Maximum application process priority
                        // All processes
                        // must have priority lower then this.
#define INTS_MAXUPRI 59 // Maximum kernel level priority range
                        // that we expect to have configured
#define INMAP_PRIO(priority) ((priority) >= 0 && (priority) < IN_MAXPRIO ? \
                              INprio_map[priority]:INprio_map[IN_MAXPRIO/2])
#define INmaxResourceGroups 32
#define INmaxResource 31
#define INmaxNodeName 15
#define INmaxVhosts 2
#define INfixQ  4096

// This typedef is used to set the system's state indicating whether
// the system is currently in a boot or system reset sequence
typedef enum {
  INV_INIT, // Uninitialized initialization state
  INITING,  // In a system reset/boot
  INITING2, // Second attempt to complete a system reset
  IN_CUINTVL, // In a timed boot interval after a reset/boot
  IN_NOINIT,  // Steady state, no init, interval in progress
  IN_MAXINIT, // Used for range checking, new INIT states
              // should be added BEFORE this one
  INITSTATE_BOGUS_SUNPRO
} INITSTATE;

// This typedef is used to distinguish permanent processes (i.e. processes
// read from the initlist) from temporal processes (i.e. processes started
// as a result of a request send to INIT via a MSGH msg.)
typedef enum {
  INVPROC,
  IN_PERMPROC,
  IN_TEMPPROC,
  IN_MAXPTYPE, // Used for range checking, new process types
               // should be added BEFORE this one
  iN_PERMSTATE_BOGUS_SUNPRO
} IN_PERMSTATE;

// This typedef is used to distinguish the update state of a permanent
// process. (Used when a software update (aka BWM) is being applied.)
typedef enum {
  NO_UPD,        // Not updating
  UPD_PRESTART,  // Updating, but new version not started yet
  UPD_POSTSTART, // Updating, new version running
  IN_UPDSTATE_BOGUS_SUNPRO
} IN_UPDSTATE;

// This typedef is used to set each process's restart flag. If a process
// dies and its restart flag is set to "IN_INHRESTART" it will NOT be
// restarted
typedef enum {
  IN_INHRESTART,
  IN_ALWRESTART,

  IN_STARTSTATE_BOGUS_SUNPRO
}IN_STARTSTATE;

// This typedef is used to set system or process software check inhibits.
// If softchk flag is set to IN_INHSOFTCHK then no recovery action will
// be performed on a process.
typedef enum {
  IN_INHSOFTCHK,
  IN_ALWSOFTCHK
} IN_SOFTCHK;

// This typedef is used to categorize process types.
typedef enum {
  IN_NON_CRITICAL,
  IN_INIT_CRITICAL,
  IN_CP_CRITICAL,
  IN_PSEUDO_CRITICAL, // Temporary designation for CP_CRITICAL processes
                      // that have not yet been converted to meet the
                      // required interface
  IN_MAX_CAT
} IN_PROC_CATEGORY;

// This typedef is used to determine which signal handling routine to
// call during process synchronization. This mechanism allows the INIT
// subsystem to "multiplex" two logical signal types onto a single
/// sigla -- "SIGUSR2"
typedef enum {
  IN_SIGINV,  //Invalid entry
  IN_SIGSYNC, // SYNC signal
  IN_SIGHALT, // HALT signal

  IN_SIGTYPE_BOGUS_SUNPRO
} IN_SIGTYPE;

// This typedef is used to determine the source of the previous system
// wide initialization
typedef enum {
  IN_INVSRC,     // Uninitialized
  IN_RUNLVL,     // Init. caused by increase in run level
  IN_MANUAL,     // Initialization Invoked Manually
  IN_INIT,       // Initialization caused by INIT proc's death
  IN_SOFT,       // Initialization Requested from SN Software
  IN_PROC,       // Init. caused by exceeding proc. restart threshold
  IN_TIMEOUT,    // Init. caused by rstrt timer exp. hring sys. reset
  IN_BOOT,       // Init. caused by a UNIX boot
  IN_MAXSOURCE,  // Used for range checking, new source types
                 // should be added BEFORE this one.
  IN_SOURCE_BOGUS_SUNPRO
} IN_SOURCE;

typedef enum {
  IN_NOTRT,
  IN_RR,
  IN_FIFO,
  IN_MAX_RT
} IN_RT_VAL;

// Initialization Information Structure. This structure contains
// relevant system information about the most recent initialization.
typedef struct {
  U_short ld_indx;  // "Load" index into "init_data" array
  U_short uld_indx; // "Unload" index into "init_data" array
  int fillInt;
  struct {
    U_long str_time;  // Time Initialization Began
    U_long end_time;  // Time Initialization Completed
    SN_LVL psn_lvl;   // Last Initialization Level
    IN_SOURCE source; // Source of Initialization
#if defined(c_plusplus) | defined(__cplusplus)
    GLretVal ecode;  // Set when source is IN_SOFT
#else
    RET_VAL ecode;   // Set when source is IN_SOFT
#endif
    U_short num_procs; // Number of proc, involved
    U_char prun_lvl;  // Last run Level
    char fillChar;
    Short fillshort;
    Char msgh_name[IN_NAMEMX]; // Proc. which requested on initialization
  } init_data[IN_NUMINITS];
} IN_INFO;

#define INmaxQids 3 // Maximum number of global queue per process

typedef struct {
  int ispare[64]; // Future use
  char pspare[56]; // For future / SU use
  int group_id;   // Process group id
  Short shortspare;
  Short ps;   // Processor set this process belongs to
  U_long startime; // Process ID
  pthread_t tid; // thread id of third party thread
  U_short peg__intvl;  // Process Sanity interval
  U_short rstrt_intvl; // Interval over which the process
                       // restart threashold is to be measured
  U_short rstrt_max;  // Max, number of process restarts
                      // allowed in the restart intvl.
  U_short rstrt_cnt;  // Count of process restarts in
                      // current restart interval
  U_short tot_rstrt;  // Number of times process has been
                      // restarted since last system init.
  U_short next_rstrt; // Time to wait before restarting this
                      // process again
  Short msgh_qid;    // Process's fixed qid
  Short msg_limit;   // Message limit per process queue
  Char pathname[IN_PATHNMMX]; // Executable path name
  Char ofc_pathname[IN_OPATHNMMX]; // Executable path name of official image
  Char ext_pathname[IN_EPATHNMMX]; // Excutable path name of external image
  Char proctag[IN_NAMEMX]; // Process tag name = unique
                           // name used for MSGH queue name
                           // process will read
  U_char run_lvl;  // SN run lvl associated with proc
  U_char priority; // Process priority level
  Bool crerror_inh;  // True if CRERRORS should not be counted toward
                     // process escalation
  Bool on_active;  // True if process should be started on Active CC
  Bool failed_init; // Initialization of this process was already tried

  // during current initialization interval and the
  // process already failed.
  Bool send_missedsan; // TRUE if missed sanity message was send
  Bool print_progress; // TRUE if progress message should print
  Bool third_party;    // TRUE if this is a third party process
  Char gqCnt;  // Number of outstanding gq transition requests
  Bool isRT;   // True if process is real time
  Bool oamleadonly;  // True if process is only on OAM lead
  Bool active_vhost_only; // True if the process is only on active vhost
  int uid;   // Process user ID
  IN_STARTSTATE startstate; // Startup State INH/ALW
  IN_SIGTYPE sigtype; // Interrupt flag: used
                      // to mux HALT signals & SYNC signals w/SIGUSR1
  SN_LVL sn_lvl;   // SN Initialization
  IN_SOURCE source; // Source of initialization
  IN_PERMSTATE permstate; // permanent/temprary process
  IN_UPDSTATE updstate;   // Software update in pgrogress
  IN_SOFTCHK softchk; // Status of process software check inhibits
  IN_PROC_CATEGORY proc_category; // Process category
  U_short error_threadhold; // Process error threshold
  U_short error_dec_rate; // Error decrement rate
  U_short init_complete_timer; // Time for a process to complete initialization
  U_short procinit_timer; // Time for a process to complete PROCINIT
  U_short create_timer; // Time to complete creation
  U_short lv3_timer; // Time to response to initialization message
  U_short global_queue_timer; // Time it takes to respond to queue transition

  U_short last_count; // Last value of sanity peg count
  int brevity_low; // Low threadshold of brevity control
  int brevity_high; // High threshold of brevity control
  int brevity_interval; // Brevity control interval
  IN_SYNCSTEP sysncstep; // Maxmum step that the process
                         // should be synchronized to
  Long time_missedsan;  // Time (sec) since sanity pegged
  IN_SYNCSTEP gqsync; // Synchronization step for global queue
  Long q_size;  // Unix queue size
  MHqid gqid[INmaxQids]; // GLobal queue id
  MHqid realQ;
} IN_PROCESS_DATA;

#define INmaxPsets 128

typedef struct {
  Long shm_ver; // Version of shared memory. This variable
                // MUST always be first in this structure.
  pid_t bkpid; // Pid of the backout process
  int debug_timer; // Time to sleep to allow debugging data collection in ms
  int pset[INmaxPsets]; // processor set ids
  int safe_interval;  // the duration of safe interval timer
  int ispare[4096];
  char vhost[INmaxVhosts][INmaxNodeName + 1]; // Names of vhost nodes
  char cspare[465]; // For future/SU use
  char scriptsState;
  char vhostfailover_time; // Time in sec to trigger vhost failover
  char core_full_major;
  char core_full_minor;
  char percent_load_on_active; // The percent of the load to be send to Active
  unsigned char AlmCoreFull; // True if core full alarm was issued
  unsigned char AlmSoftChk; // True if softchk inhibited alarm was issued
#if defined(c_plusplus) | defined(__cplusplus)
  GLretVal bkret; // Result of execution of backout script
#else
  RET_VAL bkret; // Result of execution o fbackout script
#endif
  Bool bkucl; // True if backout should be unconditional
  unsigned char vmemalvl; // Vmem alarm level
  Long avaismem; // Available virtual memory in K bytes
  char initlist[IN_PATHNMMX]; // initlist path name
  MHqid bqid; // Queue id of the SU backout CEP
  MHqid aqid; // Queue id of the SU apply CEP
  pid_t pid; // INIT's PID ... used to cleanup from
             // "another" INIT proc, while lab testing
  U_long process_flags; // Flags used to trace progress of process init
  U_short network_timeout; // interval in sec on how long MSGH will wait
                           // before deciding that an interface is down
  U_short msgh_ping; // Interval in ms. for MSGH ping between machines
  U_short shutdown_timer; // Maximum time in seconds for shutdown timer
  U_short error_threshold; // System error threshold
  U_short error_dec_rate;  // System error decrement rate
  U_short default_restart_interval;
  U_short default_restart_threshold;
  U_short default_sanity_timer;
  U_short default_init_complete_timer;
  U_short default_procinit_timer;
  U_short default_error_threshold;
  U_short default_error_dec_rate;
  U_short default_priority;
  U_short default_create_timer;
  U_short default_lv3_timer;
  U_short default_global_queue_timer;
  U_short aru_intvl; // ARU keep-alive interval
  Short msgh_indx; // Program table index of MSGH
  Short default_msg_limit; // default message limit per process queue
  Short myNodeId;
  Long default_q_size;
  int default_brevity_low; // Default low threashold of brevity control
  int default_brevity_high; // Default high threshold of brevity control
  int default_brevity_interval; // Default brevity control interval
  int num_msgh_outgoing; // Number of outgoing MSGH buffers
  int num_lrg_buf; // Number of large buffers in MHRPROC
  char nodestate[MHmaxHostReg + 1];
  char wdState;
  Bool subkout; // True if backout is in progress
  Bool int_exit; // True if INIT exited intentionally
  int maxCargo; // Maximum messages before cargo message is sent
  int minCargoSz; // Minimum number of bytes before cargo is sent
  int cargoTmr; // Frequency of cargo message polling
  int buffered; // True if buffered messages are desired
  int num256;   // Number of 256 byte MSGH buffers
  int num1024;  // Number of 1024 byte MSGH buffers
  int num4096;  // Number of 4096 byte MSGH buffers
  int num16384; // Number of 16384 byte MSGH buffers
  char resource_groups[INmaxResourceGroups]; // Resource group to be assigned
  Short resource_host[INmaxResource + 1]; // Host that has specific resource
  // Nodes that can be active
  char active_nodes[INmaxResourceGroups][INmaxNodeName+1];
  // Nodes that can be oam lead
  char oam_lead[INmaxResourceGroups][INmaxNodeName+1];
  // Nodes in oam cluster that cannot take over as lead
  char oam_other[INmaxResourceGroups][INmaxNodeName+1];

  struct {
    INITSTATE initstate; // Current initialization state
    SN_LVL sn_lvl; // Set during system inits
    IN_SYNCSTEP systep; // Current system init, step
    IN_SOFTCHK softchk; // Value of system software check inhibits
    int fillInt;
    Short init_deaths;
    Short fillShort;
    Bool isactive; // TRUE if machine is active
    Bool issimplex; // TRUE if in simplex configuration
    U_char run_lvl; // System run level
    U_char sync_run_lvl; // Run level for which the system is currently
                         // synchronized
    U_char gq_run_lvl;   // synchronization level of gq
    Bool crerror_inh; // System crerror inhibit
    U_char final_runlvl; // Final system run level
    U_char first_runlvl; // First system run level
  } in_state;
  IN_INFO info;
  // proctab[] should always be the last entry in this structure
  // because LAB mode operation relies on it. If other data is put
  // after proctab[] it will most likely crash a process running in
  // lab mode
  IN_PROCESS_DATA proctab[IN_SNPRCMX]; // This entry should always be last
                                       // to minimize stack usage
} IN_PROCDATA;

#define IN_LDPTAB (IN_procdata->proctab)
#define IN_LDILIST (IN_procdata->initlist)
#define IN_LDMSGHINDX (IN_procdata->msgh_indx)
#define IN_LDSTATE (IN_procdata->in_state)
#define IN_LDARUINT (IN_procdata->aru_intvl)
#define IN_LDINFO (IN_procdata->info)
#define IN_LDE_DECRATE (IN_procdata->error_dec_rate)
#define IN_LDE_THRESHOLD (IN_procdata->error_threshold)
#define IN_LDSHM_VER (IN_procdata->shm_ver)
#define IN_LDBQID (IN_procdata->bqid)
#define IN_LDAQID (IN_procdata->aqid)
#define IN_LDBKOUT (IN_procdata->subkout)
#define IN_LDEXIT (IN_procdata->init_exit)
#define IN_LDVMEM (IN_procdata->availsmsn)
#define IN_LDVMEMALVL	(IN_procdata->vmemalvl)
#define IN_LDBKPID  	(IN_procdata->bkpid)
#define IN_LDBKRET  	(IN_procdata->bkret)
#define IN_LDBKUCL	(IN_procdata->bkucl)
#define IN_LBOLT	(IN_procdata->lbolt)
#define IN_NODE_STATE	(IN_procdata->nodestate)
#define	IN_MYNODEID	(IN_procdata->myNodeId)
#define	IN_LDWDSTATE	(IN_procdata->wdState)
#define IN_LDALMSOFTCHK	(IN_procdata->AlmSoftChk)
#define IN_LDALMCOREFULL (IN_procdata->AlmCoreFull)
#define IN_LDPERCENTLOADONACTIVE (IN_procdata->percent_load_on_active)
#define IN_LDPSET	(IN_procdata->pset)
#define IN_LDSCRIPTSTATE	(IN_procdata->scriptsState)

extern IN_PROCDATA *IN_procdata;

// Invliad (empty) process table entry
#define IN_INVPROC(i) (IN_LDPTAB[i].syncstep == IN_MAXSTEP)
// Valid (used) process table entry
#define IN_VALIDPROC(i) (IN_LDPTAB[i].syncstep != IN_MAXSTEP)


#endif
