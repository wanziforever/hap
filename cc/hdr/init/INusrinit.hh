#ifndef _INUSRINIT_H
#define _INUSRINIT_H

// DESCRIPTION:
//  This file contains data structures and manifest constants
//  needed by permanent processes to interface with the Initialization
//  subsystem (INIT).

#include <stdio.h>  /* For printf() declaration */
#include <signal.h>

#include "hdr/GLtypes.h" /* SN-wide typedefs */
#include "hdr/GLreturns.h" // SN-wide return values
#include "hdr/GLmsgs.h"

#include "cc/hdr/init/INinit.hh" // INIT data structures & constants
#include "cc/hdr/init/INproctab.hh" // INIT data structures modified
// exclusively by INIT
#include "cc/hdr/init/INlib.hh" // Interfaces that can be used by any
                                // process in the system. not just proceses
                                // under INIT control.
#define LAB_MODE (int) 0x2020 // Run process in Lab Testing Mode
#define NORM_MODE (int) 0x0202 // Run processes in Normal Mode

#if defined(c_plusplus) | defined(__cplusplus)
extern "C" Short sysinit(int, char *[], SN_LVL, U_char);
extern "C" Short procinit(int, char *[], SN_LVL, U_char);
extern "C" Short cleanup(int, char *[], SN_LVL, U_char);
extern "C" Void process(int, char *[], SN_LVL, U_char);
extern "C" Void _inargchk(Short argc, Char *argv[],
                          SN_LVL *sn_lvl, U_char *run_lvl);
extern "C" Short _ingetindx(char *); // Get index into INIT Tables
extern "C" Void _indist(int); // user proc, signal disth'n
// user process init request
extern "C" Void _inreq(SN_LVL, GLretVal, const char *, IN_REQTYPE);
extern "C" Void _inassert(Short); // Peg and escalate on process errors
extern "C" Void _in_progress(const char*); // Report initialization progress
// check in during initialization
extern "C" Void _in_step(U_long, const char *);
// Brevity control parameters
extern "C" Void _in_brev_params(int*, int*, int*);
extern "C" Void _in_sanpeg(); // Sanity pegging function
extern "C" Short _in_msgh_qid(); // Value of MSGH qid
extern "C" Long _in_q_size_limit(); // Queue size limit
extern "C" Long _in_default_q_size_limit(); // Default queue size limit
extern "C" Short _in_msg_limit(); // Process msg limit
extern "C" Short _in_default_msg_limit(); // process default msg limit
// Process initialization complete
extern "C" GLretVal _in_init_complete(); 
// process reached critical functionality
extern "C" GLretVal _in_crit_complete();
extern "C" GLretVal _in_shutdown(); // Application shutdown
// Get the state of the specified node
extern "C" char _in_getNodeState(short);
// Get the state of watchdog interface
extern "C" char _in_getWdiState();
// the percentage of load to be directed to Active
extern "C" int _in_percentLoadOnActive();
// Number of outgoing MSGH buffers
extern "C" int _in_num_msgh_outgoing();
extern "C" int _in_num_lrg_buf(); // Number of large buffers in MHRPROC

// Following are MSG tuning parameters only
extern "C" int _in_maxCargo();
extern "C" int _in_minCargoSz();
extern "C" int _in_cargoTmr();
extern "C" int _in_buffered();
extern "C" int _in_num256();
extern "C" int _in_num1024();
extern "C" int _in_num4096();
extern "C" int _in_num16384();
extern "C" int _in_msgh_ping();
extern "C" U_short _in_network_timeout();
extern "C" int _in_shutdown_timer();

// Cluster management methods
extern "C" int _in_get_resource_group(short);
extern "C" short _in_get_oam_members(char[][INmaxNodeName+1], int);

#else

extern Short sysinit(); // User-defined system init, routine
extern Short clueanup(); // User-defined cleanup routine
extern Short procinit(); // User-defined process init, routine
extern Void process(); // User-defined main processing loop
extern Void _inargchk(); // Get Command Line Arguments
extern Short _ingetindx(); // Index into Initialization Tables
extern Void _indist(); // User Porcess signal distribution
extern Void _inreq(); // User Process Initialization Request
extern Void _inassert();
extern Void _in_process();
extern Void _in_step();
extern Void _in_brev_parms(); // Brevity control parameters
extern Void _in_sanpeg(); // Sanity pegging function
extern Short _in_msgh_qid(); // MSGH qid
extern Long _in_q_size_limit(); // Queue size limit
extern Long _in_default_q_size_limit(); // Default queue size limit
extern Short _in_msg_limit(); // Process msg limit
extern Short _in_default_msg_limit(); // Process default msg limit
extern RET_VAL _in_init_complete();
extern RET_VAL _in_crit_complete();
extern RET_VAL _in_shutdown(); // Application shutdown
extern char _in_getNodeState(); // Get the state of the specified node
extern char _in_getWdiState(); // Get the state of watchdog interface
// the percentage of load to be directed to Active
extern int _in_percentLoadOnActive();
// Number of outoing MSGH buffers
extern int _in_num_msgh_outgoing();
// Number of large buffers in MHRPROC
extern int _in_num_lrg_buf();

// following are MSGH tuning parameters only
extern int _in_maxCargo();
extern int _in_minCargoSz();
extern int _in_cargoTmr();
extern int _in_buffered();
extern int _in_num256();
extern int _in_num1024();
extern int _in_num4096();
extern int _in_num16384();
extern int _in_msgh_ping();
extern U_short _in_network_timeout();
extern int _in_shutdown_timer();

// Cluster management methods
extern int _in_get_resource_group(short);
extern short _in_get_oam_members(char[][INmaxNodeName+1], int);

#endif

// User Process Private Initialization Area -- "process local" data
typedef struct {
  Short indx; // Index into control tables
  Short mode; // Operating Mode -- DEBUG or NORM_MODE
  int argc;   // argc passed to main()
  char **argv; // argv passed to main()
} IN_LDATA;

#define IN_PINDX (IN_ldata->indx) // Index into process table

// User Process Initialization Request Macro.
// SN_LVL initlevel - requested recovery level, should be minimal
//         initialization likely to fix a problem.
//         ususally SN_LV0
// GLretVal error - error code associated with this request
// char * err_string - a short string explaining the error code
// IN_REQTYPE req_type - depending on the request type (IN_ABORT or
//         IN_EXIT) either abort() or exit() will be called
//         by _inreq().
//
// NOTE: A caller of this function must be prepared that it will return.
// Requests for initializations are ignored if system or process software
// checks are inhitbited.
#define INITREQ(initlevel, error, err_string, req_type) { \
  _inreq(initlevel, error, err_string, req_type);         \
  }

// Check if software checks are inhibited, both for system and process
#define IN_SOFTCHK_INHIBITED()                  \
  (IN_LDSTATE.softchk == IN_INHSOFTCHK ||       \
   IN_LDPTAB[IN_PINDX].softchk == IN_INHSOFTCHK)

// Critical Exit State Macros
#define INSETCRIT()
#define INRSTCRIT()

// Check if running in simplex configuration. Always true for simplex
// configurations
#define INISSIMPLEX() (IN_LDSTATE.issimplex == TRUE)

// Sanity Pegging function for User Processes
#define IN_SANPEG() _in_sanpeg()

// Return the value of the msgh_qid. This primitive returns -1 if qid
// is not initialized.
#define IN_MSGH_QID() _in_msgh_qid()

// Return the queue size limit for this process
#define IN_Q_SIZE_LIMIT() _in_q_size_limit()
#define IN_DEFAULT_Q_SIZE_LIMIT() _in_default_q_size_limit()

// return message size limits
#define IN_MSG_LIMIT() _in_msg_limit()
#define IN_DEFAULT_MSG_LIMIT() _in_default_msg_limit()

// return brevity control values
#define IN_BREV_PARMS(low, high, interval) _in_brev_parms(low, high, interval)

// Marco to check whether the process has just begun a new initialization
// or not -- if the process is not in the "STEADY", "CLEANUP", "PROCESS",
// "CPREADY" or
// "PROCINIT" states then it should not be receiving messages (this
// macro is used by the "MHinfoExt.receive()" method):

#define IN_INITSEQ ( ((IN_SDPTAB[IN_PINDX].procstep!=IN_STEADY)  &&		\
	   (IN_SDPTAB[IN_PINDX].procstep!=IN_PROCESS) &&		\
	   (IN_SDPTAB[IN_PINDX].procstep!=IN_CPREADY) &&		\
	   (IN_SDPTAB[IN_PINDX].procstep!=IN_PROCINIT) &&		\
	   (IN_SDPTAB[IN_PINDX].procstep!=IN_CLEANUP) &&		\
	   (IN_SDPTAB[IN_PINDX].procstep!=IN_SU) &&			\
	   (IN_SDPTAB[IN_PINDX].procstep!=IN_BSU) ) ?TRUE:FALSE)

// macro to check if the process in in steady state
#define IN_INSTEADY() (IN_SDPTAB[IN_PINDX].procstep == IN_STEADY)
// Report on completion major functionality during initialization
// This results in output in prmlog and OMlog.
#define IN_PROGRESS(step_desc) {                \
  _in_progress(step_desc); \
  }

// Provide feedback on progress of initialization to INIT
// INIT expects to see progress_mark to advance during initialization
// with a design goal of a step every 10 seconds.
// If progress mark is not updated in about 100 sec., progress initialization
// will fail. Progress_mark should be as specific as possible instead of
// being just a number that is merely incremented. It is best if progress
// marks are assigned specific numbers for each piece of initialization
// work accomplished.
// step_desc is currently not used, however if display pages are supported
// in future. the step_desc string may be displayed.
// ARGUMENTS:
// U_long progress_mark;
// char * step_desc;
#define IN_STEP(progress_mark, step_desc) {     \
    _in_step((progress_mark), (step_desc));     \
    }

// Transition the calling progress from CPREADY to STEADY state
// If the process is not in CPREADY state, transition request will
// result in failing return code and transition will not occur.
#define IN_INIT_COMPLETE() {                    \
  _in_init_complete();                          \
  }

// Transition the calling process from PROCESS to CPREADY state
// If the process is not in CPREADY state, transition request will
// result in failing return code and transition will not occur.
#define IN_CRIT_COMPLETE() {                    \
  _in_crit_complete();                          \
  }

// Get the state of the node identified by the node host id
// If the name is unkown, return 0 for the node state
#define IN_GETNODESTATE(hostid) _in_getNodeState(hostid)

// Get the percentage of the load that should be directed to Active side
#define IN_PERCENT_LOAD_ON_ACTIVE() _in_percentLoadOnActive()

// Get the number of outgoing MSGH buffers
#define IN_NUM_MSGH_OUTGOING() _in_num_msgh_outgoing()

// Get the number of large message buffers in MHRPROC
#define IN_NUM_LRG_BUF() _in_num_lrg_buf()

// Get state of watchdog interface on the local node
#define IN_GETWDISTATE() _in_getWdiState()

// MSGH parameters
#define IN_MAX_CARGO()  _in_maxCargo()
#define IN_MIN_CARGOSZ() _in_minCargoSz()
#define IN_CARGO_TMR()  _in_cargoTmr()
#define IN_BUFFERED()   _in_buffered()
#define IN_NUM256() _in_num256()
#define IN_NUM1024() _in_num1024()
#define IN_NUM4096() _in_num4096()
#define IN_NUM16384() _in_num16384()
#define IN_MSGH_PING() _in_msgh_ping()
#define IN_SHUTDOWN_TIMER() _in_shutdown_timer()
#define IN_NETWORK_TIMEOUT() _in_network_timeout()

// Cluster information
// Return members capable of being OA&M lead
#define INoamMembersLead 2
#define INGETOAMMEMBERS(names, type) _in_get_oam_members(names, type)
#define INGETRESOURCEGROUP(hostid) _in_get_resources_group(hostid)

// This function will cause the shutdown of the RSP application
// It should only be used by maintance entities, mostly FT
#define IN_IS_LAB() (IN_ldata->mode == LAB_MODE)

extern IN_LDATA *IN_ldata;

// Stubs for no longer supported IN_IGNPDEATH() macros to be removed as
// soon as all users stopped using them
#define IN_IGNPDEATH()
#define IN_CLRIGNPDEATH()
#define IN_RPTIGNPDEATH()
#endif
