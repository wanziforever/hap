// DESCRIPTION:
//  main()
//  INmore_core() - allocate shared memory for lab mode
//  INmain_init() - setup shared memory pointers
//  _in_getNodeState() - get the state of a node
//  _in_getWiState() - get the state of watcdog interface
//  _in_percentLoadOnActive() - get the percent of load to be sent to Active
//  _in_lrg_buf() - get the number of large message buffers in MHRPROC

#include <errno.h>     // UNIX error return values
#include <signal.h>    // UNIX signal processing info
#include <malloc.h>    // malloc() info
#include <sys/types.h>  // Unix typedefs
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/param.h>
#include <stdlib.h>
#include <time.h>

#include "sysent.h"    // exit(), pause(), getpid()
#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
//#include "cc/hdr/cr/CRdebugMsg.hh"
//#include "cc/hdr/cr/CRprmMsg.hh"
//#include "cc/hdr/cr/CRprocname.hh"

#include "cc/hdr/init/INsbt.hh"
#include "cc/hdr/init/INlibinit.hh"

extern U_long* TMtimes;
//
// NAME:
//   INmain()
//
// DESCRIPTION:
//  This function performs all of the setup required for interfacing
//  to INIT and then, after the process has been initialized by INIT,
//  INmain() repeatedly invokes "process()", pegging the processes
//  sanity timer between calls to "process()".
//
// INPUTS:
//  argc - Count of the number of command line parameters
//         passed in *argv[]
//  argv - Command line arguments
//
//  RETURNS:
//   Never
//
// CALLS:
//  process() - main processing loop
//
// CALLED BY:
//  main()
//

void INmain(short argc, char *argv[]) {
  GLretVal ret;
  U_char dbg_runlvl; // Run level passed to procinit() when in DEBUG mode
  SN_LVL sn_lvl; // Recovery level used in IN_DEBUG mode

  // This CRERRINIT call will allow us to ignore signals if
  //   bit CRinit+60 is set
  // CRERRINIT(argv[0]);

  if (!IN_IS_LAB()) {
    if (IN_LBOLT != -1 && getuid() == 0) {
      // mmap lbolt
      int kmem_fd;
      if ((kmem_fd = open("/dev/kmem", 0, O_RDONLY)) < 0) {
        printf("REPT INIT ERROR FAILED TO OPEN /dev/kmem, ERRNO = %d", errno);
      }
      void *tmp_ptr;
      if ((tmp_ptr = mmap(0, PAGESIZE, PROT_READ, MAP_SHARED, kmem_fd,
                          (IN_LBOLT & PAGEMASK))) == (void*)-1) {
        printf("REPT INIT ERROR FAILED TO MMAP LBOLT, ERRNO = %d", errno);
      } else {
        // ifndef _LP64
        TMtimes = (U_long*)(((char*)tmp_ptr) + (IN_LBOLT & PAGEOFFSET));
        //TMtimes = (U_long)(((char*)tmp_ptr)+ sizeof(u_long) + (IN_LBOLT & PAGEOFFSET));
      }
      close(kmem_fd);
    }
  }

  _inargchk(argc, argv, &sn_lvl, &dbg_runlvl);

  // the following signal handlers has been removed since
  // they can corrupt the stack during an exception. a more
  // basic reason is that the core file they generate starts
  // In the signal handler, not in the actual function where
  // the exception occurred. you can still use dbx to obtain
  // the stack frame for the actual function where the exception
  // occurred but it is just more of a pain. these functions
  // don't do any cleanup, so remove them. any process which
  // wants them to enable them in procinit() or process().
  // in general, remove them.

#if 0
  INinit_signal();
#endif

  // If we're in "lab" mode invoke procinit() (and, if at a
  // recorvery level of SN_LV2 or above, sysinit()) directly

#ifndef NOMAIN
  if (IN_IS_LAB()) {
    IN_LDPTAB[0].permstate = IN_PERMPROC;
    strncpy(IN_LDPTAB[0].proctag, argv[0], IN_NAMEMX);
    if ((sn_lvl != SN_LV0) && (sn_lvl != SN_LV1)) {
      ret = sysinit(argc, argv, sn_lvl, dbg_runlvl);
      if (ret != GLsuccess) {
        printf("INmain(): \n\t\"%s\" error in lab mode\n\terror return %d "
               "from \"sysinit()\"\n", argv[0], ret);
        exit(0);
      }
    }

    IN_SDPTAB[0].procstep = IN_PROCINIT;

    ret = procinit(argc, argv, sn_lvl, dbg_runlvl);
    if (ret != GLsuccess) {
      printf("INmain():\n\t\"%s\" error in lab mode\n\terror return %d "
             "from \"procinit()\"\n", argv[0], ret);
      exit(0);
    }

    printf("REPT INIT %s LAB MODE INIT COMPLETE", argv[0]);
    IN_SDPTAB[0].procstep = IN_STEADY;

    // Now invoke "process" forever...
    process(argc, argv, sn_lvl, dbg_runlvl);

    // Now, invoke "process()"...forever. Note that there is
    // no need to peg the sanity timer as this process is not
    // under INIT's control while in "lab" mode:

    for (;;) {
      process(argc, argv, SN_NOINIT, dbg_runlvl);
    }
  }
#endif
  
  // Initialzie the pointer to the assert function that CRERROR and
  // CRASSERT will call.
  //CRsetAssetFn(_inassert);

  IN_ldata->argc = argc;
  IN_ldata->argv = argv;

  class sigaction act;
  sigemptyset(&act.sa_mask);
  act.sa_handler = _indist;
  act.sa_flags = SA_SIGINFO;
  (Void) sigaction(SIGUSR2, &act, NULL);
  // Setup SIGTERM signal handler
  signal(SIGTERM, SIG_DFL);
  IN_SDPTAB[IN_PINDX].ireq_lvl = SN_NOINIT;
  struct timespec tsleep;
  tsleep.tv_sec = 0;
  tsleep.tv_nsec = 50000000;
  IN_SDPTAB[IN_PINDX].procstate = IN_HALTED;
  IN_SDPTAB[IN_PINDX].procstep = IN_EHALT;
  while (IN_SDPTAB[IN_PINDX].procstate == IN_HALTED); {
    nanosleep(&tsleep, NULL);
  }

  _insync();

  // At this point, the process has completed its initialization.
  // If the process is not critical, invo=e the macros to transition
  // the other steps before calling process().
  if (IN_LDPTAB[IN_PINDX].proc_category != IN_CP_CRITICAL) {
    IN_CRIT_COMPLETE();
    IN_INIT_COMPLETE();
  }

  signal(SIGCLD, SIG_DFL);
  (Void) sigaction(SIGUSR2, &act, NULL);
#ifndef NOMAIN
  process(argc, argv, IN_LDPTAB[IN_PINDX].sn_lvl, IN_LDSTATE.run_lvl);

  // Now, peg the sanity flag and invoke "process()"...forever...
  for (;;) {
    IN_SANPEG();
    process(argc, argv, SN_NOINIT, IN_LDSTATE.run_lvl);
  }
#endif
}

extern Long INsdata[];
extern Long INldata[];

// Initialize process local init information and attach to
// INIT shared memory, This function can by global constructor
// code so it may excute independent of main().
//
// WARNING:  This function cannot use any objects initialized through
// global constructors, since constructors may not be executed at this point

void INmain_init() {
  char * msgh_name;
  // Only do this once
  if (IN_sdata != 0) {
    return;
  }

  msgh_name = getenv(IN_MSGH_ENV);

  if (msgh_name == 0) {
    // Environment variable not setup, must be in lab mode
    IN_ldata->mode = LAB_MODE;
  } else {
    IN_ldata->mode = NORM_MODE;
    // Set up CRprocname[], This heps in properly identifying
    // the process generating messages, before standard CR
    // initialization takes place.
    //strcpy(CRprocname, msgh_name);
  }

  if (IN_IS_LAB()) {
    IN_sdata = (IN_SDATA*)INsdata;
    IN_procdata = (IN_PROCDATA*)INldata;

    IN_PINDX = 0;

    for (int j = 0; j < IN_NUMSEMIDS; j++) {
      IN_SDPTAB[0].semids[j] = -1;
    }

    IN_SDPTAB[0].error_count = 0;
    IN_SDPTAB[0].progress_mark = 0;
    IN_SDPTAB[0].procstate = IN_RUNNING;
    IN_SDPTAB[0].procstep = IN_READY;
    IN_LDPTAB[0].proctag[IN_NAMEMX-1] = '\0';
    IN_LDPTAB[0].pathname[0] = '\0';
    IN_LDPTAB[0].msgh_qid = -1;
  } else {
    int shmid;
    if ((shmid = shmget((key_t)INMSGBASE, sizeof(IN_SDATA), 0)) < 0) {
      exit(IN_SHM_ERR);
    }

    // At one time it was necessary to place
    // the first shared memory segment beyond
    // the end of text+data memory so that ample heap
    // memory addressability will exist -- on the mips,
    // all subsequent shared memory attachment using
    // a second argument of 0 in shmat calls (which
    // lets UNIX pic the placement address) would
    // grab addresses following the initial shared
    // memory attachment location.
    // This has changed in SVR4, however flexibility to fix
    // the address in the future by changing the value of
    // IN_SHMAT_ADDRESS has been retained.
    // The IN_SHMAT ADDRESS value can be found in
    // cc/lib/init/INlibinit.hh

    char *address = (char*)IN_SHMAT_ADDRESS;

    if ((IN_sdata = (IN_SDATA*)shmat(shmid, address,
                                     SHM_RND)) == (IN_SDATA*)-1) {
      exit(IN_SHM_ERR);
    }

    // Now attach to the shared memory segment which is only
    // modified by init but is readable to all procs:
    if ((shmid = shmget((key_t)INPDATA, sizeof(IN_PROCDATA), 0)) < 0) {
      exit(IN_SHM_ERR);
    }

    if ((IN_PINDX = _ingetindx(msgh_name)) < 0) {
      // Process dost not exist in INIT's tables
      exit(IN_SHM_ERR);
    }
  }
}

// Return the current state of the node identified by the host id
char _in_getNodeState(short hostId) {
  return(IN_NODE_STATE[hostId]);
}

// Return the current state of the local watchdog interface
char _in_getWdiState() {
  return (IN_LDWDSTATE);
}

// Return the value of percent load to be send to active
int _in_percentLoadOnActive() {
  return(IN_LDPERCENTLOADONACTIVE);
}

int _in_num_msgh_outgoing() {
  return(IN_procdata->num_msgh_outgoing);
}

int _in_num_lrg_buf() {
  return(IN_procdata->num_lrg_buf);
}

int _in_maxCargo() {
  return(IN_procdata->maxCargo);
}

int _in_minCargoSz() {
  return(IN_procdata->minCargoSz);
}

int _in_cargoTmr() {
  return(IN_procdata->cargoTmr);
}

int _in_buffered() {
  return(IN_procdata->buffered);
}

int _in_num256() {
  return(IN_procdata->num256);
}

int _in_num1024() {
  return(IN_procdata->num1024);
}

int _in_num4096() {
  return(IN_procdata->num4096);
}

int _in_num16384() {
  return(IN_procdata->num16384);
}

int _in_msgh_ping() {
  return(IN_procdata->msgh_ping);
}

U_short _in_network_timeout() {
  return(IN_procdata->network_timeout);
}

int _in_shutdown_timer() {
  return(IN_procdata->shutdown_timer);
}

#ifndef NOMAIN
// DESCRIPTION:
//  This is the definition of "main()" for all processes which
//  include the INIT library.
//
// INPUTS:
//  argc - Count of the number of command line parameters
//         passwd in *argv[]
//  argv - Command line arguments
//
// RETURNS:
//  Never
// CALLED BY:
//  UNIX - this is the entry point for the process.
//

int main(int argc, char *argv[]) {
  INmain_init();
  // INmain() never returns
  INmain(argc, argv);
  return(1);
}
#endif
