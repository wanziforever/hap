// DESCRIPTION:
//   This file contains the main() routine for the init process as
//   well as some assaciated utilities.
//
// FUNCTIONS:
//  main() -  INIT's initialization and control loop
//  INinit() - Initialize INIT's shared memory on a "full boot"
//  INfreeres() - Deallocate shared memory segments & semaphores
//  INinitptab[] - Initialize a process table entry for future use
//  INsigcld() - Catch SIGCLD signals
//  INsigterm() - Cach SIGTERM signals, cleanup and exit
//  INsigint() - Catch SIGINT signals
//  INsigsu() - Cach SIGUSR2 signals, setup for SU exit
//  INcheck_zombie() - Check for zombie processes
//  INprocinit() - Setup MSGH interface
//  INdump() - Dump INITs shared memory data
//  INprintHistory() - used by INdump
//  INprtDetailProcInfo() - used by INdump
//  INprtGeneralProcInfos() - used by INdump
//  INcountprocs() - used by INdump
//  INmain_init() - initialize environment
//  INkernel_map() - map kernel info into INIT variables
//  INmhMutexCheck() - check messaging mutex and INIT sanity
//  INsanset() - set the current value of sanity pegging timer
//  INsanstrobe() - strobe the sanity count
//  printprocparam() - prints the process parameters
//  printallparam() - print the reset of all the parameters

#include <string.h>
#include <time.h>
#include <malloc.h>
#include <stdlib.h>
#include <sysent.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <memory.h>
#include <unistd.h>
#include <sched.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/statvfs.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <hdr/GLtypes.h>
#include <hdr/GLreturns.h>
#include <hdr/GLmsgs.h>

#include "cc/hdr/msgh/MHresult.hh" // MHintr definition
#include "cc/hdr/tim/TMreturns.hh" // TMINTERR() macro
#include "cc/hdr/tim/TMmtype.hh" // TMtmrExpTyp message type
#include "cc/hdr/tim/TMtmrExp.hh" // TMtmrExp "message" class
#include "cc/hdr/eh/EHhandler.hh"
#include "cc/hdr/eh/EHreturns.hh"
//#include "cc/hdr/cr/CRmtype.hh"

//#include "cc/hdr/su/SUmtype.hh"
#include "cc/init/proc/INmsgs.hh"
#include "cc/hdr/init/INproctab.hh"
#include "cc/init/proc/INtimers.hh"
#include "cc/init/proc/INlocal.hh"
#include "cc/hdr/init/INinitialize.hh"
//#include "cc/hdr/ft/FTwatchdog.hh"
//#include "cc/hdr/ft/FTdr.hh"
//#include "cc/hdr/ft/FTreturns.hh"
#include "cc/hdr/init/INdr.hh"
//#include "cc/hdr/ft/FTbladeMsg.hh"
#include "cc/hdr/msgh/MHlrgMsgExt.hh"

const char *INhaltFile = "/opt/config/data/.psp-init.halt";

Long INmain_error = 0; // Keep track of errors found in INmain_init
thread_t INbase_thread;

#define INnTIMERS 3 * IN_SNPRCMX

Bool INlevel4 = FALSE;
int INsnstop = FALSE;

void printprocparam(int);
void printallparam(int);

extern GLretVal INthirdPartyExec(U_short, IN_SYNCSTEP);
void INsetSignals();
void INsetConfig();
#ifdef OLD_SU
void INcheckSU();
#endif
extern char CRprocname[];
extern "C" void* INthirdParty(void*);
extern int INrunSetup();
extern Bool MHisCC(Bool& isSimplex, Short& hostId);

Bool INissimplex;

// NAME:
//  main()
//
// DESCRIPTION:
//  This is the main entry point for the init process.
//
// INPUTS:
//
// RETURNS:
//
// CALLS:
//
// CALLED BY:
//  This process should be started by UNIX init and should have a
//  corresponding entry as a persistent (RESPAWN) process in the
//  "/etc/initab" file.
//  This process can also be called from command line in which case
//  command options are accepted. Those options are not accepted when
//  INIT is created by UNIX init.
int main(Short argc, char *argv[]) {
  int pd_shmid = (-1);
  int ld_shmid = (-1); // shared memory IDs
  const Char *initlist;
  GLretVal ret;
  U_short tag; // Tmr tag returned from tmrExp()
  Bool create_flg;
  IN_SOFTCHK sys_softchk;
  Bool dump_flg = FALSE;
  Bool kill_flg = FALSE;
  // used for graceful shutdown
  Bool graceful_kill = FALSE;

  Bool init_flg = FALSE;
  Bool reset_flg = FALSE;
  Bool offline = FALSE;
  struct stat stbuf;
  struct shmid_ds membuf;
  //FTdr dr(FTdrSS_INIT);
  INdr inDR;
  Short hostId = -1;


  //strcpy(CRprocname, "INIT");
  INbase_thread = thr_self();

  if (getuid() == 0 || getuid() == 0) {
    printf("INmain::main() set INroot to True\n");
    INroot = TRUE;
  } else {
    printf("INmain::main() set INroot to False\n");
    INroot = FALSE;
  }

  if (stat(INinhfile, &stbuf) < 0) {
    printf("INmain::main() set sys_softchk to IN_ALWSOFTCHK\n");
    sys_softchk = IN_ALWSOFTCHK;
  } else {
    printf("INmain::main() set sys_softchk to IN_INHSOFTCHK\n");
    sys_softchk = IN_INHSOFTCHK;
  }

  // Make sure init was not started from command line
  if (argc == 2 && strcmp(argv[1], "inittab") == 0) {
    if (INroot == FALSE) {
      printf("INIT: inittab option can only be used by root\n");
      exit(1);
    }
    // INIT is not being run from command line
    printf("INmain::main() INIT is not being run from command line\n");
    INcmd = FALSE;
  }else {
    printf("INmain::main() INIT is being run from command line\n");
    INcmd = TRUE;
  }

  if (INcmd == FALSE) {
    // If halt file exists, wait to continue until either a reboot
    // or the halt file goes away. This will be used to keep INIT
    // from starting up the application if it gets created
    // prematurely during Astart. Also used by Astop poweroff
    // to keep INIT from starting ofter being killed.
    while (access(INhaltFile, F_OK) == 0) {
      sleep(5);
    }

    if (access(IN_OFFLINE_FILE, F_OK) == 0) {
      printf("REPT INIT SYSTEM STOPPED, EXITING");
      system("/opt/config/bin/enableINIT FALSE");
      system("/sbin/init q");
      exit(0);
    }
    GLretVal ret = INrunSetup();
    if (ret > 0) {
      exit(0);
    } else if (ret < 0) {
      sleep(120);
      if (sys_softchk == IN_ALWSOFTCHK) {
        INsysboot(IN_SETUPFAIL, TRUE);
      } else {
        exit(0);
      }
    }
  }

  MHisCC(INissimplex, hostId);
  printf("MHisCC set INissimplex to %d\n", INissimplex);
  INmain_init();

  if (INcmd == FALSE) {
    printf("REPT INIT INIT PROGRESS 0 CREATED\n");
    // Report on any errors in environment variables
    if (INnoenv_vars & IN_NO_TZ) {
      printf("REPT INIT ERROR TZ ENVIRONMENT VARIABLE NOT SET\n");
    }

    if (INnoenv_vars & IN_NO_LANG) {
      printf("REPT INIT ERROR LC_TIME ENVIRONMENT VARIABLE NOT SET\n");
    }
  }

  // Ignore command line options if this version of INIT
  // was started by /etc/init
  if ((INcmd == TRUE) && ((argc > 3)) ||
      ((argc > 1) &&
       (strcmp(argv[1], "kill") != 0) &&
       (strcmp(argv[1], "dump") != 0))) {
    printf("%s [kill | dump [msgh_name]]\n", *argv);
    exit(0);
  }

  if (argc > 1 && INcmd == TRUE) {
    if (strcmp(argv[1], "kill") == 0) {
      kill_flg = TRUE;
      // graceful shutdown
    } else if (strcmp(argv[1], "shutdown") == 0) {
      kill_flg = TRUE;
      graceful_kill = TRUE;
    } else {
      dump_flg = TRUE;
    }
  }

  //GLretVal fdRet;
  char bStarted = -1;
  //if (hostId > 0) {
  //  if ((fdRet = dr.Open(O_RDIBKT)) == GLsuccess) {
  //    if (dr.Read(&inDR, (off_t)0, sizeof(inDR)) == sizeof(inDR)) {
  //      if (inDR.m_ccState[hostId&0x1] != S_OFFLINE) {
  //        if (stat(IN_OFFLINE_FILE, &stdbuf) == 0 &&
  //            unlink(IN_OFFLINE_FILE) == -1) {
  //          printf("REPT INIT ERROR FAILED TO DELETE OFFLINE FILE"
  //                 "ERRNO %d", errno);
  //        }
  //      } else {
  //        int fd;
  //        if ((fd = open(IN_OFFLINE_FILE, O_RDWR ! O_CREAT, 0644)) < 0) {
  //          printf("REPT INIT ERROR FAILED TO CREATE OFFLINE FILE "
  //                 "ERRNO %d", errno);
  //        }
  //        close(fd);
  //      }
  //      bStarted = inDR.m_bStarted;
  //      dr.Close();
  //    } else {
  //      printf("rept init error failed to read repository error %d",
  //             dr.ReturnCode());
  //    }
  //  } else if (fdRet != FTdrNotExist) {
  //    printf("REPT INIT ERROR FAILED TO OPEN REPOSITORY ERROR %d", fdRet);
  //  }
  //}

  if (stat(IN_OFFLINE_FILE, &stbuf) >= 0) {
    offline = TRUE;
  }
  printf("INmain::main() the offline is set to %d\n", offline);

  int i;
  // Initialize global timer variables
  printf("Initialize global timer variables\n");
  for (i = 0; i < IN_SNPRCMX; i++) {
    INproctmr[i].sync_tmr.tindx = -1;
    INproctmr[i].rstrt_tmr.tindx = -1;
    INproctmr[i].gq_tmr.tindx = -1;
  }
  INinittmr.tindx = -1;
	INpolltmr.tindx = -1;
	INarutmr.tindx = -1;
	INaudtmr.tindx = -1;
	INvmemtmr.tindx = -1;
	INsanitytmr.tindx = -1;
	INcheckleadtmr.tindx = -1;
	INsetLeadTmr.tindx = -1;
	INsetActiveVhostTmr.tindx = -1;
	INoamReadyTmr.tindx = -1;
	INvhostReadyTmr.tindx = -1;
	INevent.tmrInit(FALSE, INnTIMERS);	/* Initialize relative timers */
	INworkflg = TRUE;
  printf("Initialize signals\n");
	INsetSignals();

  printf("Check to see if we should use the data in the shared memory\n");
  // Check to see if we should use the data in the shared memory
  // segments -- if they exist;
  if (((pd_shmid = shmget((key_t)INPDATA, sizeof(IN_PROCDATA), 0)) >= 0) &&
      ((ld_shmid = shmget((key_t)INMSGBASE, sizeof(IN_SDATA), 0)) >= 0)) {
    // Shared memory segments already existed, clear
    // create flag
    create_flg = FALSE;
    printf("Shared memory segments already existed, clear it, set create flag to %d\n", create_flg);
  } else if (dump_flg == TRUE) {
    // INIT's shared memory is not currently there, simply
    // print a warning message and exit;
    printf("INIT: Shared memory segments are not alocated!\n");
    exit(0);
  } else if (kill_flg == TRUE) {
    // Deallocate pd_shmid if it was attached
    // NOTE: INfreeres must always release client shared memory first
    printf("INmain::main() kill flag is True, rm the share memory\n");
    if (pd_shmid > 0 && shmctl(pd_shmid, IPC_RMID, &membuf) < 0) {
      printf("INmain(): shmctl() fail, errno = %d", errno);
    }
    exit(0);
  } else {
    // Deallocate pd_shmid if it was attached
    printf("Deallocate pd_shmid if it was attached\n");
    if (pd_shmid > 0 && shmctl(pd_shmid, IPC_RMID, &membuf) < 0) {
      printf("INmain(): shmctl() fail, error = %d", errno);
    }
    printf("Going to create the share memory for INPDATA\n");
    if ((pd_shmid = shmget((key_t)INPDATA, sizeof(IN_PROCDATA),
                           (0744 | IPC_CREAT))) < 0) {
      // Something's wrong, can't go on without shared memory
      // so try to boot if possibel
      printf("INmain():shmget() error for IN_PROCDATA, errno = %d", errno);
      // Only try to boot if stated by UNIX init
      if ((sys_softchk = IN_ALWSOFTCHK) && (INcmd == FALSE)) {
        // Do UNIX BOOT
        sleep(150);
        INsysboot(IN_SHMFAIL, TRUE);
      }
      exit(0);
    }
    printf("Going to create the share memory for IN_SDATA\n");
    if ((ld_shmid = shmget((key_t)INMSGBASE, sizeof(IN_SDATA),
                           (0766 | IPC_CREAT))) < 0) {
      // Something's wrong, can't go on without shared memory
      // so try to boot if possible
      printf("INmain(): shmget() error for IN_SDATA, errno = %d\n", errno);
      if ((sys_softchk == IN_ALWSOFTCHK) && (INcmd == FALSE)) {
        // Do UNIX BOOT
        sleep(150);
        INsysboot(IN_SHMFAIL, TRUE);
      }
      exit(0);
    }
    printf("INmain::main() set create flag to True\n");
    create_flg = TRUE;
  }
  // If SHM_RDONLY bit is not set, read/write mode is the default for shmat
  int shmflag;
  if (dump_flg == TRUE) {
    // For the "dump" case we only need to access memory
    // so set mode to read only.
    shmflag = SHM_RDONLY;
  } else {
    shmflag = SHM_LOCK;
  }
  // Now attach to the shared memory segmens
  printf("Now attach to the shared memory segmens for IN_procdata\n");
  if ((IN_procdata = (IN_PROCDATA*)shmat(pd_shmid, (Char*)0,
                                         shmflag)) == (IN_PROCDATA*)-1) {
    // "Fatal" error, boot
    printf("INmain():shmat() error on PROCDATA table, errno = %d\n", errno);
    if ((sys_softchk == IN_ALWSOFTCHK) && (INcmd == FALSE)) {
      // Do Unix BOOT
      sleep(150);
      INsysboot(IN_SHMFAIL, TRUE);
    }
    printf("REPT INIT ERROR SYSTEM BOOT DENIED INIT EXITING\n");
    sleep(150);
    exit(0);
  }

  // Check the version of shared memory fo rcompatibility.
  // If memory is incompatible, just return and assume that
  // incomplatible field update was attempted.
  if (create_flg == FALSE && IN_LDSHM_VER != IN_SHM_VER &&
      INcmd == FALSE) {
    printf("REPT INIT ERROR INCOMPATIBLE SHARED MEMORY VERSION "
           "0x%1x INIT EXITING\n", IN_LDSHM_VER);
    if (sys_softchk == IN_ALWSOFTCHK) {
      // Do not boot, just exit
      sleep(150);
      exit(0);
    }
    // If software checks were inhibited, force the
    // shared memory version to conform.
    IN_LDSHM_VER = IN_SHM_VER;
  }
  printf("Now attach to the shared memory segmens for IN_SDATA\n");
  if ((IN_sdata = (IN_SDATA*)shmat(ld_shmid, (Char*)0,
                                   shmflag)) == (IN_SDATA*)-1) {
    // "Fatal" error, boot
    printf("INmain(): shmat() error on IN_SDATA table, errno = %d\n", errno);

    if ((sys_softchk == IN_ALWSOFTCHK) && (INcmd == FALSE)) {
      // Do UNIX BOOT
      sleep(150);
      IN_procdata = (IN_PROCDATA*) -1;
      INsysboot(IN_SHMFAIL, TRUE);
    }
    printf("REPT INIT ERROR SYSTEM BOOT DENIED INIT EXITING\n");
    sleep(150);
    exit(0);
  }
  int indx;
  if (create_flg) {
    // "New start" -- must initialize sahred data tables:
    // preset the shared memory to 0xff to easily notice
    // uninitialized fields.
    // Also report that full node initialization is occuring
    printf("INmain::main() create flag is True branch enter\n");
    time_t last_running = (time_t) -1;
    int timefd = open(INtimeFile, O_RDONLY);
    if (timefd >= 0) {
      read(timefd, &last_running, sizeof(last_running));
      close(timefd);
    }
    if (last_running != (time_t)-1) {
      printf("REPT INIT SYSTEM INITIALIZATION STARTED - LAST RUNNING %s\n",
             ctime(&last_running));
    } else {
      printf("REPT INIT SYSTEM INITIALIZATION STARTGED - LAST RUNNING UNKOWN\n");
    }

    memset((char*)IN_sdata, 0xff, sizeof(IN_SDATA));
    memset((char*)IN_procdata, 0xff, sizeof(IN_PROCDATA));
    INinit();
    init_flg = TRUE;
    printf("INmain::main() init_flag is set to True here\n");
  }
  if (kill_flg == TRUE) {
    printf("INmain::main() init use kill argument\n");
    if (graceful_kill == TRUE) {
      printf("REPT INIT EECUTION A GRACEFUL SYSTEM SHUT DOWN");
    } else {
      printf("REPT INIT EXECUTION A SYSTEM SHUTDOWN\n");
    }
    // Kill the other INIT process if it is still around
    // NOTE: kill option cannot be used if UNIX run level is 4
    // since /etc/init will just keep on respawning init
    if (IN_procdata->pid > (pid_t)1) {
      kill(IN_procdata->pid, SIGKILL);
      sleep(1);
    }

    INsanset(0L);
    // Ignore this request if shared memory is
    // incompatible.
    if (IN_LDSHM_VER != IN_SHM_VER) {
      printf("rept init error incompatible shared memory version "
             "0x1x init exiting", IN_LDSHM_VER);
      exit(1);
    }
    // graceful system shutdown
    if (graceful_kill == TRUE) {
      INevent.attach();
      if (IN_procdata->vhost[0][0] != 0) {
        char name[20];
        INevent.getMyHostName(name);
        if (strcmp(name, IN_procdata->vhost[0]) == 0) {
          INvhostMate = 1;
        } else {
          INvhostMate = 0;
        }
        sprintf(INvhostMateName, "%s:INIT", IN_procdata->vhost[INvhostMate]);
      }
      //FTbladeStChgMsg msg(S_OFFLINE, INV_INIT, IN_LDSTATE.softchk);
      //if (INvhostMate >= 0) {
      //  msg.setVhostmate(IN_procdata->vhost[INvhostMate]);
      //  msg.setVhostState(INstandby);
      //}
      INswitchVhost();
      // stopp processes by runlevel, only runlevel 20 and
      // above will be shutdown 1st

      int MAXRUNLVL = 100;
      int MINRUNLVL = 15;
      IN_LDSTATE.systep = IN_CLEANUP;

      printf("Attempting a graceful stop of all processes\n");
      for (int runLevelndx=MAXRUNLVL; runLevelndx >= MINRUNLVL;
           runLevelndx--) {
        for (int i = 0; i < IN_SNPRCMX; i++) {
          if (IN_LDPTAB[i].run_lvl == runLevelndx) {
            // time to try graceful kill, inhibit restart, make non-critical
            IN_LDPTAB[i].startstate = IN_INHRESTART;
            IN_LDPTAB[i].proc_category = IN_NON_CRITICAL;
            if (IN_LDPTAB[i].third_party == FALSE) {
              if (INreqinit(SN_LV0, i, 0, IN_MANUAL,
                            "MANUAL ACTION") != GLsuccess) {
                printf("Error on shutdown of process %s\n", IN_LDPTAB[i].proctag);
              }
            } else {
              INthirdPartyExec(i, IN_CLEANUP);
              IN_LDPTAB[i].tid = IN_FREEPID;
            }
          }
        }
      }
      // sleep while processes exit
      sleep(5);
    }
    INsanset(0L);
    INkillprocs();
    INfreeres();

    exit(0);
  }

  if (dump_flg == TRUE) {
    if (argc == 3) {
      INdump(argv[2]);
    } else {
      INdump((char *)0);
    }
    exit(0);
  }
  // Save the proces pid only if this iniht is not command line version
  if (INcmd == FALSE) {
    IN_procdata->pid = getpid();
    INsanset(600000L);
  }
  memset(INqInUse, 0x0, sizeof(INqInUse));
  for(indx = 0; indx < IN_SNPRCMX; indx++) {
    if (IN_INVPROC(indx)) {
      continue;
    }
    // Clear out tid, there are no threads running
    IN_LDPTAB[indx].tid = IN_FREEPID;
    // Initialize queue resevation array
    if (IN_LDPTAB[indx].msgh_qid >= 0 && IN_LDPTAB[indx].msgh_qid < MHmaxQid) {
      INqInUse[IN_LDPTAB[indx].msgh_qid] = 1;
    }
  }

  // Set system software checks
  printf("Set system software checks\n");
  IN_LDSTATE.softchk = sys_softchk;

  if (sys_softchk == IN_INHSOFTCHK) {
    INSETTMR(INsoftchktmr, INSOFTCHKTMR, INSOFTCHKTAG, TRUE);
  } else {
    INCLRTMR(INsoftchktmr);
    if (IN_LDALMSOFTCHK != POA_INF) {
      printf("REPT INIT SYSTEM SOFTWARE CHECKS INHIBITED\n");
      IN_LDALMSOFTCHK = POA_INF;
    }
  }

  INsetConfig();
  if (offline) {
    IN_LDCURSTATE = S_OFFLINE;
  }
  if (bStarted == 0) {
    IN_LDCURSTATE = S_UNAV;
  }
  initlist = (const Char*)INdinitlist;

  if (initlist == (Char *const) 0) {
    initlist = (const Char *)INdinitlist;
  }
  if (strlen(initlist) >= IN_PATHNMMX) {
    printf("rept init error initlist name exceeds max length %d name: %s",
           IN_PATHNMMX, initlist);
    sleep(150);
    exit(0);
  }

#ifdef OLD_SU
  INcheckSU();
#endif

  strcpy(IN_LDILIST, initlist);
  IN_LDILIST[IN_PATHNMMX - 1] = (Char)0; // for audits

  ret = INgetpath(IN_LDILIST, FALSE);

  // Decide if all information in initlist should be updated
  if (IN_LDSTATE.initstate != IN_NOINIT && IN_LDSTATE.initstate != IN_CUINTVL) {
    init_flg = TRUE;
  }
  // Read the initlist, read in audit mode if intentional exit
  if (IN_LDEXIT == FALSE) {
    if ((ret != GLsuccess) || (INrdinls(init_flg, FALSE) == GLfail)) {
      // Keep the system from rolling to fast because of
      // bad initlist
      sleep(150);
      INescalate(SN_LV2, INBADINITLIST, IN_SOFT, INIT_INDEX);
    }
  } else {
    // Re-read in audit mode, ignore errors
    INrdinls(init_flg, TRUE);
  }

  // Reset the value of sysmon time based on initlist parameter
  // If initlist parameter is odd or node is offline, disable sanity pegging
  if (offline || !bStarted || (IN_LDARUINT & 0x1)) {
    INsanset(0L);
  } else {
    INsanset(IN_LDARUINT * 1000L);
  }
  INetype = EHTMRONLY;

  // Set the runlevel based on the state we want to go to.
  printf("INmain::main() going to check IN_LDCURSTATE %c\n", IN_LDCURSTATE);
  switch (IN_LDCURSTATE) {
  case S_INIT:
  case S_UNAV:
  case S_OFFLINE:
  case S_STBY:
    printf("REPT INIT BEGINNING STATE %c", IN_LDCURSTATE);
    // Delete all the process except MSGH
    for (i = 0; i < IN_SNPRCMX; i++) {
      if (IN_LDPTAB[i].run_lvl != IN_MSGH_RLVL) {
        INinitptab(i);
      }
    }
  case S_ACT:
    if (IN_LDSTATE.initstate != IN_NOINIT) {
      IN_LDSTATE.sn_lvl = SN_LV4;
    }
    break;
  case S_LEADACT:
    break;
  default:
    //INIT_ERROR("Invalid state %c", IN_LDCURSTATE);
    printf("Invalid state %c\n", IN_LDCURSTATE);
    INescalate(SN_LV4, INBADSTATE, IN_SOFT, INIT_INDEX);
    IN_LDCURSTATE = S_ACT;
  }

  printf("INmain::main() going to check sn_lvl %d\n", IN_LDSTATE.sn_lvl);
  switch (IN_LDSTATE.sn_lvl) {
  case SN_NOINIT:
    // Take no recovery actions on intentional INIT exits
    if (IN_LDEXIT == TRUE) {
      break;
    }

    if (++IN_LDSTATE.init_deaths > IN_MAXDEATHS) {
      INescalate(SN_LV2, IN_DEATH_THRESH, IN_SOFT, INIT_INDEX);
    }
    // RUN audit
    if (INaudit(TRUE) != GLsuccess) {
      INescalate(SN_LV2, INBADSHMEM, IN_SOFT, INIT_INDEX);
    }
    break;
  case SN_LV2:
    // Not supported
    printf("REPT INIT ERROR UNSUPPORTED INITIALIZATION LEVEL 2");
  case SN_LV3:
    // Take no recovery actions on intential INIT exits
    if (IN_LDEXIT == TRUE) {
      break;
    }
    if (IN_LDSTATE.initstate != INITING2) {
      INescalate(IN_LDSTATE.sn_lvl, IN_DEATH_INIT, IN_SOFT, INIT_INDEX);
      // Try to continue current activities
      break;
    } else {
      // Valid initialization
      IN_LDSTATE.initstate = INITING;
      printf("REPT INIT STARTING %s SYSTEM RESET",
             IN_SNLVLNM(IN_LDSTATE.sn_lvl));
    }
    // Run audit
    if (INaudit(TRUE) != GLsuccess) {
      INescalate(SN_LV4, INBADSHMEM, IN_SOFT, INIT_INDEX);
    }
    IN_SDERR_COUNT = 0;
#ifdef OLD_SU
    INautobkout(FALSE, TRUE);
#endif
    if (IN_LDSTATE.sn_lvl == SN_LV2) {
      IN_LDSTATE.systep = IN_CLEANUP;
      for (indx = 0; indx < IN_SNPRCMX; indx++) {
        if (IN_VALIDPROC(indx)) {
          IN_LDPTAB[indx].sn_lvl = IN_LDSTATE.sn_lvl;
          // Sequence the process through cleanup
          INsync(indx, IN_BCLEANUP);
        }
      }
    } else {
      INprocinit();
      INfailover failover;
      GLretVal retval;
      if ((retval = INevent.broadcast(INmsgqid, (char*)&failover,
                                      sizeof(failover))) != GLsuccess) {
        printf("failed to send failover message, ret %d\n", retval);
      }
      INdidFailover = TRUE;
      INlv4_count(FALSE);
      IN_LDSTATE.systep = IN_SYSINIT;
      IN_LDSTATE.sync_run_lvl = 0;
      for (indx = 0; indx < IN_SNPRCMX; indx++) {
        if (IN_VALIDPROC(indx)) {
          IN_LDPTAB[indx].sn_lvl = IN_LDSTATE.sn_lvl;
          if (IN_SDPTAB[indx].procstate == IN_NOEXIST) {
            IN_LDPTAB[indx].syncstep = IN_SYSINIT;
          }
        }
      }
    }
    // Set up information about this init, for future reference
    // in IN_LDINFO:
    IN_LDINFO.init_data[IN_LDINFO.ld_indx].psn_lvl = IN_LDSTATE.sn_lvl;
    IN_LDINFO.init_data[IN_LDINFO.ld_indx].prun_lvl = IN_LDSTATE.final_runlvl;
    IN_LDINFO.init_data[IN_LDINFO.ld_indx].str_time = time((time_t *)0);
    IN_LDINFO.init_data[IN_LDINFO.ld_indx].num_procs = INcountprocs();

    // Hard to tell how we got here ...
    IN_LDINFO.init_data[IN_LDINFO.ld_indx].source = IN_SOFT;
    strcpy(IN_LDINFO.init_data[IN_LDINFO.ld_indx].msgh_name, IN_MSGHQNM);
    IN_LDINFO.init_data[IN_LDINFO.ld_indx].ecode = 0;
    break;
  case SN_LV4:
  case SN_LV5:
    // Take no recovery actions on intentional INIT exits
    if (IN_LDEXIT == TRUE || IN_LDCURSTATE == S_OFFLINE) {
      break;
    }
    if (create_flg == FALSE && IN_LDSTATE.initstate != INITING2) {
      // INIT merely died during system init
      printf("INIT merely died during system init, IN_LDEXIT=%d, initstat=%d\n", IN_LDEXIT, IN_LDSTATE.initstate);
      INescalate(SN_LV4, IN_DEATH_INIT, IN_SOFT, INIT_INDEX);
    } else {
      printf("setup information about this init\n");
      IN_LDSTATE.initstate = INITING;
      // If threshold of level 4 inits is exceeded, do a UNIX Boot
      long lv4_count;
      //lv4_count = INlv4_count(FALSE);
      // Set up information about this init. for future reference
      // in IN_LDINFO:
      IN_LDINFO.init_data[IN_LDINFO.ld_indx].psn_lvl = IN_LDSTATE.sn_lvl;
      IN_LDINFO.init_data[IN_LDINFO.ld_indx].prun_lvl = IN_LDSTATE.final_runlvl;
      IN_LDINFO.init_data[IN_LDINFO.ld_indx].str_time = time((time_t *)0);
      IN_LDINFO.init_data[IN_LDINFO.ld_indx].num_procs = INcountprocs();

      // Hard to tell how we got here...
      IN_LDINFO.init_data[IN_LDINFO.ld_indx].source = IN_BOOT;
      strcpy(IN_LDINFO.init_data[IN_LDINFO.ld_indx].msgh_name, IN_MSGHQNM);
      IN_LDINFO.init_data[IN_LDINFO.ld_indx].ecode = 0;
      printf("REPT INIT STARTING %s SYSTEM RESET\n",
             IN_SNLVLNM(IN_LDSTATE.sn_lvl));
#ifdef OLD_SU
      INautobkout(FALSE, TRUE);
#endif
    }
    break;
  default:
    INescalate(SN_LV2, INBADSHMEM, IN_SOFT, INIT_INDEX);
  }
  printf("going to reset intentional exit variable\n");
  // Reset intentional exit variable
  IN_LDEXIT = FALSE;

  // Map kernel variables used by INIT
  if (INkernel_map() != GLsuccess) {
    INescalate(SN_LV0, INKMEM_FAIL, IN_SOFT, INIT_INDEX);
  }
  printf("going to setup timers\n");
  // Set up timers
  if (IN_LDSTATE.initstate == IN_CUINTVL) {
    // Setup post init timer
    INSETTMR(INinittmr, IN_procdata->safe_interval, (INITTAG|INSEQTAG), FALSE);
  }

  for (indx = 0; indx < IN_SNPRCMX; indx++) {
    if (IN_INVPROC(indx)) {
      continue;
    }

    if (IN_LDPTAB[indx].third_party == TRUE && IN_SDPTAB[indx].procstep >= IN_EHALT) {
      // Create new thread
      thread_t newthread;
      if (thr_create(NULL, thr_min_stack()+64000, INthirdParty,
                     (void*)indx, THR_DETACHED, &newthread) != 0) {
        //INIT_ERROR("Failed to create third party thread, errno %d", errno);
        printf("Failed to create third party thread, errno %d", errno);
        INescalate(SN_LV1, INFORKFAIL, IN_SOFT, INIT_INDEX);
      }
      IN_LDPTAB[indx].tid = newthread;
    }

    // Per process sync timers are already set no need to reset them
    if (INproctmr[indx].sync_tmr.tindx >= 0) {
      continue;
    }

    // Set if the process should be scheduled for future restart
    if (IN_LDPTAB[indx].syncstep == INV_STEP &&
        IN_LDPTAB[indx].next_rstrt < IN_NO_RESTART) {
      INSETTMR(INproctmr[indx].sync_tmr,
               IN_LDPTAB[indx].next_rstrt,
               (INPROCTAG | INSYNCTAG | indx), FALSE);
      continue;
    }
    // set restart timer only if process is IN_STEADY
    if (IN_LDPTAB[indx].rstrt_cnt > 0 &&
        IN_SDPTAB[indx].procstep == IN_STEADY &&
        IN_LDPTAB[indx].rstrt_intvl > 0) {
      INSETTMR(INproctmr[indx].rstrt_tmr, IN_LDPTAB[indx].rstrt_intvl,
               (INPROCTAG|INRSTRTAG|indx), FALSE);
    }
    // Recreate any timers we should be keeping
    if (IN_SDPTAB[indx].procstep < IN_ESYSINIT) {
      INSETTMR(INproctmr[indx].sync_tmr, INPCREATETMR,
               (INPROCTAG|INSYNCTAG|indx), FALSE);
    } else if (IN_SDPTAB[indx].procstep < IN_EPROCINIT) {
      INSETTMR(INproctmr[indx].sync_tmr, IN_LDPTAB[indx].procinit_timer,
               (INPROCTAG|INSYNCTAG|indx), FALSE);
    } else if (IN_SDPTAB[indx].procstep == IN_STEADY) {
      IN_LDPTAB[indx].sent_missedsan = FALSE;
    } else if (IN_SDPTAB[indx].procstep < IN_ECLEANUP) {
      INSETTMR(INproctmr[indx].sync_tmr, INPCLEANUPTMR,
               (INPROCTAG|INSYNCTAG|indx), FALSE);
    }
    // Rreset global queue timers
    if (IN_LDPTAB[indx].gqsync == IN_GQ) {
      INSETTMR(INproctmr[indx].gq_tmr, IN_LDPTAB[indx].global_queue_timer,
               (INPROCTAG|INGQTAG|indx), FALSE);
    }
  }
  // Attach to MSGH if possible and necessary
  if (INetype != EHBOTH) {
    INprocinit();
  }

  INsettmr(INpolltmr, INITPOLL, (INITTAG|INPOLLTAG), TRUE, TRUE);
  // Check if there is any synchronization work to do
  INinitover();

  // Start sanity pegging timer
  INSETTMR(INsanitytmr, INSANITYTMR, INSANITYTAG, TRUE);

  // Start the cyclic ARU timer, and send inital ARU msg
  INSETTMR(INarutmr, (IN_LDARUINT - 2)/3, INARUTAG, TRUE);
  INarumsg();

  // Start the cyclic audit timer
  INSETTMR(INaudtmr, INAUDTMR, INAUDTAG, TRUE);

  // Check and report on any virtual memory conditions
  INvmem_check(TRUE, FALSE);

  // Print INIT initialization complete message
  if (INcmd == FALSE) {
    printf("REPT INIT INIT RPGRESS 100 INIT COMPLETE\n");
  }

  // Print any errors that might have occured when manipulating
  // stdio file descriptors
  printf("INmain():INmain_erro = %lx\n", INmain_error);

  int e_count = 0;
  // This union is used to guarantee alignment of msgbfr on double boundary.
  // This is necessary to avoid alignment problems when accessing msgbfr
  // with a type casted structure
  union {
    double align;
    char msgbfr[MHlrgMsgBlkSize];
  };

  align = 0; // Eliminate compiler warning

  // Fall into main processing loop exit for SU if SIGUSR2 received
  while (!IN_LDEXIT) {
    // Peg main thread sanity
    INsanityPeg++;
    // Check for any initialization work...
    INsequence();
    // Now process the next "event"
    int msgSz;
    msgSz = MHlrgMsgBlkSize;
    memset(msgbfr, 0xff, 600);

    // Always read any pending messages first
    ret = INevent.getEvent(INmsgqid, msgbfr, msgSz, 0, TRUE, INetype, TRUE);
    if (ret == GLsuccess) {
      Short mtype = ((class MHmsgBase *)msgbfr)->msgType;
      // We successfully received an event (message),
      // check to see if it's a timer expiration "message":
      switch (mtype) {
      case TMtmrExpTyp:
        // Timer expired - check to see if it
        // was the "polling" timer or a timer
        // which requires some real processing:
        tag = (U_short)((class TMtmrExp *)msgbfr)->tmrTag;

        if (tag != (INITTAG | INPOLLTAG)) {
          // This timer requires some
          // processing -- invoke timer
          // processing routine:
          printf("INmain():\n\tgetEvent() indicated an expired "
                 "timer\n\ttag %x", tag);
          INtimerproc(tag);
        } else {
          // Poll timer is only set during system
          // init or a single process init. When
          // single process is initing, polltimer
          // is very short resulting in too many
          // calls to check for process death.
          // Increase that duration by using e_count.
          // INgrimreaper is called out of INaudit()
          // every 15 seconds during normal operation.

          // Check for any dead process...
          if ((e_count++ & 0x3) == 0 ||
              IN_LDSTATE.initstate == INITING) {
            INgrimreaper();
          }
        }
        break;
      default:
        INrcvmsg(msgbfr, msgSz);
        break;
      }
    } else if (ret != MHintr) {
      // We should always return successfully from
      // "getEvent()" or get interrupted (by a process
      // death/SIGCLD signal)...output an error message
      // and go on...may want to take some recovery action
      // (other than exit()!) if this is an internal
      // timing library error
      printf("\"getEvent()\" returned error - %d", ret);
      if (TMINTERR(ret) == TRUE) {
        INescalate(SN_LV0, ret, IN_SOFT, INIT_INDEX);
      }
    } else {
      // Check for any dead processes since probably
      // signal was received
      INgrimreaper();
    }
  }

  if (INsnstop) {
    // Transition the node to offline, but first beartbeat INIT
    if (IN_LDCURSTATE != S_INIT && IN_LDCURSTATE != S_OFFLINE) {
      //FTbladeStChgMsg msg(S_OFFLINE, INV_INIT, IN_LDSTATE.softchk);
      //if (INvhostmate >= 0) {
      //  msg.setVhostmate(IN_procdata->vhost[INvhostMate]);
      //  msg.setVhostState(INstandby);
      //}
      //msg.send();
      IN_LDCURSTATE = S_INIT;
    }
    INrm_check_state(S_OFFLINE);
  }
  return(0);
}

#ifdef OLD_SU
void INcheckSU() {
  struct stat stbuf;
  int sufd;

  // Check if SU is in progress and update the SU info
  if (stat(INsufile, &stbuf) >= 0) {
    // SU path file exits
    if (stbuf.st_size > 0 && stbuf.st_size < IN_PATHNMMX) {
      if ((sufd = open(INsufile, O_RDONLY)) < 0) {
        INIT_ERROR(("INmain: failed to open %s, errno %d, file removed",
                    INsufile, errno));
        (void)unlink(INsufile);
      }
      char proclist[IN_PATHNMMX];
      memset(proclist, 0 , IN_PATHNMMX);
      if (read(sufd, (void*)proclist, (int)stbuf.st_size) != stbuf.st_size) {
        INIT_ERROR(("INmain: failed to read %s, errno %d, file removed",
                    INsufile, errno));
        (void)unlink(INsufile);
      }
      if (INgetsudata(proclist, FALSE) == GLsuccess) {
        INsupresent = TRUE;
      } else {
        printf("REPT INIT ERROR BAD SU PROCESS LIST FILE %s", proclist);
        // Should INsufile be deleted here
      }
    } else {
      INIT_ERROR(("INmain: invalid %s size %d, file removed",
                  INsufile, stbuf.st_size));
      (void)unlink(INsufile);
    }
  }
}
#endif

void INsetSignals() {
  class sigaction act;
  act.sa_flags = 0;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  act.sa_handler = SIG_IGN;
  (void)sigaction(SIGQUIT, &act, NULL);
  (void)sigaction(SIGPROF, &act, NULL);
  (void)sigaction(SIGHUP, &act, NULL);
  (void)sigaction(SIGALRM, &act, NULL);
  act.sa_sigaction = INsigsnstop;
  act.sa_flags = SA_SIGINFO;
  (void)sigaction(SIGUSR1, &act, NULL);

  act.sa_handler = INsigint;
  (void)sigaction(SIGINT, &act, NULL);
  act.sa_handler = INsigsu;
  (void)sigaction(SIGUSR2, &act, NULL);
  act.sa_sigaction = INsigterm;
  act.sa_flags = SA_SIGINFO;
  (void)sigaction(SIGTERM, &act, NULL);
  (void)sigset(SIGCLD, INsigcld);
}

void INsetConfig() {
  struct stat stbuf;
  // Check to see if we are running from command line, if not,
  // take the following actions:
  // - set INIT's UNIX priority to zero
  // - make sure that the core file directory exists, creating
  //   it if necessary
  // - make sure that INIT's core file directory exists, creating
  //   it if necessary
  // - move any core files for INIT for safe keeping
  // - change the current working directory to INIT's core dump
  //   directory
  if (INcmd == FALSE && INroot == TRUE) {
    // Make init real time class in linux
    struct sched_param params;
    params.sched_priority = INmaxPrioRT;

    if (sched_setscheduler((pid_t)0, SCHED_FIFO, &params) != 0) {
      //INIT_ERROR(("Failed to set priority, errno %d"));
      printf("Failed to set priority, errno %d", errno);
    }

    // make sure that the dump directory exists so it can be
    // used as the root of the working directories used for
    // INIT's children:
    if (stat(INexecdir, &stbuf) < 0) {
      if (mkdir(INexecdir, 0777) < 0) {
        printf("INmain(): mkdir() call:\n\terrno = %d, path = \"%s\"",
               errno, INexecdir);
      } else {
        (Void)chmod(INexecdir, 0777);
        printf("INmain(): created \"%s\" directory", INexecdir);
      }
    } else {
      if (!(stbuf.st_mode & S_IFDIR)) {
        printf("INmain(): stat() call:\n\t\"%s\" not a directory", INexecdir);
      } else if ((stbuf.st_mode && 0x7) != 0x7) {
        printf("INmain(): stat() call:\n\t\"%s\" doesn't allow read, write, "
               "and execute for \"other\"\n\tmode 0%0d",
               INexecdir, stbuf.st_mode);
      }
    }
    // Set INIT up so that any core files it generates will
    // be saved in the appropriate place:
    Char edir[IN_NAMEMX + IN_PATHNMMX];
    strcpy(edir, INexecdir);
    strcat(edir, "/INIT");

    if (stat(edir, &stbuf) < 0) {
      if (mkdir(edir, 0777) < 0) {
        printf("Inmain(): stat() call error return:\n\terrno = %d, "
               "path = \"%s\"", errno, edir);
      } else if (chmod(edir, 0777) < 0) {
        printf("INmain(): \"chmod()\" call error return:\n\terrno = %d,"
               "path = \"%s\"", errno, edir);
      }
    } else {
      if (!(stbuf.st_mode & S_IFDIR)) {
        printf("INmain(): \"stat()\" call:\n\t \"%s\" is not a directory",
               edir);
      }
    }
    // Now check to see if a core file already exists and,
    // if so, move it:
    INchkcore("INIT");
    if (chdir(edir) < 0) {
      printf("INmain(): \"chdir()\" call returned errorno %d "
             "for directory \"%s\"", errno, edir);
    }
  }
}

// Name:
//    INinit()
//
// Description:
//   This routine is invoked during a full boot
//   The purpose of the routine is
//   to establish INIT's operating environment
//
// Inputs:
//   shared memory:
//     IN_LDPTAB[] table
//     IN_SDPTAB[] table
//     IN_LDINFO[] table
//
// Returns
//
// Calls:
//
// Called By:
//   main()
//
// Side Effects:
//   The shared memory tables mentioned above as well as
//   certain key state variables are explicitly set to
//   default values. This is done as a defensive programming
//   technique to guard against uninitialized variable errors
// 

Void INinit() {
  printf("INmain::INinit() enter\n");
  IN_LDSHM_VER = IN_SHM_VER;
	IN_LDINFO.ld_indx = 0;
	IN_LDINFO.uld_indx = 0;
	IN_LDMSGHINDX = -1;
	IN_LDARUINT = INMAXARU;
	IN_LDE_THRESHOLD = 500;
	IN_LDE_DECRATE = 20;
	IN_LDBKOUT = FALSE;
	IN_LDBQID = MHnullQ;
	IN_LDAQID = MHnullQ;
	IN_LDEXIT = FALSE;
	IN_LDBKPID = IN_FREEPID;
	IN_LDBKUCL = FALSE;
	IN_LDALMSOFTCHK = POA_INF;
	IN_LDALMCOREFULL = POA_INF;
	IN_LDSCRIPTSTATE = INscriptsNone;
#ifdef CC
	IN_LDVMEM = -1;
#endif
  IN_LDVMEMALVL = POA_INF;
  // Only use timers, must wait for
	INetype = EHTMRONLY;
  // MSGH to be active
  // INIT	deaths count
	IN_LDSTATE.init_deaths = 0;	
	IN_LDSTATE.crerror_inh = FALSE;
	IN_LDSTATE.sync_run_lvl = 0;
	IN_LDSTATE.gq_run_lvl = 0;
	IN_LDSTATE.initstate = INITING;
	IN_LDSTATE.systep = IN_SYSINIT;
	IN_LDSTATE.issimplex = INissimplex;
	IN_LDSTATE.isactive = TRUE;
	IN_procdata->network_timeout = 10;

  GLretVal ret;
  printf("INmain::INinit() IN_LDSTATE.sn_lvl=%d, INlevel4=%d, IN_LDSTATE.sn_lvl=%d\n", IN_LDSTATE.sn_lvl, INlevel4, IN_LDSTATE.sn_lvl);
  if(IN_LDSTATE.sn_lvl == -1) {
    IN_MYNODEID = MHmaxHostReg;
    IN_LDCURSTATE = S_INIT;
    IN_LDSTATE.sn_lvl = SN_LV5;
    if(INissimplex){
      printf("INmain::INinit() it is a simplex mode\n");
      IN_LDCURSTATE = S_LEADACT;
    } else {
      IN_LDCURSTATE = S_ACT;
      IN_LDSTATE.sn_lvl = SN_LV4;
    }
  }

	/* Initialize the shared memory mutex	*/
	mutex_init(&IN_SDSHMLOCK, USYNC_PROCESS, NULL);
	memset(IN_SDSHMDATA, 0xff, sizeof(IN_SDSHMDATA));
	memset(IN_procdata->active_nodes, 0x0, sizeof(IN_procdata->active_nodes));
	memset(IN_procdata->oam_lead, 0x0, sizeof(IN_procdata->oam_lead));
	memset(IN_procdata->oam_other, 0x0, sizeof(IN_procdata->oam_other));
	memset(IN_procdata->vhost, 0x0, sizeof(IN_procdata->vhost));

	/*
	 *	Initialize per-process shared memory table info.
	 */
	U_short indx;

	for (indx = 0; indx < IN_SNPRCMX; indx++) {
    INinitptab(indx);
  }

	/* Initialize SYSTAT critical indicators data */
	for(indx = 0; indx < CRNUM_INDICATORS; indx++){
    strcpy(IN_SDCIDATA.indicator[indx],"NORMAL");
  }
	
	IN_SDCIDATA.overload_value = 0;
	return;
}
  
Void INfreeres(Bool release_init) {
  int pd_shmid, ld_shmid; // Shared memory IDs
  struct shmid_ds membuf;

  // Deallocate shared memory segments & semaphores
  for (U_short indx = 0; indx < IN_SNPRCMX; indx++) {
    if (IN_VALIDPROC(indx)) {
      INfreeshmem(indx, TRUE);
      INfreesem(indx);
    }
  }

  if (release_init != TRUE) {
    return;
  }

  // Release shared memory segment used by client processes
  if (((ld_shmid = shmget((key_t)INMSGBASE, sizeof(IN_SDATA), 0)) >= 0)) {
    // Free up private INIT shared memory
    if (shmctl(ld_shmid, IPC_RMID, &membuf) < 0) {
      // Error deallocating shared memory segment:
      printf("INfreeres():\n\tshmctl() error while freeing up "
             "IN_SDATA segment, error=%d", errno);
    } else {
      // Successfully freed up the shared memory segment
      //INIT_DEBUG("INfreeres: INfreeres(): the IN_SDATA segment was deallocated");
      printf("INfreeres: INfreeres(): the IN_SDATA segment was deallocated");
    }
  } else {
    // IN_PROCDATA segment was not allocated:
    printf("Infreeres: the IN_SDATA segment did not exist");
  }

  // Release shared memory segment used by INIT processes:
  if ((pd_shmid = shmget((key_t)INPDATA, sizeof(IN_PROCDATA), 0)) > 0) {
    // Free up private INIT shared memory
    if (shmctl(pd_shmid, IPC_RMID, &membuf) < 0) {
      printf("INfreeres():\n\tshmctl() error while freeing up "
             "IN_PROCDATA segment, errno = %d", errno);
    } else {
      //INIT_DEBUG((IN_DEBUG|IN_RSTRTR),
      //           (POA_INF, "INfreeres: INfreeres(): the IN_PROCDATA"
      //            "segment was deallocated"));
      printf("INfreeres: INfreeres(): the IN_PROCDATA"
             "segment was deallocated");
    }
  } else {
    printf("INfreeres: the IN_PROCDATA segment did not exists");
  }
}

// NAME:
//   INinitptab()
// DESCRIPTION:
//    This routine initializes shared memory process table data to
//    default state.
//
// INPUTS:
//    indx - The index associated with the entry in the process tables
//           which is to be set into an "unused state"
//
// RETURNS:
//    None
//
// CALLS:
//
// CALLED BY:
//
// SIDE EFFECTS:
//   Shared memory referenced by "in_sdata[indx]" and
//   "IN_procdata.proctab[indx]" is initialized.

Void INinitptab(U_short indx) {
  IN_SDPTAB[indx].procstate = IN_INVSTATE;
  IN_SDPTAB[indx].procstep = INV_STEP;
	IN_SDPTAB[indx].ret = GLsuccess;
	IN_SDPTAB[indx].count = 1;
	IN_SDPTAB[indx].ireq_lvl = SN_NOINIT;
	IN_SDPTAB[indx].error_count = 0;
	IN_SDPTAB[indx].progress_mark = 0;
	IN_SDPTAB[indx].progress_check = 0;
  IN_SDPTAB[indx].alvl = POA_INF;

	int	i;
	for (i = 0; i < IN_NUMSEMIDS; i++) {
		IN_SDPTAB[indx].semids[i] = -1;
	}

	IN_LDPTAB[indx].syncstep = IN_MAXSTEP;
	IN_LDPTAB[indx].gqsync = IN_MAXSTEP;
	IN_LDPTAB[indx].updstate = NO_UPD;
	IN_LDPTAB[indx].sigtype = IN_SIGINV;
	memset(IN_LDPTAB[indx].proctag, 0x0, IN_NAMEMX);
	if(IN_LDSTATE.sn_lvl != SN_NOINIT){
		IN_LDPTAB[indx].sn_lvl = IN_LDSTATE.sn_lvl;
	} else if(INevent.onLeadCC()){
		IN_LDPTAB[indx].sn_lvl = SN_LV5;
	} else {
		IN_LDPTAB[indx].sn_lvl = SN_LV4;
	}
	IN_LDPTAB[indx].peg_intvl = 0;
	IN_LDPTAB[indx].rstrt_cnt = 0;
	IN_LDPTAB[indx].rstrt_intvl = IN_procdata->default_restart_interval;
	IN_LDPTAB[indx].rstrt_max = IN_procdata->default_restart_threshold;
	IN_LDPTAB[indx].next_rstrt = 0;
	IN_LDPTAB[indx].tot_rstrt = 0;
	IN_LDPTAB[indx].last_count = 0;
	IN_LDPTAB[indx].sent_missedsan = FALSE;
	IN_LDPTAB[indx].time_missedsan = 0;
	IN_LDPTAB[indx].startime = 0;
	IN_LDPTAB[indx].permstate = INVPROC;
	IN_LDPTAB[indx].pid = IN_FREEPID;
	IN_LDPTAB[indx].tid = IN_FREEPID;
	memset(IN_LDPTAB[indx].pathname, 0x0, IN_PATHNMMX);
	memset(IN_LDPTAB[indx].ofc_pathname, 0x0, IN_OPATHNMMX);
	memset(IN_LDPTAB[indx].ext_pathname, 0x0, IN_EPATHNMMX);
	IN_LDPTAB[indx].run_lvl = 0;
	IN_LDPTAB[indx].priority = IN_procdata->default_priority;
	IN_LDPTAB[indx].uid = 0;
	IN_LDPTAB[indx].source = IN_SOFT;
	IN_LDPTAB[indx].softchk = IN_ALWSOFTCHK;
	IN_LDPTAB[indx].crerror_inh = FALSE;
	IN_LDPTAB[indx].proc_category = IN_NON_CRITICAL;
	IN_LDPTAB[indx].error_threshold = IN_procdata->default_error_threshold;
	IN_LDPTAB[indx].error_dec_rate = IN_procdata->default_error_dec_rate;
	IN_LDPTAB[indx].init_complete_timer = IN_procdata->default_init_complete_timer;
	IN_LDPTAB[indx].procinit_timer = IN_procdata->default_procinit_timer;
	IN_LDPTAB[indx].failed_init = FALSE;
	if(IN_LDPTAB[indx].msgh_qid >= 0){
		INqInUse[IN_LDPTAB[indx].msgh_qid] = 0;
	}
	IN_LDPTAB[indx].msgh_qid = -1;
	IN_LDPTAB[indx].realQ = MHnullQ;
	for(i = 0; i < INmaxgQids; i++){
		IN_LDPTAB[indx].gqid[i] = MHnullQ;
	}
	IN_LDPTAB[indx].gqCnt = 0;
	IN_LDPTAB[indx].print_progress = TRUE;
	IN_LDPTAB[indx].q_size = IN_procdata->default_q_size;
	IN_LDPTAB[indx].lv3_timer = IN_procdata->default_lv3_timer;
	IN_LDPTAB[indx].global_queue_timer = IN_procdata->default_global_queue_timer;
  IN_LDPTAB[indx].brevity_low = IN_procdata->default_brevity_low;
  IN_LDPTAB[indx].brevity_high = IN_procdata->default_brevity_high;
  IN_LDPTAB[indx].brevity_interval = IN_procdata->default_brevity_interval;
  IN_LDPTAB[indx].msg_limit = IN_procdata->default_msg_limit;
  IN_LDPTAB[indx].third_party = FALSE;
  IN_LDPTAB[indx].ps = -1;
	IN_LDPTAB[indx].oamleadonly = FALSE;
	IN_LDPTAB[indx].active_vhost_only = FALSE;
	IN_LDPTAB[indx].on_active = FALSE;
}

/*
** NAME:
**	INdef_parm_init()
**
** DESCRIPTION:
**	This routine initializes shared memory process table data to
**	an "unused state".
**
** INPUTS:
**	parm_ptr - pointer to process initlist controlled parameter structure.
**	sys_parms - pointer to system parameters, containing default values.
**
** RETURNS:
**	None
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
** 	Initialize a process parameters structure to default values.
*/
Void
INdef_parm_init(IN_PROC_PARMS * parm_ptr,IN_SYS_PARMS *sys_parms)
{
	parm_ptr->inhibit_restart = IN_ALWRESTART;
	parm_ptr->msgh_name[0] = '\0';
	parm_ptr->path[0] = '\0';
	parm_ptr->ofc_path[0] = '\0';
	parm_ptr->ext_path[0] = '\0';
	parm_ptr->user_id = 0;
	parm_ptr->group_id = -1;
	parm_ptr->run_lvl = 0;
	parm_ptr->crerror_inh = FALSE;
	parm_ptr->proc_category = IN_NON_CRITICAL;
	parm_ptr->error_threshold = sys_parms->default_error_threshold;
	parm_ptr->error_dec_rate = sys_parms->default_error_dec_rate;
	parm_ptr->init_complete_timer = sys_parms->default_init_complete_timer;
	parm_ptr->procinit_timer = sys_parms->default_procinit_timer;
	parm_ptr->create_timer = sys_parms->default_create_timer;
	parm_ptr->restart_threshold = sys_parms->default_restart_threshold;
	parm_ptr->priority = sys_parms->default_priority;
	parm_ptr->sanity_timer = sys_parms->default_sanity_timer;
	parm_ptr->restart_interval = sys_parms->default_restart_interval;
	parm_ptr->inh_softchk = IN_ALWSOFTCHK;
	parm_ptr->msgh_qid = -1;
	parm_ptr->q_size = sys_parms->default_q_size;
	parm_ptr->global_queue_timer = sys_parms->default_global_queue_timer;
	parm_ptr->brevity_low = sys_parms->default_brevity_low;
	parm_ptr->brevity_high = sys_parms->default_brevity_high;
	parm_ptr->brevity_interval = sys_parms->default_brevity_interval;
	parm_ptr->msg_limit = sys_parms->default_msg_limit;
	parm_ptr->third_party = FALSE;
	parm_ptr->isRT = IN_NOTRT;
	parm_ptr->ps = -1;
	parm_ptr->oamleadonly = FALSE;
	parm_ptr->active_vhost_only = FALSE;
	parm_ptr->on_active = FALSE;
}

#define INW_ANY_PID	(pid_t)(-1)
/*
** NAME:
**	INsigcld()
**
** DESCRIPTION:
**	This routine traps "SIGCLD" signals.  It is here only
**	to break out of message waiting loop.
**
** INPUTS:
**
** RETURNS:
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
*/

Void
INsigcld(int sig)
{
	//INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INsigcld(): death of child signal %d caught", sig));
  printf("INsigcld(): death of child signal %d caught\n", sig);
	if(INcmd){
		int	pstatus;	
		waitpid(INW_ANY_PID,&pstatus, WNOHANG);
	}
}

/*
** NAME:
**	INcheck_zombie
**
** DESCRIPTION:
**	This routine waits for all processess that might be dead
**	and it updates init request data if they died for a specific
**	reason. i.e. bad shared memory.
**
** INPUTS:
**
** RETURNS:
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
*/
pid_t
INcheck_zombie()
{

	register pid_t	dead_pid;	/* Process ID of the dead child */
	Bool	bad_shm = FALSE;	/* Incompatible shared memory 	*/
	register IN_PROCESS_DATA *ldp, *eldp;
	int	indx = IN_SNPRCMX;

	int	pstatus;	/* Status of process */

	if ((dead_pid = waitpid(INW_ANY_PID,&pstatus, WNOHANG)) > 0) {
		//INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INcheck_zombie(): death of child proc PID %d", dead_pid));
    printf("INcheck_zombie(): death of child proc PID %d\n", dead_pid);
		// Find this process pid in the process table
		eldp = &IN_LDPTAB[IN_SNPRCMX];
		for(ldp = IN_LDPTAB; ldp < eldp; ldp++){
			if(ldp->pid == dead_pid){
				indx = (int)(ldp - IN_LDPTAB);
				break;
			}
		}
		if(indx != IN_SNPRCMX && IN_LDPTAB[indx].third_party){
			IN_LDPTAB[indx].pid = (pid_t) (-1);
		}
		/*
		 * Determine the status of the returned process:
		 */
		if (WIFSTOPPED(pstatus)) {
			if(indx != IN_SNPRCMX){
				//CR_PRM(POA_INF,"REPT INIT %s STOPPED BY SIGNAL %d", IN_LDPTAB[indx].proctag, WSTOPSIG(pstatus));
        printf("REPT INIT %s STOPPED BY SIGNAL %d\n",
               IN_LDPTAB[indx].proctag, WSTOPSIG(pstatus));
			}
			//INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"\tprocess was stopped by signal %d", WSTOPSIG(pstatus)));
      printf("\tprocess was stopped by signal %d\n", WSTOPSIG(pstatus));
		}
		else if (WIFEXITED(pstatus)) {
			if(indx != IN_SNPRCMX){
				if(!IN_LDPTAB[indx].third_party){
					//CR_PRM(POA_INF,"REPT INIT %s EXIT CODE %d", IN_LDPTAB[indx].proctag, WEXITSTATUS(pstatus));
          printf("REPT INIT %s EXIT CODE %d\n",
                 IN_LDPTAB[indx].proctag, WEXITSTATUS(pstatus));
				} else {
					IN_SDPTAB[indx].ret = WEXITSTATUS(pstatus);
					return(dead_pid);
				}
			}
			//INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"\tprocess exited, exit code %d",WEXITSTATUS(pstatus)));
      printf("\tprocess exited, exit code %d",WEXITSTATUS(pstatus));
			if(WEXITSTATUS(pstatus) == (int)IN_SHM_ERR){
				bad_shm = TRUE;
			}
		}
		else if (WIFSIGNALED(pstatus)) {
			if(indx != IN_SNPRCMX && IN_LDPTAB[indx].print_progress){
				//CR_PRM(POA_INF,"REPT INIT %s RECEIVED SIGNAL %d", IN_LDPTAB[indx].proctag, WTERMSIG(pstatus));
        printf("REPT INIT %s RECEIVED SIGNAL %d\n",
               IN_LDPTAB[indx].proctag, WTERMSIG(pstatus));
			}
			if (WCOREDUMP(pstatus)) {
				//INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"\tprocess terminated due to signal %d, core file was created",WTERMSIG(pstatus)));
        printf("\tprocess terminated due to signal %d, core file was created",
               WTERMSIG(pstatus));
			}
			else {
				//INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"\tprocess terminated due to signal %d",WTERMSIG(pstatus)));
        printf("\tprocess terminated due to signal %d\n",
               WTERMSIG(pstatus));
      }
			if(indx != IN_SNPRCMX && IN_LDPTAB[indx].third_party){
				IN_SDPTAB[indx].ret = GLfail;
				return(dead_pid);
			}
		}
		else {
			//INIT_ERROR(("Error process status 0x%x",pstatus));
      printf("Error process status 0x%x\n",pstatus);
		}
	}
			
	else {
		//INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INcheck_zombie(): no PID returned from waitpid()"));
    printf("INcheck_zombie(): no PID returned from waitpid()\n");
	}

	/* If process died because of bad shared memory,
	** find it's pid and update it's reason for death.
	*/
	if(bad_shm == TRUE && indx != IN_SNPRCMX){
		if(IN_SDPTAB[indx].ireq_lvl < SN_LV1){
      IN_SDPTAB[indx].ireq_lvl = SN_LV1;
		}
		IN_SDPTAB[indx].ecode = INBADSHMEM;
	}

	return(dead_pid);
}


/*
** NAME:
**	INsigterm
**
** DESCRIPTION:
**	This routine handles SIGTERM signals received by INIT.  This
**	signal is received when the UNIX run level is changed to a target
**	level at which INIT is not running and, therefore, requires that
**	we "clean up" and exit.
**
** INPUTS:
**
** RETURNS:
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
*/
Void
INsigterm(int, siginfo_t* info, void *)
{
	if(INbase_thread != thr_self()){
		// Ignore if this is not main thread
		return;
	}
	(Void) signal(SIGTERM,SIG_IGN);
	(Void) signal(SIGALRM,SIG_IGN);
	(Void) signal(SIGCLD,SIG_IGN);

	if(info != NULL){
		if(info->si_pid != ETC_INIT_PID){
			// Do ps -ef and dump the output to a file
      printf("REPT INIT SIGTERM RECEIVED FROM PID %d - IGNORED", info->si_pid);
      return;
    } else {
			//CR_PRM(INadjustAlarm(POA_CRIT), "REPT INIT SIGTERM RECEIVED FROM PID %d - INIT EXITING", info->si_pid);
      printf("REPT INIT SIGTERM RECEIVED FROM PID %d - INIT EXITING\n",
             info->si_pid);
		}
	} else {
		//CR_X733PRM(POA_TMN, "SIGTERM NO INFO", processingErrorAlarm, softwareError,
    //           NULL, ";210", "REPT INIT SIGTERM RECEIVED NO SENDER INFO - IGNORED");
    printf("REPT INIT SIGTERM RECEIVED NO SENDER INFO - IGNORED\n");
		return;
	}
	/* IN_procdata could be uninitialized when SIGTERM received */
	if(IN_procdata != (IN_PROCDATA *)0){
		IN_procdata->pid = IN_FREEPID;
	}
  exit(0);
}

/*
** NAME:
**	INsigint
**
** DESCRIPTION:
**	This routine handles SIGINT signals received by INIT to insure
**	that INIT does not die.  However this should not occur and INIT
**	will generate a message.
**
** INPUTS:
**
** RETURNS:
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
*/

Void
INsigint(int)
{
	if(!INcmd){
		//CR_PRM(INadjustAlarm(POA_MAJ),"REPT INIT SIGINT RECEIVED");
    printf("REPT INIT SIGINT RECEIVED\n");
		IN_LDEXIT = TRUE;
	}
}

/*
** NAME:
**	INsigalrm
**
** DESCRIPTION:
**	This routine handles SIGALRM signals received by INIT to insure
**	that INIT does not die.  However this should not occur and INIT
**	will generate a message.
**
** INPUTS:
**
** RETURNS:
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
*/

Void
INsigalrm()
{
	//CR_PRM(INadjustAlarm(POA_MAJ),"REPT INIT SIGALRM RECEIVED");
  printf("REPT INIT SIGALRM RECEIVED\n");
}

/*
** NAME:
**	INsigsu
**
** DESCRIPTION:
**	This routine handles SIGUSR2 signals received by INIT that
**	are sent to INIT for SU.
**
** INPUTS:
**
** RETURNS:
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS: Update INsuexit to cause INIT to return from main loop
*/

Void
INsigsu(int)
{
	if(INbase_thread != thr_self()){
		// Ignore if this is not main thread
		return;
	}
	IN_LDEXIT = TRUE;
}

/*
** NAME:
**	INsigsu
**
** DESCRIPTION:
**	This routine handles SIGUSR2 signals received by INIT that
**	are sent to INIT to go offline.
**
** INPUTS:
**
** RETURNS:
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS: Transition the node offline 
*/

Void
INsigsnstop(int, siginfo_t* info, void *)
{
	if(INbase_thread != thr_self()){
		// Ignore if this is not main thread
		return;
	}
	IN_LDEXIT = TRUE;
	INsnstop = TRUE;
	//CR_PRM(POA_INF,"REPT INIT GOING OFFLINE, REQUESTING PID %d", info->si_pid);
  printf("REPT INIT GOING OFFLINE, REQUESTING PID %d\n", info->si_pid);
}

MHenvType	INenv;
char		INmyPeerHostName[MHmaxNameLen + 1];
Short		INmyPeerHostId = -1;
Bool		INcanBeOamLead;

#define		INmaxMSGHtries	30


/*
** NAME:
**	INprocinit()
**
** DESCRIPTION:
**	This function handles INIT's "process initialiation".  In effect,
**	INIT itself has to perform some initialization actions in step
**	with the rest of the system because it depends on MSGH
**
** INPUTS:
**	None
**
** RETURNS:
**	None
**
** CALLS:
**	MHinfoExt::attach()	- Attach to MSGH shared memory
**	MHinfoExt::regName()	- Register INIT's queue name
**
** CALLED BY:
**	INmain()		- Called when INIT restarts and must perform
**				  these functions to get back in step
**	INsequence()		- Called when MSGH has been initialized
**
** SIDE EFFECTS:
**	If this function fails to attach to MSGH's shared memory or to
**	register INIT's name with MSGH it will cause a system reset and,
**	in the most severe case, force a UNIX boot which effectively
**	causes INIT to "exit()".
*/

Void
INprocinit()
{
	GLretVal	ret;
	static int	tries = 0;

	//INIT_DEBUG((IN_DEBUG | IN_RSTRTR | IN_PROCITR),(POA_INF,"INprocinit() entered..."));
  printf("INprocinit() entered...\n");

	if((IN_LDMSGHINDX < 0) || (IN_SDPTAB[IN_LDMSGHINDX].procstep < IN_PROCINIT)){
		//INIT_DEBUG((IN_DEBUG | IN_RSTRTR | IN_PROCITR),(POA_INF,"INprocinit() returned without registering queue name \"%s\"...",IN_MSGHQNM));
    printf("INmain::INprocinit() IN_SDPTAB[%d].procstep=%d\n", IN_LDMSGHINDX, IN_SDPTAB[IN_LDMSGHINDX]);
    printf("INprocinit() returned without registering queue name \"%s\"...\n",
           IN_MSGHQNM);
		return;
	}

	IN_SLEEPN(0,50000000);

	ret = INevent.attach();
	if (ret != GLsuccess && ret != MHexist) {
		INescalate(SN_LV4,ret,IN_SOFT,INIT_INDEX);
	}

	tries++;

	MHqid msghqid;
	if((ret = INevent.getMhqid("MSGH", msghqid)) != GLsuccess){
		if(tries > INmaxMSGHtries){
			//CRERROR("INprocinit() returned without registering queue name, MSGH still not there \"%s\"...",IN_MSGHQNM);
      printf("INprocinit() returned without registering queue name, MSGH still not there \"%s\"...\n",
             IN_MSGHQNM);
			INescalate(SN_LV4,ret,IN_SOFT,INIT_INDEX);
		}
		return;
	}

	// Make sure MSGH is on this host
	if(INevent.Qid2Host(msghqid) != INevent.getLocalHostIndex()){
		if(tries > INmaxMSGHtries){
			// Found MSGH on another machine, 
			//CRERROR("INprocinit() returned without registering queue name, MSGH on another machine msghqid %s", msghqid.display());
      printf("INprocinit() returned without registering queue name, MSGH on another machine msghqid %s\n",
             msghqid.display());
			INescalate(SN_LV4,ret,IN_SOFT,INIT_INDEX);
		}
		return;
	}

	if(tries > 2){
		//CR_PRM(POA_INF,"REPT INIT MSGH REGISTRATION TRIED %d TIMES", tries);
    printf("REPT INIT MSGH REGISTRATION TRIED %d TIMES\n",
           tries);
	}

	// Check if I can be oam Lead and set my host id (adjusted for mixed cluster)
	// If I can be OAM lead, set the OAM lead timer

	MHenvType       oldEnv;  
	INevent.getEnvType(oldEnv);
	INevent.setPreferredEnv(MH_peerCluster);
	INmyPeerHostId = INevent.getLocalHostIndex();
	INevent.getMyHostName(INmyPeerHostName);
	INevent.setPreferredEnv(oldEnv);

	INcanBeOamLead = FALSE;
	for(int j = 0; j < INmaxResourceGroups; j++){
		if(strcmp(INmyPeerHostName, IN_procdata->oam_lead[j]) == 0){
			INcanBeOamLead = TRUE;
			break;
		}
	}
	if(INcanBeOamLead){
		memcpy(IN_procdata->vhost, IN_procdata->oam_lead, sizeof(IN_procdata->vhost));
		
	}

	//INIT_DEBUG((IN_DEBUG | IN_PROCITR),(POA_INF,"peerHostName %s, peerHostId %d, canBeOamLead %d",
  //                                    INmyPeerHostName, INmyPeerHostId, INcanBeOamLead));
  printf("peerHostName %s, peerHostId %d, canBeOamLead %d\n",
         INmyPeerHostName, INmyPeerHostId, INcanBeOamLead);

	MHregisterTyp	regType;

	if(IN_LDCURSTATE == S_OFFLINE || IN_LDCURSTATE == S_INIT){
		regType = MH_LOCAL;
	} else {
		regType = MH_GLOBAL;
	}
	
	ret = INevent.regName((const Char *)IN_MSGHQNM, INmsgqid, FALSE, FALSE, FALSE, regType);
	if (ret != GLsuccess) {
		INescalate(SN_LV4,ret,IN_SOFT,INIT_INDEX);
	}

	// Increase INIT q parameters
	INevent.setQueLimits(INmsgqid, 300, 300000);

	/* Enable message reception for "EHhandler::getEvent()" call: */
	INetype = EHBOTH;
	if(IN_procdata->vhost[0][0] != 0){
		char name[20];
		INevent.getMyHostName(name);
		if(strcmp(name, IN_procdata->vhost[0]) == 0){
			INvhostMate = 1;
		} else {
			INvhostMate = 0;
		}
		sprintf(INvhostMateName, "%s:INIT", IN_procdata->vhost[INvhostMate]);
	}
	if(IN_LDCURSTATE != S_OFFLINE && IN_LDCURSTATE != S_UNAV &&
     IN_LDSTATE.initstate == INITING){
		//FTbladeStChgMsg msg(S_INIT, INITING, IN_LDSTATE.softchk);
		//if(INvhostMate >= 0){
		//	msg.setVhostMate(IN_procdata->vhost[INvhostMate]);
		//	msg.setVhostState(INstandby);
		//}
		//msg.send();
//		FTbladeStQryMsg query;
//		query.send(INmsgqid);
	}
  // Create mutex audit thread
  if(thr_create(NULL,thr_min_stack()+IN_MIN_STACK, INmhMutexCheck,NULL,THR_BOUND,NULL) != 0){
    //INIT_ERROR(("can't create mutex audit thread, errno %d",errno));
    printf("can't create mutex audit thread, errno %d\n",errno);
		INescalate(SN_LV4,errno,IN_SOFT,INIT_INDEX);
  }       

	// Initialize node status info if necessary
	char	oldState = IN_LDCURSTATE;
	IN_MYNODEID = INevent.getLocalHostIndex();
	Bool 		isUsed;
	Bool		isActive;

	INevent.getEnvType(INenv);
	
	// Initialize all the node states if shared memory did not exist before
	// or was never configured.  This should only be true after a reboot
	// so change all the node states to INIT 
	if(IN_LDCURSTATE == -1){
		int i;
		for(i = 0; i < MHmaxHostReg; i++){
			INevent.Status(i, isUsed, isActive);
			if(!isUsed){
				IN_NODE_STATE[i] = S_OFFLINE;
			} else {
				IN_NODE_STATE[i] = S_INIT;
			}
		}
	}

	IN_LDCURSTATE = oldState;

	if(IN_LDCURSTATE == S_LEADACT){
		INevent.setLeadCC(INmsgqid);
		if(!IN_LDSTATE.issimplex){
			INSETTMR(INsetLeadTmr,INSETLEADTMR, INSETLEADTAG, TRUE);
		}
	} else {
		INSETTMR(INcheckleadtmr,INCHECKLEADTMR, INCHECKLEADTAG, TRUE);
		if(!IN_LDSTATE.issimplex){
			/* Duplex system without watchdog	*/
			INchecklead();
			INSETTMR(INsetLeadTmr,INSETLEADTMR, INSETLEADTAG, TRUE);
		}
	}

	if(INvhostMate >= 0 && IN_LDCURSTATE != S_OFFLINE && (!INevent.isVhostActive())){
		INSETTMR(INsetActiveVhostTmr, IN_procdata->vhostfailover_time, INSETACTIVEVHOSTTAG, TRUE);
	}

	//INIT_DEBUG((IN_DEBUG | IN_RSTRTR | IN_PROCITR),(POA_INF,"INprocinit() returned successfully after registering queue name \"%s\"...",IN_MSGHQNM));
  printf("INprocinit() returned successfully after registering queue name \"%s\"...\n",
         IN_MSGHQNM);

}

/*
** NAME:
**	INdump()
**
** DESCRIPTION:
**	This routine performs a dump of INIT's shared memory data
**	default state.
**
** INPUTS:
**	Char* msgh_name	// if NULL prints brief info about all processes
**			// if not NULL prints detailed info about the process with the name "msgh_name"
**
** RETURNS:
**	None
**
** CALLS:
**	INprtDetailProcInfo(Char* msgh_name);
**	INprtGeneralProcInfos(Void);
**
** CALLED BY:
**
** SIDE EFFECTS:
*/

Void
INdump(Char* msgh_name)
{
	if (IN_LDSHM_VER != IN_SHM_VER) {
		fprintf(stderr, "Incompatible shared memory. Expected 0x%lx, found 0x%lx\n",IN_SHM_VER,IN_LDSHM_VER);
		exit(0);
	}

	if ((char *) msgh_name) {
		INprtDetailProcInfo(msgh_name);
	} else {
		INprtGeneralProcInfos();
	}
}


/*
** NAME:
**      INprintHistory()
**
** DESCRIPTION:
**      Prints to stdout the history associated with the INIT process
**      i.e. contents of IN_LDINFO
**
** INPUTS:
**
** RETURNS:
**      None
**
** CALLS:
**
** CALLED BY: INprtGeneralProcInfos()
**
** SIDE EFFECTS:
*/

Void
INprintHistory(Void)
{
	Long indx;

	printf("System reset history:\n");

	for (indx = IN_LDINFO.uld_indx; indx != IN_LDINFO.ld_indx;
	     indx = IN_NXTINDX(indx, IN_NUMINITS)) {

		time_t str_time = (time_t)IN_LDINFO.init_data[indx].str_time;
		time_t end_time = (time_t)IN_LDINFO.init_data[indx].end_time;

		printf("START: %.24s\tFINISH: %.24s\t\n%s, runlvl %2d, Last init level %s requested by %s, err_code %d\n",
           asctime(localtime(&str_time)),
           asctime(localtime(&end_time)),
           IN_SNLVLNM(IN_LDINFO.init_data[indx].psn_lvl),
           IN_LDINFO.init_data[indx].prun_lvl,
           IN_SNSRCNM(IN_LDINFO.init_data[indx].source),
           IN_LDINFO.init_data[indx].msgh_name,
           IN_LDINFO.init_data[indx].ecode);
	}

  printf("procs %d,  deaths %d, softchk %s, crerror_inh %d \n",
         INcountprocs(),IN_LDSTATE.init_deaths,
         ((IN_LDSTATE.softchk == IN_INHSOFTCHK)?"IN_INHSOFTCHK":
          ((IN_LDSTATE.softchk == IN_ALWSOFTCHK)?"IN_ALWSOFTCHK":
           "Invalid softchk state")), IN_LDSTATE.crerror_inh);

  printf("isactive = %d, newstate = %c, wdstate %c, final_runlvl = %d, issimplex = %d\n",
         IN_LDSTATE.isactive, IN_LDCURSTATE, IN_LDWDSTATE, IN_LDSTATE.final_runlvl, IN_LDSTATE.issimplex);

  printf("INIT: state %s, step %s, run_lvl %d, sync_run_lvl %d, sn_lvl %s\n",
         IN_STATENM(IN_LDSTATE.initstate), IN_SQSTEPNM(IN_LDSTATE.systep),
         IN_LDSTATE.run_lvl,IN_LDSTATE.sync_run_lvl,IN_SNLVLNM(IN_LDSTATE.sn_lvl));

  printf("INIT: sys_err_cnt %d, pid %d version 0x%lx q_size %ld\n\n",
         IN_SDERR_COUNT,IN_procdata->pid,IN_LDSHM_VER, IN_procdata->default_q_size);
}
		

/*
** NAME:
**      INprtDetailProcInfo(Char* msgh_name)
**
** DESCRIPTION:
**      Prints to stdout the contents of the shared memory associated
**      with the process msgh_name.
**
** INPUTS:
**	(Char* msgh_name): name of the process for which information is requested
**
** RETURNS:
**      None
**
** CALLS:
**
** CALLED BY: INprtGeneralProcInfos()
**
** SIDE EFFECTS:
*/

Void
INprtDetailProcInfo(Char* msgh_name)
{

	Long j;	
	const long numbItems = 64;	// total number of entries to be printed
	Char buf[numbItems][128];	// storage for formatting data

	Long i = INfindproc(msgh_name);

	if (i == IN_SNPRCMX) {
		fprintf(stderr, "INIT: process %s not found\n",msgh_name);
		return;
	}


	printf("INIT: shm_ver %ld, bkucl %d, bkpid %d, availsman %ld\n",
         IN_LDSHM_VER,
         IN_LDBKUCL,
         IN_LDBKPID,
         IN_LDVMEM);
 
	printf("INIT: initlist=%s, aru_intvl=%d, msgh_indx=%d\n",
         msgh_name,
         IN_LDILIST,
         IN_LDARUINT,
         IN_LDMSGHINDX);
 
	printf("INIT: bqid %s, aqid %s, subkout %d, int_exit %d\n",
         IN_LDBQID.display(),
         IN_LDAQID.display(),
         IN_LDBKOUT,
         IN_LDEXIT); 

	sprintf(buf[0], "index=%i", i);
  sprintf(buf[1], "updstate=%i", IN_LDPTAB[i].updstate);
  sprintf(buf[2], "pathname=%.35s", IN_LDPTAB[i].pathname); 
  sprintf(buf[3], "peg_intvl=%i", IN_LDPTAB[i].peg_intvl); 
  sprintf(buf[4], "error_count=%i", IN_SDPTAB[i].error_count); 
  sprintf(buf[5], "proc_ctgr=%.35s", IN_PROCCATNM(IN_LDPTAB[i].proc_category)); 
  sprintf(buf[6], "rstrt_cnt=%i", IN_LDPTAB[i].rstrt_cnt); 
  sprintf(buf[7], "crerr_inh=%.35s", (IN_LDPTAB[i].crerror_inh == TRUE ? "YES":"NO")); 
  sprintf(buf[8], "strtstate=%.35s", ((IN_LDPTAB[i].startstate == IN_INHRESTART)
                                      ? "IN_INHRESTART"
                                      : ((IN_LDPTAB[i].startstate == IN_ALWRESTART)
                                         ? "IN_ALWRESTART"
                                         : "Invalid restart state"
                                        )
            ));

  sprintf(buf[9], "tot_rstrt=%i", IN_LDPTAB[i].tot_rstrt);
  sprintf(buf[10], "failed_init=%i", (int)IN_LDPTAB[i].failed_init);        
  sprintf(buf[11], "softchk=%.35s", ((IN_LDPTAB[i].softchk == IN_INHSOFTCHK)
                                     ? "IN_INHSOFTCHK"
                                     : ((IN_LDPTAB[i].softchk == IN_ALWSOFTCHK)
                                        ? "IN_ALWSOFTCHK"
                                        : "Invalid softchk state"
                                       )
            ));

  sprintf(buf[12], "next_rstrt=%i", (int)IN_LDPTAB[i].next_rstrt);        
  sprintf(buf[13], "rstrt_max=%i", IN_LDPTAB[i].rstrt_max);        
  sprintf(buf[14], "permstate=%.35s", IN_PSTATENM(IN_LDPTAB[i].permstate));        
  sprintf(buf[15], "count=%i", IN_SDPTAB[i].count);        
  sprintf(buf[16], "creat_tmr=%i", IN_LDPTAB[i].create_timer);        
  sprintf(buf[17], "err_threshold=%i", IN_LDPTAB[i].error_threshold);        
  sprintf(buf[18], "startime=%li", IN_LDPTAB[i].startime);        
  sprintf(buf[19], "sn_lvl=%.35s", IN_SNLVLNM(IN_LDPTAB[i].sn_lvl));        
  sprintf(buf[20], "init_complete_tmr=%i", IN_LDPTAB[i].init_complete_timer);        
	sprintf(buf[21], "priority=%i", IN_LDPTAB[i].priority);
  sprintf(buf[22], "syncstep=%.35s", IN_SQSTEPNM(IN_LDPTAB[i].syncstep));
  sprintf(buf[23], "sent_missedsan=%i", IN_LDPTAB[i].sent_missedsan);
  sprintf(buf[24], "msgh_qid=%i", IN_LDPTAB[i].msgh_qid);
  sprintf(buf[25], "time_missedsan=%li", IN_LDPTAB[i].time_missedsan);
  sprintf(buf[26], "procstate=%.35s", IN_PROCSTNM(IN_SDPTAB[i].procstate));
  sprintf(buf[27], "sigtype=%i", IN_LDPTAB[i].sigtype);
  sprintf(buf[28], "last_count=%i", IN_LDPTAB[i].last_count);
  sprintf(buf[29], "procstep=%.35s", IN_SQSTEPNM(IN_SDPTAB[i].procstep));
	sprintf(buf[30], "pid=%i", IN_LDPTAB[i].pid);
  sprintf(buf[31], "progress_check=%i", IN_SDPTAB[i].progress_check);
  sprintf(buf[32], "rstrt_intvl=%i", IN_LDPTAB[i].rstrt_intvl);
	sprintf(buf[33], "uid=%i", IN_LDPTAB[i].uid);
  sprintf(buf[34], "prt_prog=%i", IN_LDPTAB[i].print_progress);
  sprintf(buf[35], "ireq_lvl=%.35s", IN_SNLVLNM(IN_SDPTAB[i].ireq_lvl));

  sprintf(buf[36], "run_lvl=%i", IN_LDPTAB[i].run_lvl);
  sprintf(buf[37], "proctag=%.35s", IN_LDPTAB[i].proctag);
  sprintf(buf[38], "error_dec_rate=%i", IN_LDPTAB[i].error_dec_rate);
  sprintf(buf[39], "progress_mark=%li", IN_SDPTAB[i].progress_mark);
  sprintf(buf[40], "procinit_timer=%i", IN_LDPTAB[i].procinit_timer);
	sprintf(buf[41], "ofcPthNm=%.35s", IN_LDPTAB[i].ofc_pathname);
	sprintf(buf[42], "source=%.35s", IN_SNSRCNM(IN_LDPTAB[i].source));
	sprintf(buf[43], "onActive=%.35s", IN_LDPTAB[i].on_active ? "YES" : "NO");
	sprintf(buf[44], "lv3_timer=%i", (int)IN_LDPTAB[i].lv3_timer);
  sprintf(buf[45], "globQtimer=%i", (int)IN_LDPTAB[i].global_queue_timer);
  sprintf(buf[46], "gqCnt=%d", IN_LDPTAB[i].gqCnt);
	sprintf(buf[47], "realQ=%s", IN_LDPTAB[i].realQ.display());
	sprintf(buf[48], "brevity_low=%d", IN_LDPTAB[i].brevity_low);
	sprintf(buf[49], "brevity_high=%d", IN_LDPTAB[i].brevity_high);
	sprintf(buf[50], "brevity_interval=%d", IN_LDPTAB[i].brevity_interval);
	sprintf(buf[51], "msg_limit=%d", IN_LDPTAB[i].msg_limit);
	sprintf(buf[52], "ext_path=%s", IN_LDPTAB[i].ext_pathname);
	sprintf(buf[53], "q_size=%d", IN_LDPTAB[i].q_size);
	sprintf(buf[54], "gqid[0]=%s", IN_LDPTAB[i].gqid[0].display());
	sprintf(buf[55], "gqid[1]=%s", IN_LDPTAB[i].gqid[1].display());
	sprintf(buf[56], "gqid[2]=%s", IN_LDPTAB[i].gqid[2].display());
	sprintf(buf[57], "realQ=%s", IN_LDPTAB[i].realQ.display());
	sprintf(buf[58], "third_party=%.35s", IN_LDPTAB[i].third_party ? "YES" : "NO");
	sprintf(buf[59], "tid=%i", IN_LDPTAB[i].tid);
	sprintf(buf[60], "alvl=%i", IN_SDPTAB[i].alvl);
	sprintf(buf[61], "rt=%d", IN_LDPTAB[i].isRT);
	sprintf(buf[62], "ps=%d", IN_LDPTAB[i].ps);
	sprintf(buf[63], "oamleadonly=%.35s", IN_LDPTAB[i].oamleadonly ? "YES" : "NO");
	/* change "const long numbItems" before adding new elements */

	for (j=0; j < 3*(numbItems/3); j+=3) {
		printf("%-15s %-25s %-35s\n", buf[j], buf[j+1], buf[j+2]);
	}

	/* print the rest */
	for (; j < numbItems; j++) printf("%-15s ", buf[j]);
	printf("\n");

	printf("%3s) %-11s %-10s %-12s\n", "No", "shmids", "shm_rel", "shm_pkey");

	for (j = 0; j < INmaxSegs; j++) {
		if(IN_SDSHMDATA[j].m_pIndex != i){
			continue;
		}
		printf("%3i) %-11i %-10s %-11i\n", j, 
           IN_SDSHMDATA[j].m_shmid,
           (IN_SDSHMDATA[j].m_rel ? "True" : "False"), 
           IN_SDSHMDATA[j].m_pkey);
	}

}

/*
** NAME:
**     INprtGeneralProcInfos(Void) 
**
** DESCRIPTION:
**      Prints to stdout the contents of the shared memory 
**      of all processes and the history of INIT
**
** INPUTS:
**      
**
** RETURNS:
**      None
**
** CALLS:
**	INprintHistory()
**
** CALLED BY: INdump()
**
** SIDE EFFECTS:
*/

Void
INprtGeneralProcInfos(Void)
{
	Long i;

	INprintHistory();
	
	printf(	"A) %-10.10s %-10.10s %-14.14s %-14.14s %s\n"
          "B) %-10.10s %-10.10s %-14.14s %-14.14s %s\n"
          "C) %-10.10s %-10.10s %-14.14s %-14.14s %s\n"
          "D) %-10.10s %-10.10s %-14.14s %-14.14s %s\n"
          "E) %-10.10s %-10.10s %-14.14s %-14.14s %s\n"
          "F) %-10.10s %-10.10s %-14.14s %-14.14s %s\n"
          "G) %-10.10s %-10.10s %-14.14s %-14.14s %s\n",

          "MSGH name",	"runlvl",	"PID",		"procstate",	"path name",
          "rstrt-intvl",	"rstrt-cnt","rstrt-tot",	"permstate",	"startstate",
          "crerror_inh",	"sanity-intvl",	"procstep",	"syncstep",	"proc_cat",
          "nxt_rstrt",	"rstrt_max",	"qsize",	"softchk",	"e_(thr:drate:cnt)",
          "faild_init",	"sn_lvl",	"ireq_lvl",	"icmptmr",	"onact",
          "progs_mrk","progs_chk","san_count",   "prio",		"ptmr",
          "brev_low", "brev_high","brev_intvl","msg_limit", "ext_name");


	for (i=0; i < IN_SNPRCMX; i++) {

		if (!IN_VALIDPROC(i)) continue;	

		printf("Proc indx %i\n", i);

		printf( "A) %-10s %-10i %-14i %-14s %s\n"
            "B) %-10i %-10i %-14i %-14s %s\n"
            "C) %-10s %-10i %-14s %-14s %s\n"
            "D) %-10i %-10i %-14i %-14s %i:%i:%i\n"
            "E) %-10i %-10s %-14s %-14i %i\n"
            "F) %-10i %-10i %-14i %-14i %i\n"
            "G) %-10i %-10i %-14i %-14i %s\n",

            IN_LDPTAB[i].proctag, 
            IN_LDPTAB[i].run_lvl, 
            IN_LDPTAB[i].pid, 
            IN_PROCSTNM(IN_SDPTAB[i].procstate), 
            IN_LDPTAB[i].pathname,

            IN_LDPTAB[i].rstrt_intvl, 
            IN_LDPTAB[i].rstrt_cnt, 
            IN_LDPTAB[i].tot_rstrt,  
            IN_PSTATENM(IN_LDPTAB[i].permstate),
            ((IN_LDPTAB[i].startstate == IN_INHRESTART)
             ? "IN_INHRESTART" 
             : ((IN_LDPTAB[i].startstate == IN_ALWRESTART)
                ? "IN_ALWRESTART"
                : "Invalid restart state")),

            (IN_LDPTAB[i].crerror_inh == TRUE ? "YES":"NO"), 
            IN_LDPTAB[i].peg_intvl, 
            IN_SQSTEPNM(IN_SDPTAB[i].procstep), 
            IN_SQSTEPNM(IN_LDPTAB[i].syncstep), 
            IN_PROCCATNM(IN_LDPTAB[i].proc_category),

            IN_LDPTAB[i].next_rstrt, 
            IN_LDPTAB[i].rstrt_max, 
            IN_LDPTAB[i].q_size,
            ((IN_LDPTAB[i].softchk == IN_INHSOFTCHK) 
             ? "IN_INHSOFTCHK"
             : ((IN_LDPTAB[i].softchk == IN_ALWSOFTCHK) 
                ? "IN_ALWSOFTCHK"
                : "Invalid softchk state")),
            IN_LDPTAB[i].error_threshold, 
            IN_LDPTAB[i].error_dec_rate, 
            IN_SDPTAB[i].error_count, 

            (int)IN_LDPTAB[i].failed_init, 
            IN_SNLVLNM(IN_LDPTAB[i].sn_lvl), 
            IN_SNLVLNM(IN_SDPTAB[i].ireq_lvl), 
            IN_LDPTAB[i].init_complete_timer, 
            IN_LDPTAB[i].on_active,

            IN_SDPTAB[i].progress_mark, 
            IN_SDPTAB[i].progress_check, 
            IN_SDPTAB[i].count, 
            IN_LDPTAB[i].priority, 
            IN_LDPTAB[i].procinit_timer,
            IN_LDPTAB[i].brevity_low,
            IN_LDPTAB[i].brevity_high,
            IN_LDPTAB[i].brevity_interval,
            IN_LDPTAB[i].msg_limit,
            IN_LDPTAB[i].ext_pathname
      );
	}

}



/*
** NAME:
**	INcountprocs()
**
** DESCRIPTION:
**	This routine counts the processes currently under INITs control
**
** INPUTS:
**
** RETURNS:
**	Number of processes
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
*/

int
INcountprocs()
{
	int num_procs = 0;
	for(int i = 0; i < IN_SNPRCMX; i++){
		if(IN_VALIDPROC(i)){
			num_procs++;
		}
	}

	return(num_procs);
}

/* Error failures	*/
#define IN0_OPEN	0x80000000
#define IN0_FCNTL	0x40000000
#define IN0_ISOPEN	0x20000000
#define IN0_OPENED	0x10000000
#define IN1_OPEN	0x08000000
#define IN1_FCNTL	0x04000000
#define IN1_ISOPEN	0x02000000
#define IN1_OPENED	0x01000000
#define IN2_FCNTL	0x00800000

#define INSETERR(err)	INmain_error |= ((err) | errno)

/*
** NAME:
**	INmain_init()
**
** DESCRIPTION:
**	Initialize environment variables.
**
**
** INPUTS:
**
** RETURNS:
**
** CALLS:
**
** CALLED BY:
**	main() 
**
** SIDE EFFECTS:
** 	Initializes environment variables.
*/
Void
INmain_init()
{
  printf("INmain::INmain_init() enter\n");
	struct stat	stbuf;
	// Record our start time
	(void)time(&INinit_start_time);
	
	INppid = getppid();
	
	if(INcmd == FALSE){
		/*
		 * Check to see if standard in, out and standard error are open,
		 * if they're not, open and assign them to /dev/null:
		 */
		int tfd;
		if ((tfd = open("/dev/null", O_RDONLY)) < 0) {
			INSETERR(IN0_OPEN);
		} else {
			if (dup2(tfd, 0) < 0) {
				INSETERR(IN0_FCNTL);
			}
			(Void)close(tfd);
		} 

		if ((tfd = open("/dev/null", O_WRONLY)) < 0) {
			INSETERR(IN1_OPEN);
		} else {
			if (dup2(tfd, 1) < 0) {
				INSETERR(IN1_FCNTL);
			}
			(Void)close(tfd);
		} 

		if ((fstat(2, &stbuf) < 0) && (fcntl(1, F_DUPFD, 2) < 0)) {
			INSETERR(IN2_FCNTL);
		}

	} else {
		INppid = 0;
	}

	char * ep;
	/* If environment variables not set change them to defaults */
	if((ep = getenv("TZ")) == 0 || strlen(ep) == 0) {
		putenv((char *)"TZ=CST6CDT");
		INnoenv_vars |= IN_NO_TZ;
	}
	if((ep = getenv("LC_TIME")) == 0 || strlen(ep) == 0) {
		putenv((char *)"LC_TIME=psp_usa_std");
		INnoenv_vars |= IN_NO_LANG;
	}

	/* Added in R23 to stop SPA malloc mutex hang on process startup */
	if((ep = getenv("LD_BIND_NOW")) == 0 || strlen(ep) == 0) {
		putenv((char *)"LD_BIND_NOW=1");
	}
  printf("INmain::INmain_init() exit\n");
}

/*
** NAME:
**	INlv4_count()
**
** DESCRIPTION:
**	This function returns the accumalated count of level 4 inits
**	saved in INinitcount[] file. This function increments the
**	count and returns the new value every time it is called,
**	unless clear_count flag is set, in which case the file will
**	be removed.
**
**
** INPUTS:
**	clear_count - if TRUE, zero level 4 count
**
** RETURNS:
**
** CALLS:
**
** CALLED BY:
**	INmain() 
**
** SIDE EFFECTS:
** 	Updates file that keeps track of number of inits.
*/
Long
INlv4_count(Bool clear_count, Bool update)
{
	int fd;
	int numread;
	long init_count;

	/* Zero the init count by removing the file */
	if(clear_count == TRUE){
		Long cur_cnt = INlv4_count(FALSE, FALSE);
		if(unlink(INinitcount) < 0 && errno != ENOENT){
			//CRERROR("Failed to unlink %s, errno %d",INinitcount,errno);
      printf("Failed to unlink %s, errno %d\n",INinitcount,errno);
		}
		if(INmax_boots > 0 && cur_cnt >= (INsys_init_threshold + INmax_boots)){
			//CR_X733PRM(POA_CLEAR, "INIT escalation", processingErrorAlarm,   
      //           applicationSubsystemFailure, "The threshold for maximum number of boots has been reached",  ";235",
      //           "REPT INIT MAXIMUM BOOT THRESHOLD CLEARED");
      printf("REPT INIT MAXIMUM BOOT THRESHOLD CLEARED\n");
		}

		return(0);
	}
	if((fd = open(INinitcount,O_CREAT | O_RDWR,0644)) < 0){
		//CRERROR("Failed to open %s, errno %d",INinitcount,errno);
    printf("Failed to open %s, errno %d\n",INinitcount,errno);
		return(0);
	}
	if((numread = read(fd,(void *)&init_count,sizeof(long))) < 0){
		//CRERROR("Failed to read %s, errno %d",INinitcount,errno);
    printf("Failed to read %s, errno %d\n",INinitcount,errno);
	} else if(numread == 0){
		/* File did not exist */
		init_count = 1;
	} else if(numread == sizeof(long)){
		/* File existed, increment init_count */
		if(update == TRUE){
			init_count++;
		}
	} else {
		//CRERROR("Invalid size of  %s, size = %d",INinitcount,numread);
    printf("Invalid size of  %s, size = %d\n",INinitcount,numread);
		if(unlink(INinitcount) < 0){
			//CRERROR("Failed to unlink %s, errno %d",INinitcount,errno);
      printf("Failed to unlink %s, errno %d\n",INinitcount,errno);
		}
		return(0);
	}

	if(lseek(fd,0L,SEEK_SET) < 0){
		//CRERROR("Failed to lseek %s, errno %d",INinitcount,errno);
    printf("Failed to lseek %s, errno %d\n",INinitcount,errno);
	}

	if(write(fd,(char *)&init_count,sizeof(long)) != sizeof(long)){
		//CRERROR("Failed to write %s, errno %d",INinitcount,errno);
    printf("Failed to write %s, errno %d\n",INinitcount,errno);
	}
	
	close(fd);
	return(init_count);
}

#define IN_NUM_VARS	3	/* Number of kernel variables mmaped */

/*
** NAME:
**	INkernel_map()
**
** DESCRIPTION:
**	This function obtains addresses of kernel variables that 
**	INIT process needs to access. It also mmap()'s those variables
**	for efficient access.
**
**
** INPUTS:
**
** RETURNS:
**	GLfail 	- could not get access to the kernel variables
**	GLsuccess - otherwise
**
** CALLS:
** 	mmap(), nlist()
**
** CALLED BY:
**	INmain() 
**
** SIDE EFFECTS:
**	Updates global variables with the mmap()'ed kernel addresses. 
**	Mapped variables:
**	availsmem - keeps track of total available virtual memory (swap +
**		    core) in pages., this is only meaningful on TANDEM
**	anoninfo - structure that contains information on swap space usage
*/
GLretVal INkernel_map() {
  return(GLsuccess);
}

/*
** NAME:
**	INsanset()
**
** DESCRIPTION:
**	This function set the duration of sanity pegging that SYSMON should
**	expect.
**
** INPUTS:
**	duration - duration of the timer in miliseconds, 0 to disable
**
**
*/
Void
INsanset(Long duration)
{
	//INIT_DEBUG((IN_DEBUG|IN_SANITR),(POA_INF, "INsanset entered, duration %ld", duration));
  printf("INsanset entered, duration %ld\n", duration);
}

/*
** NAME:
**	INsanstrobe()
**
** DESCRIPTION:
**	This function informes the SYSMON driver to strobe hardware watchdog
**
** INPUTS:
**
**
*/

Void
INsanstrobe()
{
	//INIT_DEBUG((IN_DEBUG|IN_SANITR),(POA_INF, "INsanstrobe entered"));
  printf("INsanstrobe entered\n");
}

// This thread also checks the main thread sanity
// and also monitors UNIX scheduler problems

int		INinitmissed = 0;

Void *
INmhMutexCheck(void*)
{
  sigset_t new_mask;
  /* Disable all signals */
  (void)sigfillset(&new_mask);
  (void)thr_sigsetmask(SIG_BLOCK,&new_mask,NULL);

	Bool 		ret;
	int 		shmlastcount = 0;
	unsigned long 	initlastcount = 0;
	struct tms	buffer;
	clock_t		curtime;
	clock_t		lasttime;

	static	int entcnt = 0;

  // Check the MH mutex every second
  // Check shared memory allocator mutex every 4 seconds
  while(1){
		entcnt++;
    ret = INevent.audMutex();
    if(!INmhmutex_cleared){
			INmhmutex_cleared = ret;
		}
		
		if((entcnt & 0xf) == 0){
			// Run INIT shared memory audit every 8 sec
			if(shmlastcount != IN_SDSHMLOCKCNT){
				shmlastcount = IN_SDSHMLOCKCNT;
			} else if(mutex_trylock(&IN_SDSHMLOCK) == 0){
				mutex_unlock(&IN_SDSHMLOCK);
			} else {
				mutex_unlock(&IN_SDSHMLOCK);
				//CR_PRM(POA_INF, "REPT INIT UNLOCKED SHARED MEMORY MUTEX");
        printf("REPT INIT UNLOCKED SHARED MEMORY MUTEX");
			}
			if(INevent.getOAMLead() == INmyPeerHostId){
				INoamLead       oamLeadMsg;

				oamLeadMsg.srcQue = INmsgqid;
				MHmsgh.sendToAllHosts("INIT", (char*)&oamLeadMsg, sizeof(INoamLead), MH_scopeSystemOther);
			}
		}

		if(initlastcount != INsanityPeg){
			initlastcount = INsanityPeg;
			INinitmissed = 0;
		} else {
			INinitmissed++;
		}

		/* INsanityPeg value disables sanity checking during shut down */
		if(INinitmissed > INsanityTimeout && INsanityPeg != 0xffffffff){
			//CR_PRM(POA_INF, "REPT INIT ERROR FAILED SANITY PEG - EXITING");
      printf("REPT INIT ERROR FAILED SANITY PEG - EXITING\n");
			abort();
		} else if(INinitmissed > INsanityWarnTimeout){
			INwarnMissed = INinitmissed;
		}
		
		if(INvhostMate >= 0 && INevent.isVhostActive()){
			INsetActiveVhost  setActiveMsg;	
			// send unbuffered
			setActiveMsg.send(INvhostMateName, INmsgqid, sizeof(setActiveMsg), 0, FALSE);
		}

		lasttime = times(&buffer);
		IN_SLEEPN(0,500000000);
		curtime = times(&buffer);

		if((curtime > lasttime) && (curtime - lasttime > (HZ * 2))){
			INticksMissed = curtime - lasttime;
		} 
  }
	return(NULL);
}


/*
**
NAME:
**	INcheckDirSize()
**
** DESCRIPTION:
**	Checks for free space in the core directory
** INPUTS:
**
** RETURNS:
**	-2: if free space is < 10%
**	-1: if free space is < 50% and > 10%
**	 0: error opening the directory
**	 1: free space > 50%
**
** CALLS:
**
** CALLED BY:
**	INchkcore()
**
** SIDE EFFECTS:
*/
Long INcheckDirSize(Void)
{
	struct statvfs vfsInfo;
	Long ret = 1;
	if (statvfs(INexecdir, &vfsInfo)) {
		ret = 0;
		//CR_PRM(POA_INF, "REPT INIT ERROR unable to open %s", INexecdir);
    printf("REPT INIT ERROR unable to open %s\n", INexecdir);

	} else if (vfsInfo.f_blocks == 0) {
		ret = 0;
		//CR_X733PRM(INadjustAlarm(POA_MAJ)+7, "0 BLOCKS", qualityOfServiceAlarm, resourceAtOrNearingCapacity, NULL, ";217", 
    //           "REPT INIT ERROR 0 BLOCKS IN CORE DIRECTORY %s", INexecdir);
    printf("REPT INIT ERROR 0 BLOCKS IN CORE DIRECTORY %s\n", INexecdir);

	} else if (vfsInfo.f_bavail/(vfsInfo.f_blocks/100) < 100 - IN_procdata->core_full_major) {
		ret = -2;
		//CR_X733PRM(INadjustAlarm(POA_MAJ),
    //           "CORE FULL", qualityOfServiceAlarm, 
    //           resourceAtOrNearingCapacity, NULL, ";203", 
    //           "REPT INIT ERROR %s is %i%% full", 
    //           INexecdir,
    //           (vfsInfo.f_blocks - vfsInfo.f_bavail) / (vfsInfo.f_blocks/100));
    printf("REPT INIT ERROR %s is %i%% full\n", 
           INexecdir,
           (vfsInfo.f_blocks - vfsInfo.f_bavail) / (vfsInfo.f_blocks/100));
		IN_LDALMCOREFULL = POA_MAJ;

	} else if (vfsInfo.f_bavail/(vfsInfo.f_blocks/100) <  \
             100 - IN_procdata->core_full_minor) {
		ret = -1;
		//CR_X733PRM(POA_MIN,"CORE FULL", qualityOfServiceAlarm, 
    //           resourceAtOrNearingCapacity, NULL, ";203", 
    //           "REPT INIT ERROR %s is %i%% full", 
    //           INexecdir,
    //           (vfsInfo.f_blocks - vfsInfo.f_bavail) / (vfsInfo.f_blocks/100));
    printf("REPT INIT ERROR %s is %i%% full\n", 
           INexecdir,
           (vfsInfo.f_blocks - vfsInfo.f_bavail) / (vfsInfo.f_blocks/100));

		IN_LDALMCOREFULL = POA_MIN;
	} else if(IN_LDALMCOREFULL != POA_INF){
		IN_LDALMCOREFULL = POA_INF;
		//CR_X733PRM(POA_CLEAR,"CORE FULL", qualityOfServiceAlarm, 
    //           resourceAtOrNearingCapacity, NULL, ";203", 
    //           "REPT INIT ERROR %s is %i%% full", 
    //           INexecdir, (vfsInfo.f_blocks - vfsInfo.f_bavail) / (vfsInfo.f_blocks/100));
    printf("REPT INIT ERROR %s is %i%% full\n", 
           INexecdir, (vfsInfo.f_blocks - vfsInfo.f_bavail) / (vfsInfo.f_blocks/100));

		//CR_PRM(POA_INF, "REPT INIT ERROR %s is %i%% full", 
    //       INexecdir, (vfsInfo.f_blocks - vfsInfo.f_bavail) / (vfsInfo.f_blocks/100));
    printf("REPT INIT ERROR %s is %i%% full\n", 
           INexecdir, (vfsInfo.f_blocks - vfsInfo.f_bavail) / (vfsInfo.f_blocks/100));
	}

	return ret;
}
const char* 	INstartupScript = "/opt/config/bin/InstallPlatform";

#define INstartupWait		60
#define INmaxStartupError	200
#define INmaxStartup		20

int 
INrunSetup()
{
	pid_t	scriptPid;
	int	ret = -1;
	int	count = 0;
	int	sleepTime;
	int	maxWait = INstartupWait;
	char	command[200];

	if(access(INstartupScript, F_OK) != 0){
		//CR_PRM(POA_INF, "REPT INIT INSTALL SCRIPT %s NOT PRESENT", INstartupScript);
    printf("REPT INIT INSTALL SCRIPT %s NOT PRESENT\n", INstartupScript);
		IN_SLEEP(300);
		return(GLfail);
	}
	
	while(count < INmaxStartup){
		sprintf(command, "%s INIT %d", INstartupScript, count);
		if((scriptPid = fork()) == 0){
			int	ret2 = -1;
			(Void)setpgrp();
      //CR_PRM(POA_INF, "REPT INIT CALLING %s", command);
      printf("REPT INIT CALLING %s", command);
			ret2 = system(command);
			if(WIFEXITED(ret2)){
				exit(WEXITSTATUS(ret2));
			} else {
				exit(INmaxStartupError-1);
			}
		} else if(scriptPid == IN_FREEPID){
      //CR_PRM(POA_MAJ, "Failed to fork %s,errno %d", command, errno);
      printf("Failed to fork %s,errno %d\n", command, errno);
			IN_SLEEP(180);
			return(GLfail);
		}

		int	pstatus;
		sleepTime = 0;
		if(count > 0){
			maxWait = INstartupWait * 15;
		}
		while(sleepTime < maxWait && (waitpid(scriptPid, &pstatus, WNOHANG) != scriptPid)){
			sleep(1);
			sleepTime++;	
		}
		if(sleepTime < maxWait){
			if(WIFEXITED(pstatus)){
				ret = WEXITSTATUS(pstatus);
			} else {
				//CR_PRM(POA_MAJ, "REPT INIT INSTALL SCRIPT TERMINATED ABNORMALLY");
        printf("REPT INIT INSTALL SCRIPT TERMINATED ABNORMALLY\n");
				IN_SLEEP(180);
				return(GLfail);
			}
		} else {
			//CR_PRM(POA_MAJ, "REPT INIT INSTALL SCRIPT TIMED OUT");
      printf("REPT INIT INSTALL SCRIPT TIMED OUT\n");
			kill(-scriptPid, SIGKILL);
			IN_SLEEP(180);
			return(GLfail);
		}

		if(ret == 0){
			return(count);
		} else if(ret < INmaxStartupError){
			//CR_PRM(POA_MAJ, "REPT INIT INSTALL SCRIPT FAILED ret %d", ret);
      printf("REPT INIT INSTALL SCRIPT FAILED ret %d\n", ret);
			IN_SLEEP(180);
			return(GLfail);
		}
		count++;
	}
	
	//CR_PRM(POA_MAJ, "REPT INIT INSTALL SCRIPT EXECUTED TOO MANY TIMES");
  printf("REPT INIT INSTALL SCRIPT EXECUTED TOO MANY TIMES");
	IN_SLEEP(180);
	return(GLfail);
}

#define INscriptList "file.list"
#define INstartScripts	"/sn/init/start"
#define INstopScripts	"/sn/init/stop"
#define INfailoverScripts "/sn/init/failover"

int INrunScript(char* name, const char* arg ) {
	pid_t	scriptPid;
	int	ret = -1;
	int	sleepTime;
	char	command[200];

	if(access(name, F_OK) != 0){
		//CR_PRM(POA_INF, "REPT INIT %s SCRIPT %s NOT PRESENT", arg, name);
    printf("REPT INIT %s SCRIPT %s NOT PRESENT\n", arg, name);
		return(GLfail);
	}
	
	sprintf(command, "%s %s", name, arg);
	if((scriptPid = fork()) == 0){

		(Void)setpgrp();
		//mutex_unlock(&CRlockVarible);
		int	ret2 = -1;
    //CR_PRM(POA_INF, "REPT INIT SCRIPT %s INSTANTIATED", command);
    printf("REPT INIT SCRIPT %s INSTANTIATED\n", command);
		ret2 = system(command);
		if(WIFEXITED(ret2)){
			exit(WEXITSTATUS(ret2));
		} else {
			exit(INmaxStartupError-1);
		}
	} else if(scriptPid == IN_FREEPID){
    //CR_PRM(POA_MAJ, "Failed to fork %s,errno %d", command, errno);
    printf("Failed to fork %s,errno %d\n", command, errno);
		return(GLfail);
	}

	int	pstatus;
	sleepTime = 0;
	while(sleepTime < (INstartupWait * 10) && (waitpid(scriptPid, &pstatus, WNOHANG) != scriptPid)){
		IN_SLEEPN(0,100000000);
		sleepTime++;	
	}

	if(sleepTime < INstartupWait * 10){
		if(WIFEXITED(pstatus)){
			ret = WEXITSTATUS(pstatus);
		} else {
			//CR_PRM(POA_MAJ, "REPT INIT SCRIPT %s TERMINATED ABNORMALLY", command);
      printf("REPT INIT SCRIPT %s TERMINATED ABNORMALLY\n", command);
			return(GLfail);
		}
	} else {
		//CR_PRM(POA_MAJ, "REPT INIT SCRIPT %s TIMED OUT", command);
    printf("REPT INIT SCRIPT %s TIMED OUT\n", command);
		kill(-scriptPid, SIGKILL);
		return(GLfail);
	}

	if(ret != 0){
		//CR_PRM(POA_INF, "REPT INIT SCRIPT %s FAILED ret %d", command, ret);
    printf("REPT INIT SCRIPT %s FAILED ret %d\n", command, ret);
		return(GLfail);
	}

	return(GLsuccess);
}

void*
INrunScriptList(void* type)
{
  sigset_t 	new_mask;

/* Disable all signals */
  (void)sigfillset(&new_mask);
  (void)thr_sigsetmask(SIG_BLOCK,&new_mask,NULL);

	char		scriptList[300];
	FILE*		fs;
	Char		line[500];
	Char		scriptPath[500];
	const Char*	arg;
	
	if(*((int*)type) == INscriptsStart){
		sprintf(scriptList, "%s/%s", INstartScripts, INscriptList);
		arg = "start";
	} else if(*((int*)type) == INscriptsFailover){
		sprintf(scriptList, "%s/%s", INfailoverScripts, INscriptList);
		arg = "start";
	} else {
		sprintf(scriptList, "%s/%s", INstopScripts, INscriptList);
		arg = "stop";
	}

	if(access(scriptList, F_OK) != 0){
		IN_LDSCRIPTSTATE = INscriptsFinished;
		return(NULL);
	}

	fs = fopen(scriptList, "r");
	if(fs == NULL){
		//CR_PRM(POA_INF, "REPT INIT SCRIPT FAILED TO OPEN %s", scriptList);
    printf("REPT INIT SCRIPT FAILED TO OPEN %s\n", scriptList);
		IN_LDSCRIPTSTATE = INscriptsFailed;
		return(NULL);
	}
	
	while(fs != NULL && fgets(line, 1000, fs)){
		int	cmdLen = strlen(line);
		if(line[cmdLen - 1] == '\n'){
			line[cmdLen - 1] = 0;
		}
		
		if(*((int*)type) == INscriptsStart){
			sprintf(scriptPath, "%s/%s", INstartScripts, line);
		} else if(*((int*)type) == INscriptsFailover){
			sprintf(scriptPath, "%s/%s", INfailoverScripts, line);
		} else {
			sprintf(scriptPath, "%s/%s", INstopScripts, line);
		}
		if(INrunScript(scriptPath, arg) != GLsuccess){
			IN_LDSCRIPTSTATE = INscriptsFailed;
			return(NULL);
		}
	}

	fclose(fs);

	IN_LDSCRIPTSTATE = INscriptsFinished;
	return(NULL);
}

