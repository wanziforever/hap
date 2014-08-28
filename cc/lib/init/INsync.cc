//
// DESCRIPTION:
//  This file contains the client process synchronization
//  library routines that functions as a vehicle for
//  carrying out permanet process initialization synchronization
//
//  FUNCTIONS:
//   _insync()
//   INwaitProcinit()
//   INwaitProcess()
//   _in_init_complete()
//   _in_crit_complete()
//   _in_sanpeg()
//   _in_brev_parms()
//   _in_msgh_qid()
//   _in_q_size_limit()
//   _in_default_q_size_limit()
//   _in_msg_limit()
//   _in_default_msg_limit()
//   _in_get_oam_members()
//   _in_get_resource_group()

#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "cc/hdr/init/INsbt.hh"
#include "cc/hdr/init/INreturns.hh"
#include "cc/hdr/init/INlibinit.hh"

const char* INcompleteStr = "INIT COMPLETE";

// Name:
//   _insync()
//
// Description:
//  This routing lock steps the various initialization
//  client processes through a series of initialization
//  routines that implement the system bootstrap, system
//  reset, and process restart initialization strategies.
//
// Input:
//
//  Shared Memory:
//   IN_SDPTAB[]
//   IN_LDPTAB[]
//
//  Private Memory:
//   IN_ldata
//
// Returns:
//  None.
//
// Calls:
//  exit() - voluntary termination of this process
//
//  client-supplied cleanup() - cleanup process specific
//  and/or system-specific resources
//
//  client-supplied sysinit() - initialize system-specific
//  resources and/or structures.
//
//  client-supplied procinit() - initialize
//  process-specific resources and/or data structures
//
// Called By:
//  This routine is invoked by _indist() when
//  IN_LDPTAB[IN_PINDX].sigtype is set to SIGSYNC.
//
// Side Effects:
//  Various internal INIT-supplied initialization routines
//  will be invoked as well as corresponding client-
//  supplied initialization routines.
//
//  Certain error legs will result in the voluntary
//  termination of this process.
//
//  IN_SDPTAB[], IN_LDPTAB[] shared memory tables will be
//  updated to relflect the new global synchronization state
//  information.
//
//  IN_ldata private initialization memory will be updated to
//  reflect new process-specific state information.

Void _insync() {
  struct itimerval itmr_val;
  IN_SDATA *tmp_sdata;
  IN_PROCDATA *tmp_procdata;

  printf("Insync.c:: _insync(): routine entry:/n/tprocess = %s"
         "\n\tprocstep = %s\n", IN_LDPTAB[IN_PINDX].proctag,
         IN_SQSTEPNM(IN_SDPTAB[IN_PINDX].procstep));

  if (IN_sdata != 0 && IN_procdata != 0) {
    tmp_sdata = IN_sdata;
    tmp_procdata = IN_procdata;
  } else {
    printf("REPT INIT %s REQUESTED INIT DUE TO BAD SHARED MEMORY POINTERS",
           IN_LDPTAB[IN_PINDX].proctag);
    exit(0);
  }

  Bool doabort = FALSE;
#ifndef NOMAIN
  Bool done = FALSE;
  while (!done) {
    // Perform specific synchronization step processing
    switch ((int) IN_SDPTAB[IN_PINDX].procstep) {
    case IN_BSTARTUP:
      IN_SDPTAB[IN_PINDX].procstep = IN_STARTUP;
      // Can't think of a use for this internal routine but
      // i'll leave it here for now.
      // _instartup();
      IN_SDPTAB[IN_PINDX].procstep = IN_ESTARTUP;
      break;
    case IN_BCLEANUP:
      IN_SDPTAB[IN_PINDX].procstep = IN_CLEANUP;
      // Generate a stack trace if this cleanup is triggered by
      // INIT initialization request caused by sanity timer expiring
      if (IN_SDPTAB[IN_PINDX].ireq_lvl == SN_LV0 ||
          IN_SDPTAB[IN_PINDX].ireq_lvl == SN_LV1) {
        switch (IN_SDPTAB[IN_PINDX].ecode) {
        case IN_SANTMR_EXPIRED:
        case IN_SYNCTMR_EXPIRED:
          doabort = TRUE;
          break;
        default:
          break;
        }
      }

      IN_SDPTAB[IN_PINDX].ret = cleanup(IN_ldata->argc, IN_ldata->argv,
                                        IN_LDPTAB[IN_PINDX].sn_lvl,
                                        IN_LDSTATE.run_lvl);

      IN_SDPTAB[IN_PINDX].procstep = IN_ECLEANUP;
      // Always exit the process if it has been sequenecd through cleanup
      if (doabort == TRUE) {
        abort();
      } else {
        exit(0);
      }
      break;
    case IN_BSYSINIT:
      IN_SDPTAB[IN_PINDX].procstep = IN_SYSINIT;
      // Enable "SIGCLD" so that the proc. doesn't unkowningly
      // create zombie children (e.g via "system()"):
      signal(SIGCLD, SIG_DFL);
      IN_SDPTAB[IN_PINDX].ret = sysinit(IN_ldata->argc, IN_ldata->argv,
                                        IN_LDPTAB[IN_PINDX].sn_lvl,
                                        IN_LDSTATE.run_lvl);

      // Check the shared memory pointer just in case that .bss
      // section was zeroed in cleanup!!!
      if (IN_sdata != tmp_sdata || IN_procdata != tmp_procdata) {
        printf("REPT INIT %s REQUESTED INIT DUE TO BAD SHARED MEMORY POINTERS",
               IN_LDPTAB[IN_PINDX].proctag);
        exit(0);
      }
      // Check a return code and request initialization if a failure.
      // Process can also request an initialization directly if a
      // level bigher than SN_LV0 is wanted. INIT library will always
      // request SN_LV0.
      if (IN_SDPTAB[IN_PINDX].ret != GLsuccess) {
        INITREQ(SN_LV0, IN_SDPTAB[IN_PINDX].ret, "FAILED SYSINIT", IN_EXIT);
      }

      IN_SDPTAB[IN_PINDX].procstate = IN_HALTED;
      IN_SDPTAB[IN_PINDX].procstep = IN_ESYSINIT;
      break;
    case IN_BPROCINIT:
      IN_SDPTAB[IN_PINDX].procstep = IN_PROCINIT;
      // Enable "SIGCLD" so that the proc, doesn't unkowingly
      // create zombie children (e.g. via "system()")
      signal(SIGCLD, SIG_DFL);
      IN_SDPTAB[IN_PINDX].ret = procinit(IN_ldata->argc, IN_ldata->argv,
                                         IN_LDPTAB[IN_PINDX].sn_lvl,
                                         IN_LDSTATE.run_lvl);
      // Check global shared memory data pointer just in case the .bss
      // section was corrupted in procinit!!!

      if (IN_sdata != tmp_sdata || IN_procdata != tmp_procdata) {
        printf("REPT INIT %s REQUESTED INIT DUE TO BAD SHARED MEMORY POINTERS",
               IN_LDPTAB[IN_PINDX].proctag);
        exit(0);
      }
      // Check a return code and request initialization if a failure.
      // Process can also request an initialization directly if a
      // level higher than SN_LV0 is wanted. INIT library will always
      // request SN_LV0
      if (IN_SDPTAB[IN_PINDX].ret != GLsuccess) {
        INITREQ(SN_LV0, IN_SDPTAB[IN_PINDX].ret, "FAILED PROCINIT", IN_EXIT);
      }

      if (IN_LDPTAB[IN_PINDX].print_progress) {
        IN_PROGRESS("PROCINIT COMPLETE");
      }
      IN_SDPTAB[IN_PINDX].procstate = IN_HALTED;
      IN_SDPTAB[IN_PINDX].procstep = IN_EPROCINIT;
      break;
    case IN_BPROCESS:
      IN_SDPTAB[IN_PINDX].procstep = IN_PROCESS;
      done = TRUE;
      break;
    default:
      // IN_ERROR
      printf("INsync.c: _insync(): invalid step encoutered -> exiting:\n\t"
             "procstep = %d\n\tprocess = %s", (IN_SDPTAB[IN_PINDX].procstep,
                                               IN_LDPTAB[IN_PINDX].proctag));
      break;
    }

    // Disable any timers which may be used by a client process so
    // that they don't interfere with the initialization sequence
    itmr_val.it_value.tv_sec = 0;
    itmr_val.it_value.tv_usec = 0;
    itmr_val.it_interval.tv_sec = 0;
    itmr_val.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &itmr_val, NULL);
    setitimer(ITIMER_VIRTUAL, &itmr_val, NULL);
    setitimer(ITIMER_PROF, &itmr_val, NULL);

    (Void) alarm((U_short) 0);

    sigset(SIGINT, INsigintIgnore);
    signal(SIGPROF, SIG_IGN);
    signal(SIGVTALRM, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    sighold(SIGALRM);
    signal(SIGCLD, SIG_IGN);
    signal(SIGUSR1, SIG_IGN);
    struct timespec tsleep;
    tsleep.tv_sec = 0;
    tsleep.tv_nsec = 50000000;
    while (IN_SDPTAB[IN_PINDX].procstate == IN_HALTED) {
      nanosleep(&tsleep, NULL);
    }
  }
#endif
}

// Description:
//  This routine lock steps the various initialization
//  client processes through a series of initialization
//  until procinit is hit, the returns.
//
// Inputs:
//  Shared Memory:
//   IN_SDPTAB[]
//   IN_LDPTAB[]
//
//  Private Memory:
//   IN_ldata
//
// Returns:
//  TRUE if successfully in Procinit, false if a failure is found
//
// Calls:
//  exit() - voluntary termination of this process
//
// Called by:
//  User process to wait for proper sequencing.
//
// Side Effects:
//   Various internal INIT-supplied initializaiton routines
//   will be invoked
//
//   Certain error legs will result in the voluntary
//   termination of this process.
//
//   IN_SDPTAB[], IN_LDPTAB[] share memory tables will be
//   updated to reflect the new global synchronization state
//   information.
//
//   IN_ldata private initialization memory will be updated to
//   reflect new process-specific state information.
//

#ifdef NOMAIN

Bool INwaitProcinit() {
  struct itimerval itmr_val;
  IN_SDATA *tmp_sdata;
  IN_PROCDATA *tmp_procdata;
  struct timespec tsleep;

  printf("INsync.c: INwaitProcinit(): routine entry:\n\tprocess=%s\n\t"
         "procstep=%s\n",IN_LDPTAB[IN_PINDX].proctag,
         IN_SQSTEPNM(IN_SDPTAB[IN_PINDX].procstep));
  // If we're in lab mode, return right away
  if (IN_IS_LAB()) {
    IN_SDPTAB[IN_PINDX].procstep = IN_PROCINIT;
    return(TRUE);
  }

  if (IN_sdata != 0 && IN_procdata != 0) {
    tmp_sdata = IN_sdata;
    tmp_procdata = IN_procdata;
  } else {
    printf("rept init %s requested init due to bad shared memory"
           "pointers", IN_LDPTAB[IN_PINDX].proctag);
    exit(0);
  }

  // From INmain

  // Initialize the pointer to the assert function that CRERROR and
  // CRASSERT will call
  CRsetAssertFn(_inassert);

  IN_ldata->argc = 0;
  IN_ldata->argv = 0;

  IN_SDPTAB[IN_PINDX].ireq_lvl = SN_NOINIT;
  tsleep.tv_sec = 0;
  tsleep.tv_nsec = 50000000;
  IN_SDPTAB[IN_PINDX].procstate = IN_HALTED;
  IN_SDPTAB[IN_PINDX].procstep = IN_EHALT;
  while (IN_SDPTAB[IN_PINDX].procstate == IN_HALTED) {
    nanosleep(&tsleep, NULL);
  }

  // end of code from INmain

  Bool doabort = FALSE;
  Bool done = FALSE;

  while (!done) {
    // Perform specific synchronization step processing
    switch((int) IN_SDPTAB[IN_PINDX].procstep) {
    case IN_BSTARUP:
      IN_SDPTAB[IN_PINDX].procstep = IN_STARTUP;
      // Can't think of a use for this internal routine but
      // i'll have it here for now

      IN_SDPTAB[IN_PINDX].procstep = IN_ESTARTUP;
      break;
    case IN_BCLEANUP:
      IN_SDPTAB[IN_PINDX].procstep = IN_CLEANUP;
      // Generate a stack trace if this cleanup is triggered by
      // INIT initialization request caused by sanity timer expiring
      if (IN_SDPTAB[IN_PINDX].ireq_lvl == SN_LV0 ||
          IN_SDPTAB[IN_PINDX].ireq_lvl == SN_LV1) {
        switch (IN_SDPTAB[IN_PINDX].ecode) {
        case IN_SANTMR_EXPIRED:
        case IN_SYNCTMR_EXPIRED:
          doabort = TRUE;
          break;
        default:
          break;
        }
      }

      // Always exit the process if it has been sequenced through cleanup
      if (doabort == TRUE) {
        abort();
      } else {
        exit(0);
      }
      break;
    case IN_BSYSINIT:
      IN_SDPTAB[IN_PINDX].procstate = IN_HALTED;
      IN_SDPTAB[IN_PINDX].procstep = IN_ESYSINIT;
      break;
    case IN_BPROCINIT:
      IN_SDPTAB[IN_PINDX].procstep = IN_PROCINIT;
      // Enable "SIGCLD" so that the proc. doesn't unkowningly
      // create zombie children (e.g. via "system()")
      signal(SIGCLD, SIG_DFL);
      return(TRUE);
    case IN_BPROCESS:
      printf("INsync.c: INwaitProcinit(): invalid step (PROCESS) "
             "encountered -> exiting:.\n\tprocstep = %d\n\tprocess = %s",
             (int)IN_SDPTAB[IN_PINDX].procstep, IN_LDPTAB[IN_PINDX].proctag);
      return(FALSE);
    default:
      printf("INsync.c: _insync(): invalid step encoutered -> exiting:"
             "\n\tprocstep = %d\n\tprocess = %s",
             (int)IN_SDPTAB[IN_PINDX].procstep, IN_LDPTAB[IN_PIDNX].proctag);
      return(FALSE);
    }

    // Disable any timers which may be used by a client process so
    // that they don't interfere with the initialization sequence
    itmr_val.it_value.tv_sec = 0;
    itmr_val.it_value.tv_usec = 0;
    itmr_val.it_interval.tv_sec = 0;
    itmr_val.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &itmr_val, NULL);
    setitimer(ITIMER_VIRTUAL, %itmr_val, NULL);
    setitimer(ITIMER_PROF, &itmr_val, NULL);

    (Void) alarm((U_short) 0);

    sigset(SIGINIT, INsigintIgnore);
    signal(SIGPROF, SIG_IGN);
    signal(SIGVTALRM, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    signal(SIGALRM);
    signal(SIGCLD, SIG_IGN);
    signal(SIGUSR1, SIG_IGN);
    tsleep.tv_sec = 0;
    tsleep.tv_nsec = 50000000;
    while (IN_SDPTAB[IN_PINDX].procstate == IN_HALTED) {
      nanosleep(&tsleep, NULL);
    }
  }
}

#endif

// Name:
//   INwaitProcess()
//
// Description:
//   This is routine lock steps the various initialization
//   client processes through a series of initialization
//   between procinit and process, then returns.
//
// Inputs:
//
//   Shared Memory:
//     IN_SDPTAB[]
//     IN_LDPTAB[]
//
//   Private memory:
//     IN_ldata
//
// Returns:
//    TRUE if successfully in Process, false if a failure is found.
//
// Calls:
//   exit() - voluntary termination of this process
//
// Called By:
//   User process to wait for proper sequencing
//
// Side Effects:
//   Various internal INIT-supplied initialization routines
//   will be invoked
//
//   Certain error legs will result in the voluntary
//   termination of this process.
//
//   IN_SDPTAB[], IN_LDPTAB[] shared memory tables will be
//   updated to reflect the new global synchronization state
//   information.
//
//   IN_ldata private initialization memory will be updated to
//   reflect new process-specific state information.

#ifdef NOMAIN
Bool INwaitProcess() {
  struct itimerval itmr_val;
  IN_SDATA *tmp_sdata;
  IN_PROCDATA *tmp_procdata;
  struct timespec tsleep;

  printf("INsyc.c: INwaitProcess(): routine entry:\n\tprocess = %s"
         "\n\tprocstep = %s\n", IN_LDPTAB[IN_PIDNX].proctag,
         IN_SQSTEPNM(IN_SDPTAB[IN_PINDX].procstep));
  // If we're in lab mode, return right away
  if (IN_IS_LAB()) {
    IN_SDPTAB[IN_PINDX].procstep = IN_PROCESS;
    return(TRUE);
  }

  if (IN_sdata != 0 && IN_procdata != 0) {
    tmp_sdata = IN_sdata;
    tmp_procdata = IN_procdata;
  } else {
    printf("rept init %s requested init due to bad shared memory pointers",
           IN_LDPTAB[IN_PINDX].proctag);
    exit(0);
  }

  if (IN_LDPTAB[IN_PINDX].print_progress) {
    IN_PROGRESS("PROCINIT COMPLETE");
  }

  IN_SDPTAB[IN_PINDX].procstate = IN_HALTED;
  IN_SDPTAB[IN_PINDX].procstep = IN_EPROCINIT;
  while (IN_SDPTAB[IN_PINDX].procstate == IN_HALTED) {
    nanosleep(&tsleep, NULL);
  }

  Bool doabort = FALSE;
  Bool done = FALSE;
  while (!done) {
    // Perform specific synchronization step processing
    switch ((int) IN_SDPTAB[IN_PINDX].procstep) {
    case IN_BCLEANUP:
      IN_SDPTAB[IN_PINDX].procstep = IN_CLEANUP;
      // Generate a stack trace if this cleanup is triggered by
      // INIT initialization request cased by sanity timer expireing
      if (IN_SDPTAB[IN_PINDXf.ireq_lvl == SN_LV0 || IN_SDPTAB[IN_PINDX].ireq_lvl == SN_LV1]) {
        switch (IN_SDPTAB[IN_PINDX].ecode) {
        case IN_SANTMR_EXPIRED:
        case IN_SYNCTMR_EXPIRED:
          doabort = TRUE;
          break;
        default:
          break;
        }
      }

      // Always exit the process if it has been sequenced through cleanup
      if (doabort == TRUE) {
        abort();
      } else {
        exit(0);
      }
      break;

    case IN_BPROCESS:
      IN_SDPTAB[IN_PINDX].procstep = IN_PROCESS;
      // Enable "SIGCLD" so that the proc. doesn't unkowningly
      // create zombie children (e.g. via "system()"):

      signal(SIGCLD, SIG_DFL);
      return(TRUE);
    default:
      printf("INsync.c: _insync(): invalid step encoutered -> exiting:"
             "\n\tprocstep = %d\n\tprocess = %s",
             (int)IN_SDPTAB[IN_PIDNX].procstep, IN_LDPTAB[IN_PINDX].proctag);
      return(FALSE);
    }

    // Disable any timers which may be used by a client process so
    // that they don't interfere with the nitialization sequence.

    itmr_val.it_value.tv_sec = 0;
    itmr_val.it_vale.tv_usec = 0;
    itmr_val.it_interval.tv_sec = 0;
    itmr_val.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &itmr_val, NULL);
    setitimer(ITIMER_VIRTUAL, &itmr_val, NULL);
    setitimer(ITIMER_PROF, &itmr_val, NULL);

    (Void) alrm((U_short) 0);

    osigset(SIGINT, INsigintIgnore);
		signal(SIGPROF,SIG_IGN);
		signal(SIGVTALRM,SIG_IGN);
		signal(SIGHUP,SIG_IGN);
		signal(SIGALRM,SIG_IGN);
		sighold(SIGALRM);
		signal(SIGUSR1,SIG_IGN);
		tsleep.tv_sec = 0;
		tsleep.tv_nsec = 50000000;
		while(IN_SDPTAB[IN_PINDX].procstate == IN_HALTED){
			nanosleep(&tsleep, NULL);
		}
  }
}

#endif

// Description:
//   This function transitions the calling process to STEADY state.
//
// Inputs:
//
//   Shared Memory:
//     IN_SDPTAB[]
//
//   Private Memory:
//     IN_ldata
//  Returns:
//    GLsuccess - if transition was accepted
//    GLfail - if process state was invalid for transition
//

GLretVal _in_init_complete() {
  // Check if the previous state valid for transition to occur
  // Processes can all IN_INIT_COMPLETE() and IN_CRIT_COMPLETE()
  // in succession without INIT being able to transition a process
  // to IN_CPREADY, thereforce IN_EPROCESS is also a valid state.
  // Also, if softwar checks are inhibited, allow transition to
  // IN_CPREADY with a warning that critical functionality was
  // not achieved.
  if (IN_SDPTAB[IN_PINDX].procstep != IN_CPREADY &&
      IN_SDPTAB[IN_PINDX].procstep != IN_EPROCESS) {
    if ((IN_LDSTATE.softchk != IN_INHSOFTCHK &&
         IN_LDPTAB[IN_PINDX].softchk != IN_INHSOFTCHK) ||
        IN_SDPTAB[IN_PINDX].procstep != IN_PROCESS) {
      if (IN_SDPTAB[IN_PINDX].procstep != IN_STEADY) {
        printf("rept init %s called in_init_complete procstep %s",
               IN_LDPTAB[IN_PINDX].proctag,
               IN_SQSTEPNM(IN_SDPTAB[IN_PINDX].procstep));
      }
      return(GLfail);
    } else {
      printf("REPT INIT %s COMPETED INIT WITHOUT CRITICAL FUNCTIONALITY",
             IN_LDPTAB[IN_PINDX].proctag);
    }
  } else {
    if (IN_LDPTAB[IN_PINDX].print_progress) {
      IN_PROGRESS(INcompleteStr);
    }
  }

  IN_SDPTAB[IN_PINDX].procstep = IN_ECPREADY;
  return(GLsuccess);
}

// Description:
//   This function trnasitions the calling process to CPREADY state
//
// Inputs:
//
//   Shared memory:
//     IN_SDPTAB[]
//
//   Private Memory:
//     IN_ldata
//
// Returns:
//   GLsuccess - if transition was accepted
//   GLfail - if process state was invalid for transition
//
GLretVal _in_crit_complete() {
  // Check if the previous state valid for transition to occur
  if (IN_SDPTAB[IN_PINDX].procstep != IN_PROCESS) {
    return(GLfail);
  }

  IN_SDPTAB[IN_PINDX].procstep = IN_EPROCESS;
  // The following message is meaningless for non-critical processes so skip it
  if (IN_LDPTAB[IN_PINDX].proc_category == IN_CP_CRITICAL &&
      IN_LDPTAB[IN_PINDX].print_progress) {
    IN_PROGRESS("CRITICAL FUNCTIONALITY");
  }
  return(GLsuccess);
}

Void _in_sanpeg() {
  if (IN_PINDX != -1) {
    IN_SDPTAB[IN_PINDX].count++;
  }
}

Short _in_msgh_qid() {
  if (IN_PINDX != -1) {
    return (IN_LDPTAB[IN_PINDX].msgh_qid);
  } else {
    return (-1);
  }
}

Long _in_q_size_limit() {
  if (IN_PINDX != -1) {
    return (IN_LDPTAB[IN_PINDX].q_size);
  } else {
    return (-1);
  }
}

Void _in_brev_parms(int *low, int *high, int* interval) {
  if (IN_PINDX != -1) {
    *low = IN_LDPTAB[IN_PINDX].brevity_low;
    *high = IN_LDPTAB[IN_PINDX].brevity_high;
    *interval = IN_LDPTAB[IN_PINDX].brevity_interval;
  } else {
    *low = 0;
    *high = 0;
    *interval = 0;
  }
}

Short _in_msg_limit() {
  if (IN_PINDX != -1) {
    return (IN_LDPTAB[IN_PINDX].msg_limit);
  } else {
    return (-1);
  }
}

Short _in_default_msg_limit() {
  if (IN_procdata != NULL) {
    return (IN_procdata->default_msg_limit);
  } else {
    return (-1);
  }
}

int _in_get_resource_group(short hostid) {
  for (int i = 0; i < INmaxResourceGroups; i++) {
    if (hostid == IN_procdata->resource_hosts[i])
       return(i);
  }
  return(GLfail);
}

short _in_get_oam_members(char names[][INmaxNodeName+1], int type) {
  switch (type) {
  case INoamMembersLead:
    memcpy(names, IN_procdata->oam_lead, sizeof(IN_procdata->oam_lead[0]) * 2);
    break;
  default:
    return(GLfail);
  }
  return(GLsuccess);
}
