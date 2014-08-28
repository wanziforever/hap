#ifndef _ININIT_H
#define _ININIT_H

// DESCRIPTION:
//  This file contains the data structures and manifest contants needed
//  by all processes which use or are a part of the Initialization
//  subsystem (INIT). Note that the structure "IN_SDATA" only contains
//  INIT information which can be modified by processes started by
//  INIT. Data which si ONLY modified by INIT is include in the
//  "IN_PROCDATA" structure defined in "cc/hdr/init/INproctab.hh"
//
// NOTES:
//  IF ANY CHANGES ARE MADE TO THIS HEADER THAT AFFECT MEMORY
//  LAYOUT/SIZE/USAGE, IN_SHM_VER #DEFINE MUST BE UPDATED IN
//  cc/hdr/init/INproctab.hh

#include <sys/types.h> // UNIX-defiend typedefs
//#include <synch.h>
#include <pthread.h>
#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "cc/hdr/init/INshmkey.hh"
//#include "cc/hdr/cr/CRciMem.hh"

#define IN_SNPRCMX 1024 // Max Permanent Processes
#define IN_NUMSEMIDS 2 // Max number of semaphores allocatable
                       // per process

// IN_MSGHQNM is the MSGH queue name INIT will use to receive MSGH
// messages. INIT will not start any process which has the same
// queue name as IN_MSGHQNM:
#define IN_MSGHQNM "INIT"

// IN)MSGH_ENV is the name of the environment variable containing MSGH
// name of the process.
#define IN_MSGH_ENV "_INMSGHNAME"

// The following are priority of action indicators
// These trace-like falgs have been left for possible
// future use
#define IN_CRIT		0x10
#define IN_MAJ		0x20
#define IN_MIN		0x40
#define IN_AUTO		0x80
#define IN_NORM		0x100

// The following defines are output destinations for the inout macro
#define IN_REPT		0x01
#define IN_LOG		0x02
#define IN_BTTY		0x04

// The following defines are output message flags which are used
// to determine whether or not to output a INOUT message.
// Again, this has been left here for now in case
// we decide we want it.
//
//       NAME      BIT FLAG      SYMBOL  DESCRIPTION
//      --------  -----------   -------  ------------------
#define IN_MSGHTR	0x00000001L	//  msgh	INIT msg. processing

#define IN_SSEQTR	0x00000002L	//  seq		General sequence
#define IN_BSEQTR	0x00000004L	//  bseq	BOOTSEQ activity     
#define IN_RSEQTR	0x00000008L	//  rseq	RESETSEQ activity    

#define IN_SNCRTR	0x00000020L	//  sncr	SN process creation  
#define IN_AUDTR	0x00000040L	// audits	Audit activity       
#define IN_RDINTR	0x00000080L	//  rdin	RDINLS activity      
#define IN_PSYNCTR 	0x00000100L	// psync	SN synchronization 

#define IN_SANITR	0x00004000L	//  sani	Sanity activity    
#define IN_TESTTR	0x00008000L	//  test	Test activity	     
#define IN_IREQTR	0x00010000L	//  ireq	Init requests	     
#define IN_RSTRTR	0x00020000L	//  rstrt	Restart activity   
#define IN_TIMER	0x00040000L	//   timers	Timer activity   
#define IN_CEPTR	0x00080000L	//   cep	CEP interface	     
#define IN_SIMTR	0x00100000L	//   sim	SIM interface	     
#define IN_WDTR		0x00200000L	//   wd		WD interface

#define IN_SYSITR	0x00400000L	//  sysi	SYSINIT activity
#define IN_PROCITR	0x00800000L	//  proci	PROCINIT activity
#define IN_CLNITR	0x01000000L	//  clni	CLEANUP activity    

//		ONE EMPTY BIT POSITION HERE FOR FUTURE EXPANSION
#define IN_UCL0TR	0x04000000L	//  ucl0	User defined class 0 
#define IN_UCL1TR	0x08000000L	//  ucl1	User defined class 1 

#define IN_ALWAYSTR	0x10000000L	//  -----	ALWAYS; INFORM detail
#define IN_DEBUG	0x20000000L	//  -----	IN_DEBUG major type 
#define INFORM		0x40000000L	//  -----	INFORM major type   
#define IN_ERROR	0x80000000L	//  -----	IN_ERROR major type

// The "SN_LVL" type is used to determine the SNlevel
// of initialization currently taking place
typedef enum {
  SN_INV, // Uninitialized
  SN_NOINIT, // No SN Initialization
  SN_LV0, // Single Process - don't reload provisioning info
  SN_LV1, // Single Process - reload prvisioning info
  SN_LV2, // System Reset - don't reload provisioning info
  SN_LV3, // System Reset - reload provisioning info
  SN_LV4, // Bootstrap - re-initialize all data
  SN_LV5, // System reboot or switchover
  IN_MAXSNLVL, // Used for range checking, new SN initialization
               // levels should be added BEFORE this one
  SN_LVL_BOGUS_SUNPRO
} SN_LVL;

// This typedef is used as an initialization request type

typedef enum {
  IN_EXIT,  // exit() call should be used to terminate this process
  IN_ABORT, // abort() call should be used to terminate this process
  IN_EXPECTED, // initialization request was intentional, not necessarily
  // signifying process insanity, but taken to recover from
  // some expected operational state that only process exit
  // can clear, i.e. hung async ports. This request type
  // causes software check inhibits to be ignored and results
  // in non-alarmed initialization request messages. It should
  // be used sparingly
  IN_REQTYPE_BOGUS_SUNPROC
} IN_REQTYPE;;

// Element states
#define S_ACT     'A'
#define S_LEADACT 'L'
#define S_STBY    'S'
#define S_UNAV    'U'
#define S_OFFLINE 'O'
#define S_INIT    'I'

#define IN_OFFLINE_FILE "/opt/config/status/.psp-init.halt"

// This typedef defines the current running state of a permanent process.

typedef enum {
  IN_INVSTATE,  // Unused, Invalid, Uninitialized Entry
  IN_NOEXIST,   // Valid Entry, uninitialized
  IN_CREATING,  // Process Being Created, Awaiting Ack from PMGR
  IN_HALTED,    // Process is suspended from doing any work
  IN_RUNNING,   // Process is executing
  I_DEAD,       // Process has died
  IN_MAXSTATE,  // Used for range checking...new states should
  // be inserted BEFORE this one.
  IN_PROCSTATE_BOGUS_SUNPRO
} IN_PROCSTATE;

// Defintions for the virtual hosts state
typedef enum {
  INnotpaired,
  INstandby,
  INactive
} INvstate;

// This typedef is used to define a permanent processes current run
// step. This is used to conjunction with the process states defined
// above to cycle permanent processes through coordinated initialization
// steps.
// NOTE: Order of the steps is important, all the normal initialization
// steps should be prior to cleanup steps, i.e. cleanup steps should
// always be last.
typedef enum {
  INV_STEP,     // unitialized
  IN_BGQ,       // Begin handling of global queue transition
  IN_GQ,        // Handling global queue transition
  IN_EGQ,       // Finished handling global queue transition
  IN_BUS,       // Process is begining software update
  IN_SU,        // Process is in teh middle of software update
  IN_ESU,       // Process is ending software update
  IN_READY,     // Process Started, Rready to Receive signals
  IN_BHALT,     // System is in the STOP Phose
  IN_HALT,      // Process is beginning IN_HALT Phase
  IN_EHALT,     // Process is ending HALT Phase
  IN_BSTARTUP,  // Process is beginning the STARTUP Phase
  IN_STARTUP,   // Process is IN_STARTUP Phase
  IN_ESTARTUP,  // Process is ending STARTUP Phase
  IN_BSYSINIT,  // Process is beginning the SYSINIT Phase
  IN_SYSINIT,   // Process is in IN_SYSINIT Phase
  IN_ESYSINIT,  // Process ending SYSINIT Phase
  IN_BPROCINIT, // Process is beginning the PROCINIT Phase
  IN_PROCINIT,  // Process is in IN_PROCINIT Phase
  IN_EPROCINIT, // Process ending PROCINIT Phase
  IN_BPROCESS,  // Process is beginning the PROCESS Phase
  IN_PROCESS,   // Process is in PROCESS Phase
  IN_EPROCESS,  // Process ending PROCESS Phase
  IN_CPREADY,   // Process is in CPREADY Phase
  IN_ECPREADY,  // Process is ending CPREADY Phase
  IN_BSTEADY,   // System is in the STEADY State Phase
  IN_STEADY,    // Process is in IN_STEADY Phase
  IN_BCLEANUP,  // System is in the CLEANUP Phase
  IN_CLEANUP,   // System is in the CLEANUP Phase
  IN_ECLEANUP,  // Process Ending CLEANUP Phase
  IN_MAXSTEP,   // Used for range checking , new process step
  // should be added BEFORE this one
  IN_SYNCSTEP_BOGUS_SUNPRO
} IN_SYNCSTEP;

typedef struct {
  int semids[IN_NUMSEMIDS];  // Allocated semaphare IDs
#if defined(c_plusplus) | defined(__cplusplus)
  GLretVal ecode; // User-defined err code -- passed in INITREQ macro
#else
  RET_VAL ecode;  // User-defined err code -- passed in INITREQ macro
#endif
  U_short count;  // Sanity Peg Count

  Short ret;  // Return Value from Client routine
  Short error_count;  // number of process accumulated errors
  Short progress_check; // Time on process progress
  unsigned char alvl;  //Uncleared alarm Level
  char fillchar;
  int fillInt;
  IN_SYNCSTEP procstep; // Process Synchronization Step
  IN_PROCSTATE procstate; // Process State
  SN_LVL ireq_lvl;  // INIT level requested via
  // INITREQ macro
  U_long progress_mark; // Initialization progress mark
} IN_PTAB;

#define INmaxSegs 1000

struct INshmemInfo {
  int m_pIndex;
  IN_SHM_KEY m_pkey;
  int m_shmid;
  Bool m_rel;
  Bool m_fillchar;
  Short m_fillShort;
};

typedef struct {
  Short  error_count; // Number of system accumulated errors
  Short  fill; // 64 bit alignemnt fill
  int shm_lock_cnt;
  //struct CRciMem ci_data; // SYSTAT critical indicator data
  pthread_mutex_t shm_lock;
  struct INshmemInfo shmem[INmaxSegs];
  IN_PTAB in_ptab[IN_SNPRCMX];
} IN_SDATA;

// Macro used in C source to reference tables within IN_SDATA
#define IN_SDPTAB (IN_sdata->in_ptab)
#define IN_SDERR_COUNT (IN_sdata->error_count)
#define IN_SDCIDATA (IN_sdata->ci_data)
#define IN_SDSHMLOCK (IN_sdata->shm_lock)
#define IN_SDSHMLOCKCNT (IN_sdata->shm_lock_cnt)
#define IN_SDSHMDATA (IN_sdata->shmem)

extern IN_SDATA *IN_sdata;

#if defined(c_plusplus) | defined(__cplusplus)
extern const char *IN_procstnm[];
extern const char *IN_sqstepnm[];
extern const char *IN_snlvlnm[];
extern const char *IN_prctypnm[];
extern "C" void INsigintIgnore(int);
#else
extern char *IN_procstnm[];
extern char *IN_sqstepnm[];
extern char *IN_snlvlnm[];
extern char *IN_proctypnm[];
#endif

#endif
