//
// DESCRIPTION:
//	This file contains routines to handle the creation
//	and termination of processes.
//
// FUNCTIONS:
//	INcreate()    - create a process
//
// NOTES:
//

#include <sysent.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include <string.h>

#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/param.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <sched.h>
#include <sys/resource.h>

#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "cc/hdr/init/INproctab.hh"
#include "cc/init/proc/INtimers.hh"

#include "cc/init/proc/INlocal.hh"


extern mutex_t CRlockVarible;

void
INconfigNew(U_short indx, const char* edir)
{
	struct passwd*	pwd;
	if (INroot == TRUE) {
		/* Set the current working directory: */
		if (chdir(edir) < 0) {
			/*
			 * We shouldn't spool messages at this point
			 * as we're the child, simply output an
			 * error message on the console:
			 */
			//CR_PRM(POA_INF,"REPT INIT ERROR CREATE FAILED chdir() CALL :errno = %d, PROC = %s, DIR = %s",errno,IN_LDPTAB[indx].proctag, edir);
      printf("REPT INIT ERROR CREATE FAILED chdir() CALL :errno = %d, PROC = %s, DIR = %s",
             errno,IN_LDPTAB[indx].proctag, edir);
			_exit(0);
		}

		struct sched_param params;
		int	sched_class = SCHED_OTHER;
		if(IN_LDPTAB[indx].isRT == IN_RR) {
			sched_class = SCHED_RR;	
			params.sched_priority = IN_LDPTAB[indx].priority;
		} else if(IN_LDPTAB[indx].isRT == IN_FIFO) {
			sched_class = SCHED_FIFO;	
			params.sched_priority = IN_LDPTAB[indx].priority;
		} else {
			params.sched_priority = 0;
		}
		if(sched_setscheduler((pid_t)0, sched_class, &params) != 0){
			//CR_PRM(POA_INF,"REPT INIT ERROR FAILED sched_setcheduler() errno %d FOR PROC = %s",errno, IN_LDPTAB[indx].proctag);
      printf("REPT INIT ERROR FAILED sched_setcheduler() errno %d FOR PROC = %s",
             errno, IN_LDPTAB[indx].proctag);
		} 
		if(IN_LDPTAB[indx].isRT == IN_NOTRT){
			if(setpriority(PRIO_PROCESS, 0, (int)IN_LDPTAB[indx].priority - 20) != 0){
				//CR_PRM(POA_INF,"REPT INIT ERROR FAILED setpriority() errno %d FOR PROC = %s",errno, IN_LDPTAB[indx].proctag);
        printf("REPT INIT ERROR FAILED setpriority() errno %d FOR PROC = %s",
               errno, IN_LDPTAB[indx].proctag);
			}
		}

    /* Set usr and group ids */
		gid_t	gid;
		if((gid = IN_LDPTAB[indx].group_id) == (gid_t) -1){
			gid = IN_LDPTAB[indx].uid;
		} 
    if((setgid(gid) < 0)) {
      //CR_PRM(POA_INF,"REPT INIT ERROR CREATE FAILED setgid() %d  errno %d, PROC = %s", gid, errno, IN_LDPTAB[indx].proctag);
      printf("REPT INIT ERROR CREATE FAILED setgid() %d  errno %d, PROC = %s\n",
             gid, errno, IN_LDPTAB[indx].proctag);
      _exit(0);
    }
 
    /* Get the name for the given UID */
    if ((pwd = getpwuid(gid)) == (struct passwd *) NULL) {      
      //CR_PRM(POA_INF,"REPT INIT ERROR CREATE FAILED getpwuid() errno %d, PROC = %s", errno, IN_LDPTAB[indx].proctag);
      printf("REPT INIT ERROR CREATE FAILED getpwuid() errno %d, PROC = %s\n",
             errno, IN_LDPTAB[indx].proctag);
    }
    /* Initialize the group structure */
    if ((initgroups( pwd->pw_name, gid) < 0)) {
      //CR_PRM(POA_INF,"REPT INIT ERROR CREATE FAILED initgroups() errno %d, PROC = %s", errno, IN_LDPTAB[indx].proctag);
      printf("REPT INIT ERROR CREATE FAILED initgroups() errno %d, PROC = %s\n",
             errno, IN_LDPTAB[indx].proctag);
    }

    if((setuid(IN_LDPTAB[indx].uid) < 0)) {
      //CR_PRM(POA_INF,"REPT INIT ERROR CREATE FAILED setuid() errno %d, PROC = %s", errno, IN_LDPTAB[indx].proctag);
      printf("REPT INIT ERROR CREATE FAILED setuid() errno %d, PROC = %s\n",
             errno, IN_LDPTAB[indx].proctag);
      _exit(0);
    }
	}		

	(Void)setpgrp();
}

GLretVal
INthirdPartyExec(U_short indx, IN_SYNCSTEP syncstep)
{
	const char*	arg1;
	int		timeout;

	switch(syncstep){
	case IN_PROCINIT:
		arg1 = "start";
		timeout = IN_LDPTAB[indx].procinit_timer;
		break;
	case IN_CLEANUP:
		arg1 = "stop";
		// extended if many cleanups are requested
		if(IN_LDSTATE.systep == IN_CLEANUP){
      timeout = INCLEANUPTMR;
		} else {
      timeout = INPCLEANUPTMR;
		}
		IN_SDPTAB[indx].procstep = IN_CLEANUP;
		break;
	case IN_PROCESS:
		arg1 = "check";
		timeout = IN_LDPTAB[indx].peg_intvl/2;
		break;
	}

	pid_t  createpid;
  if((createpid = fork()) == 0){
    //mutex_unlock(&CRlockVarible);
    Char	edir[IN_NAMEMX+IN_PATHNMMX];
    char	arg0[IN_NAMEMX];
    IN_LDPTAB[indx].pid = getpid();
    sprintf(edir, "%s/%s", INexecdir, IN_LDPTAB[indx].proctag);
    INconfigNew(indx, edir);
    sigset_t        new_mask;
    (void)sigfillset(&new_mask);
    (void)thr_sigsetmask(SIG_UNBLOCK, &new_mask, NULL);
    strcpy(arg0, IN_LDPTAB[indx].proctag);
    execl(IN_LDPTAB[indx].pathname, arg0, arg1, (char*)0);
    //CR_PRM(POA_MAJ, "Failed to exec %s %s %s ,errno %d", IN_LDPTAB[indx].pathname, arg0, arg1, errno);
    printf("Failed to exec %s %s %s ,errno %d\n",
           IN_LDPTAB[indx].pathname, arg0, arg1, errno);
    _exit(1);
  } else if(createpid == ((pid_t)-1)){
    return(GLfail);
  }

  IN_LDPTAB[indx].pid = createpid;

  int	stat;
  pid_t	ret;
  struct timespec tsleep;

  tsleep.tv_sec = 1;
  tsleep.tv_nsec = 0;	


  while((--timeout) > 0 && IN_SDPTAB[indx].procstep != IN_BCLEANUP &&
        IN_SDPTAB[indx].procstep != IN_ECLEANUP){
    nanosleep(&tsleep, NULL);
    if(kill(createpid, 0) < 0){
      return(IN_SDPTAB[indx].ret);
    } 
  }

  if(timeout == 0){
    kill(-createpid, SIGKILL);
    //CR_X733PRM(POA_TMJ, "THIRD PARTY TIMEOUT", processingErrorAlarm, applicationSubsystemFailure, NULL, ";213", "REPT INIT TIMED OUT EXECUTING THIRD PARTY CHECK %s", IN_LDPTAB[indx].proctag);
    printf("THIRD PARTY TIMEOUT REPT INIT TIMED OUT EXECUTING THIRD PARTY CHECK %s",
           IN_LDPTAB[indx].proctag);
    return(GLfail);
  }

  return(GLfail);
}

extern "C" void* INthirdParty(void*);

void*
INthirdParty(void* pIndx)
{
  sigset_t        new_mask;
  (void)sigfillset(&new_mask);
  (void)sigdelset(&new_mask, SIGTERM);
  (void)sigdelset(&new_mask, SIGUSR2);
  (void)thr_sigsetmask(SIG_BLOCK, &new_mask, NULL);

  U_short indx = (U_short)((long)pIndx);
  if(IN_SDPTAB[indx].procstep == INV_STEP){
    IN_SDPTAB[indx].procstate = IN_HALTED;
    IN_SDPTAB[indx].procstep = IN_EHALT;
  }
	
  struct timespec tsleep;
  tsleep.tv_sec = 0;
  tsleep.tv_nsec = 50000000;	
	
  int	done = FALSE;

  while(!done){
    while(IN_SDPTAB[indx].procstate == IN_HALTED){
      nanosleep(&tsleep, NULL);
    }

    switch(IN_SDPTAB[indx].procstep){
    case IN_BSTARTUP:
      IN_SDPTAB[indx].procstate = IN_HALTED;
      IN_SDPTAB[indx].procstep = IN_ESTARTUP;
      break;
    case IN_BCLEANUP:
      INthirdPartyExec(indx, IN_CLEANUP);
    case IN_ECLEANUP:
      IN_SDPTAB[indx].procstep = IN_ECLEANUP;
      kill(IN_procdata->pid, SIGCLD);
      return(0);
    case IN_BSYSINIT:
      IN_SDPTAB[indx].ret = GLsuccess;
      IN_SDPTAB[indx].procstate = IN_HALTED;
      IN_SDPTAB[indx].procstep = IN_ESYSINIT;
      break;
    case IN_BPROCINIT:
      IN_SDPTAB[indx].procstep = IN_PROCINIT;
      IN_SDPTAB[indx].ret = INthirdPartyExec(indx, IN_PROCINIT);
      if(IN_SDPTAB[indx].ret != GLsuccess){
        return(0);
      }
      IN_SDPTAB[indx].procstate = IN_HALTED;
      IN_SDPTAB[indx].procstep = IN_EPROCINIT;
      //CR_PRM(POA_INF, "REPT INIT %s PROGRESS 0 PROCINIT COMPLETE", IN_LDPTAB[indx].proctag);
      printf("REPT INIT %s PROGRESS 0 PROCINIT COMPLETE\n",
             IN_LDPTAB[indx].proctag);
      break;
    case IN_BPROCESS:
      if(IN_SDPTAB[indx].alvl == POA_INF){
        //CR_PRM(POA_INF, "REPT INIT %s PROGRESS 0 INITIALIZATION COMPLETE", IN_LDPTAB[indx].proctag);
        printf("REPT INIT %s PROGRESS 0 INITIALIZATION COMPLETE\n",
               IN_LDPTAB[indx].proctag);
      } else {
        //CR_X733PRM(POA_CLEAR, IN_LDPTAB[indx].proctag, qualityOfServiceAlarm,
        //           softwareProgramAbnormallyTerminated, NULL, ";201",
        //           "REPT INIT %s PROGRESS 0 INITIALIZATION COMPLETE",
        //           IN_LDPTAB[indx].proctag);
        printf("REPT INIT %s PROGRESS 0 INITIALIZATION COMPLETE",
               IN_LDPTAB[indx].proctag);
        IN_SDPTAB[indx].alvl == POA_INF;
      }
      IN_SDPTAB[indx].procstep = IN_ECPREADY;
      done = TRUE;
      break;
    case IN_ECPREADY:
    case IN_STEADY:
      done = TRUE;
      break;
    }
  }

#define INsleepIntvl	5

  tsleep.tv_sec = INsleepIntvl;
  tsleep.tv_nsec = 0;	
  int	loopCnt = 0;
  int	sanityCnt = (IN_LDPTAB[indx].peg_intvl/INsleepIntvl) -1;
  while(1){
    nanosleep(&tsleep, NULL);
    if(IN_SDPTAB[indx].procstep == IN_BCLEANUP){
      INthirdPartyExec(indx, IN_CLEANUP);
      IN_SDPTAB[indx].procstep = IN_ECLEANUP;
      kill(IN_procdata->pid, SIGCLD);
      return(0);
    } else if(IN_SDPTAB[indx].procstep == IN_ECLEANUP){
      kill(IN_procdata->pid, SIGCLD);
      return(0);
    }

    if(((++loopCnt) % sanityCnt) == 0){
      if(INthirdPartyExec(indx, IN_PROCESS) == GLsuccess){
        IN_SDPTAB[indx].count++;
      }
    }
  }
}


/*
** NAME:
**	INcreate()
**
** DESCRIPTION:
**	This function creates a process using information stored in the
**	shared memory process table entry associated with the index
**	passwd to the routine
**
** INPUTS:
**	indx	- Process table index of process to be created.
**
** RETURNS:
**	GLsuccess	- Process successfully created
**	GLfail		- Process was not created.  In this case, "INcreate()"
**			  generates an alarmed output message indicating
**			  the nature of the process creation failure.
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
*/
/*	Side Effects:
 *			The IN_LDPTAB[] table is updated to reflect the
 *			fact that a process is pending creation.
 */



GLretVal
INcreate(U_short indx)
{
  struct stat	stbuf;
  GLretVal	ret;
  Char	arg0[IN_NAMEMX];
  Char	pathnm[IN_PATHNMMX+4];
  Char	edir[IN_NAMEMX+IN_PATHNMMX];
  static Char env_string[IN_NAMEMX + 30];

  if(INcheck_image(indx) != GLsuccess){
    return(GLfail);	
  }

  /*
   * If the process is in the update state but has not
   * yet been started, use the update path.
   */
  strcpy(pathnm, IN_LDPTAB[indx].pathname);

  if (IN_LDPTAB[indx].updstate == UPD_PRESTART) {
    strcat(pathnm, ".new");
  }

  /*
   * First, verify that the path name references a non-null
   * executable file:
   */
  ret = stat(pathnm, &stbuf);
  if (ret < 0) {
    /*
     * Can't access the file 
     */
    //CR_PRM(POA_INF,"REPT INIT ERROR CAN'T ACCESS \"%s\" FOR PROCESS CREATION", pathnm);
    printf("REPT INIT ERROR CAN'T ACCESS \"%s\" FOR PROCESS CREATION",
           pathnm);
    return(INNOEXIST);
  }

  if (stbuf.st_size == 0) {
    /*
     * Zero length file
     */
    //CR_PRM(POA_INF,"REPT INIT ERROR EXECUTABLE \"%s\" IS EMPTY", pathnm);
    printf("REPT INIT ERROR EXECUTABLE \"%s\" IS EMPTY", pathnm);
    return(INNOEXIST);
  }
			
  if (!(stbuf.st_mode & S_IXOTH)) {
    /*
     * Not executable to others, skip this entry:
     */
    //CR_PRM(POA_INF,"REPT INIT ERROR \"%s\" IS NOT EXECUTABLE, CAN'T CREATE PROCESS", pathnm);
    printf("REPT INIT ERROR \"%s\" IS NOT EXECUTABLE, CAN'T CREATE PROCESS",
           pathnm);
    return(INNOTEXECUT);
  }

  if (INroot == TRUE)
  {
    Char cmd[516];
    /*
     * On the CC we will attempt to set the child's working
     * directory to "INexecdir/msgh_name"...e.g. for MSGH this
     * is "/sn/dump/MSGH"
     */

    /* create directory name */
    sprintf(edir, "%s/%s", INexecdir, IN_LDPTAB[indx].proctag);

    /* recovery assumes that INexecdir already exists */
    if (stat(edir, &stbuf) < 0)
    {
      /* check errno to determine how to recover */
      switch (errno)
      {
      case ENOENT:
        /* file does not exist */
        if (mkdir(edir, 0777) < 0)
        {
          //CR_PRM(POA_INF,"REPT INIT ERROR CANNOT CREATE \"%s\" DIRECTORY FAILED TO CREATE \"%s\" PROCESS",edir, IN_LDPTAB[indx].proctag);
          printf("REPT INIT ERROR CANNOT CREATE \"%s\" DIRECTORY FAILED TO CREATE \"%s\" PROCESS",
                 edir, IN_LDPTAB[indx].proctag);
          return(INNOCORE);
        }
        (Void) chmod(edir, 0777);
        break;

      case ENOTDIR:
        /* /sn or /sn/core are NOT directories */
        //CR_PRM(POA_INF,"REPT INIT ERROR FILE \"%s\" NOT A DIRECTORY FAILED TO CREATE \"%s\" PROCESS", INexecdir, IN_LDPTAB[indx].proctag);
        printf("REPT INIT ERROR FILE \"%s\" NOT A DIRECTORY FAILED TO CREATE \"%s\" PROCESS",
               INexecdir, IN_LDPTAB[indx].proctag);
        return(INNOCORE);

      default:
        /* delete whatever may be there */
        sprintf(cmd, "/bin/rm -rf %s 1>/dev/null 2>&1",
                edir);
        system(cmd);
        /* create directory */
        if (mkdir(edir, 0777) < 0)
        {
          //CR_PRM(POA_MAJ,"REPT INT ERROR CANNOT CREATE \"%s\" DIRECTORY\n\tFAILED TO CREATE \"%s\" PROCESS",edir, IN_LDPTAB[indx].proctag);
          printf("REPT INT ERROR CANNOT CREATE \"%s\" DIRECTORY\n\tFAILED TO CREATE \"%s\" PROCESS",
                 edir, IN_LDPTAB[indx].proctag);
          return(INNOCORE);
        }
        (Void) chmod(edir, 0777);
        break;
      }
    }
    else if ((stbuf.st_mode & S_IFMT) != S_IFDIR)
    {
      /* not a directory, delete whatever may be there */
      sprintf(cmd, "/bin/rm -rf %s 1>/dev/null 2>&1", edir);
      system(cmd);

      /* re-create directory */
      if (mkdir(edir, 0777) < 0)
      {
        //CR_PRM(POA_INF,"REPT INIT ERROR CANNOT CREATE \"%s\" DIRECTORY\n\tFAILED TO CREATE \"%s\" PROCESS",edir, IN_LDPTAB[indx].proctag);
        printf("REPT INIT ERROR CANNOT CREATE \"%s\" DIRECTORY\n\tFAILED TO CREATE \"%s\" PROCESS",
               edir, IN_LDPTAB[indx].proctag);
        return(INNOCORE);
      }
      (Void) chmod(edir, 0777);
    }
    else if ((stbuf.st_mode & S_IRWXO) != S_IRWXO)
    {
      /* reset permissions */
      (Void) chmod(edir, 0777);
    }
  }

  /*
   * Change the process's state to "NOEXIST" so that we will not
   * find an inconsistency in "INgrimreaper()" if the child gets
   * hung up before it can store its PID:
   */
  IN_SDPTAB[indx].procstate = IN_NOEXIST;
  if(IN_LDPTAB[indx].pid != IN_FREEPID){
    IN_LDPTAB[indx].pid = IN_FREEPID;
    //INIT_ERROR(("Non null pid %d, proctag %d",IN_LDPTAB[indx].pid,IN_LDPTAB[indx].proctag));
    printf("Non null pid %d, proctag %d\n",IN_LDPTAB[indx].pid,IN_LDPTAB[indx].proctag);
  }
	
  /*
   *  Create the new process. If this is a third party process, create the 
   *  third party thread.
   */

  if(IN_LDPTAB[indx].third_party){
    thread_t	newthread;
    if(thr_create(NULL, thr_min_stack()+64000, INthirdParty, (void*)indx, THR_DETACHED, &newthread) != 0){
      //INIT_ERROR(("Failed to create third party thread, errno %d", errno));
      printf("Failed to create third party thread, errno %d\n", errno);
      return(INFORKFAIL);
    }
    IN_LDPTAB[indx].tid = newthread;
    return(GLsuccess);
  }
  pid_t new_pid;

  if((new_pid = fork()) > 0){
    IN_LDPTAB[indx].pid = new_pid;
    /*
     * New process started, if update state was pre-start 
     * change it to post-start
     */
    if (IN_LDPTAB[indx].updstate == UPD_PRESTART) {
      IN_LDPTAB[indx].updstate = UPD_POSTSTART;
    }
  } else if (new_pid < 0) {
    /*
     *  Could not fork a new process, note error,
     *  return procstate to "DEAD" and return:
     */
    IN_SDPTAB[indx].procstate = IN_DEAD;
    //CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO CREATE PROC = %s, errno = %d",IN_LDPTAB[indx].proctag, errno);
    printf("REPT INIT ERROR FAILED TO CREATE PROC = %s, errno = %d",
           IN_LDPTAB[indx].proctag, errno);
    /* Documented errno can only EAGAIN which is only a result
    ** of system resource problems.  Just do UNIX boot at this point.
    */
    INescalate(IN_MAXSNLVL,INFORKFAIL,IN_SOFT,indx);
    return(INFORKFAIL);
  } else {
    /*
     *  We're the child:
     *  Do some housekeeping.  Note that we don't need
     *  to explicitly detach the child from the shared
     *  memory segments as this happens as a result of
     *  the exec() call.  Likewise, signals caught by
     *  signal handling routines (e.g. SIGCLD) are reset
     *  to SIGIGN by exec()
     */
    IN_SDPTAB[indx].procstate = IN_CREATING;
    alarm((unsigned) 0);
    signal(SIGINT,SIG_IGN);
    signal(SIGHUP,SIG_IGN);
    signal(SIGALRM,SIG_IGN);
    signal(SIGCLD,SIG_IGN);
    signal(SIGUSR1,SIG_IGN);

    // Make sure that CR_PRM is not blocked by another thread
    //mutex_unlock(&CRlockVarible);
    INconfigNew(indx, edir);
    /*
     *  Get client's proc's tag name for exec() call
     *  (already have path name)
     */
    strcpy(arg0, IN_LDPTAB[indx].proctag);

    /* Put process name in environment variable _INMSGHNAME so that 
    ** the process can access it from the code running as part of
    ** global constructors, that might need it.
    */
    strcpy(env_string,IN_MSGH_ENV);
    strcat(env_string,"=");
    strcat(env_string,arg0);

    if(putenv(env_string) != 0){
      //CR_X733PRM(POA_TMJ, "PUTENV", processingErrorAlarm, softwareError, NULL, ";220", "INIT REPT ERROR CREATE FAILED putenv(), env_string = %s, errno = %d", env_string, errno);
      printf("INIT REPT ERROR CREATE FAILED putenv(), env_string = %s, errno = %d\n",
             env_string, errno);
      _exit(0);
    }

    /*
     * Replace process with client proc
     */
    //INIT_DEBUG((IN_DEBUG | IN_SNCRTR),(POA_INF,"INcreate(): client is alive:\n\tprocess = \"%s\", pid = %d\n\tpath = \"%s\"",IN_LDPTAB[indx].proctag,getpid(),pathnm));
    printf("INcreate(): client is alive:\n\tprocess = \"%s\", pid = %d\n\tpath = \"%s\"\n",
           IN_LDPTAB[indx].proctag,getpid(),pathnm);

    /* Print first progress step for the processes about to be
    ** created.  It is done here instead of library to avoid
    ** console messages and local logs errors from CSOP
    ** since process not yet attached to MSGH when the first 
    ** message is printed.
    */
    if(IN_LDPTAB[indx].print_progress){
      //CR_PRM(POA_INF,"REPT INIT %s PROGRESS 0 CREATED",arg0);
      printf("REPT INIT %s PROGRESS 0 CREATED\n",arg0);
    }
    (Void) execl(pathnm, arg0, (char *)0);

    /*
     *  Should not return from exec(), if so, note error
     *  and exit
     */
    //CR_X733PRM(POA_TMJ, "EXEC FAILED", processingErrorAlarm, softwareError, NULL, ";219", 
    //           "INIT REPT ERROR CREATE FAILED exec(), PATH = %s, errno = %d", pathnm, errno);
    printf("INIT REPT ERROR CREATE FAILED exec(), PATH = %s, errno = %d\n",
           pathnm, errno);
    _exit(0);
  }

  //INIT_DEBUG((IN_DEBUG | IN_SNCRTR),(POA_INF,"INcreate(): returned after successful client proc. creation"));
  printf("INcreate(): returned after successful client proc. creation\n");
  return(GLsuccess);
}

#define IN_CBUF_SZ	128*1024

/*
** NAME:
**	INcheck_image()
**
** DESCRIPTION:
** 	This function compares the official image to current images and does
**	a file copy if official image is newer or sizes don't match.
**	This function blocks INIT while copy takes place, this normally
**	is not a problem, however it could hang INIT if nfs goes away.
**	Therefore, INIT should either handshake with SMON or 
**	do the cp in a separate timed thread.
**
** INPUTS:
**	indx	- Process table index of process to be checked.
**
** RETURNS:
**	GLsuccess	- Process image ssuccessfully verified
**	GLfail		- Process image could not be brought up to date.
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
*/
GLretVal
INcheck_image(U_short indx)
{
  struct stat	stat_cur;
  struct stat	stat_ofc;
  int		to_file, from_file;
  char		buffer[IN_CBUF_SZ];
  int		num;

  if(IN_LDSTATE.final_runlvl > IN_LDSTATE.run_lvl){
    return(GLsuccess);
  }

  if(IN_LDPTAB[indx].ofc_pathname[0] != 0){  
    if(stat(IN_LDPTAB[indx].ofc_pathname, &stat_ofc) < 0){
      //CR_X733PRM(POA_TMJ, "FAILED TO FIND", processingErrorAlarm, softwareError, NULL, ";223", "REPT INIT ERROR FAILED TO FIND %s", IN_LDPTAB[indx].ofc_pathname);
      printf("REPT INIT ERROR FAILED TO FIND %s",
             IN_LDPTAB[indx].ofc_pathname);
      return(GLfail); 
    } else if(stat_ofc.st_size == 0 || !(stat_ofc.st_mode & S_IXOTH)){
      //CR_X733PRM(POA_TMJ,"INVALID SIZE", processingErrorAlarm, softwareError, NULL, ";229", "REPT INIT ERROR INVALID SIZE OR PERMISSIONS OF %s", IN_LDPTAB[indx].ofc_pathname);
      printf("REPT INIT ERROR INVALID SIZE OR PERMISSIONS OF %s",
             IN_LDPTAB[indx].ofc_pathname);
      return(GLfail);
    }
  } else {
    return(GLsuccess);
  }

  if(stat(IN_LDPTAB[indx].pathname,&stat_cur) < 0 ||
     stat_cur.st_size != stat_ofc.st_size ||
     !(stat_cur.st_mode & S_IXOTH) ||
     stat_cur.st_mtime < stat_ofc.st_mtime){
    //CR_PRM(POA_INF,"REPT INIT GETTING OFFICIAL COPY OF %s", IN_LDPTAB[indx].proctag);
    printf("REPT INIT GETTING OFFICIAL COPY OF %s\n", IN_LDPTAB[indx].proctag);
    if((from_file = open(IN_LDPTAB[indx].ofc_pathname,O_RDONLY)) < 0){
      //CR_X733PRM(POA_TMJ,"FAILED TO OPEN OFFICIAL", processingErrorAlarm, softwareError, NULL, ";225", "REPT INIT ERROR FAILED TO OPEN OFFICIAL FILE %s ERRNO %d",
      //           IN_LDPTAB[indx].ofc_pathname, errno);
      printf("REPT INIT ERROR FAILED TO OPEN OFFICIAL FILE %s ERRNO %d\n",
             IN_LDPTAB[indx].ofc_pathname, errno);
      return(GLfail);
    }

    if((to_file = open(IN_LDPTAB[indx].pathname,O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0){
      //CR_X733PRM(POA_TMJ,"FAILED TO OPEN PROCESS", processingErrorAlarm, softwareError, NULL, ";226", "REPT INIT ERROR FAILED TO OPEN PROCESS FILE %s ERRNO %d",
      //            IN_LDPTAB[indx].pathname, errno);
      printf("REPT INIT ERROR FAILED TO OPEN PROCESS FILE %s ERRNO %d\n",
             IN_LDPTAB[indx].pathname, errno);
      close(from_file);
      return(GLfail);
    }

    int	totalbytes = 0;
    struct tms tbuffer;
    clock_t	lasttime = times(&tbuffer);
    clock_t curtime;
    while(1){
      if((num = read(from_file, (void*)buffer, IN_CBUF_SZ)) < 0){
        //CR_X733PRM(POA_TMJ,"FAILED TO READ OFFICIAL", processingErrorAlarm, softwareError, NULL, ";227", "REPT INIT ERROR FAILED TO READ OFFICIAL FILE %s ERRNO %d",
        //           IN_LDPTAB[indx].ofc_pathname, errno);
        printf("REPT INIT ERROR FAILED TO READ OFFICIAL FILE %s ERRNO %d\n",
               IN_LDPTAB[indx].ofc_pathname, errno);
        break;
      }

      totalbytes += num;
      if(totalbytes > 2 * 1024 * 1024){
        curtime = times(&tbuffer);
        if((curtime - lasttime) > 10 * TMHZ){
          //CR_PRM(POA_MAJ, "REPT INIT OFFICIAL FILE COPY TOO SLOW 2 MB REQUIRED %d TICKS", curtime - lasttime);
          printf("REPT INIT OFFICIAL FILE COPY TOO SLOW 2 MB REQUIRED %d TICKS\n",
                 curtime - lasttime);
        }
        lasttime = curtime;
        INsanityPeg ++;
        totalbytes = 0;
      }

      if(num == 0){
        break;
      }

      if((num = write(to_file, buffer, num)) < 0){
        //CR_X733PRM(POA_TMJ,"FAILED TO WRITE", processingErrorAlarm, softwareError, NULL, ";228", "REPT INIT ERROR FAILED TO WRITE FILE %s ERRNO %d",
        //           IN_LDPTAB[indx].pathname, errno);
        printf("REPT INIT ERROR FAILED TO WRITE FILE %s ERRNO %d\n",
               IN_LDPTAB[indx].pathname, errno);
        break;
      }
    }

    close(from_file);
    close(to_file);

    if(num < 0){
      //CR_X733PRM(POA_TMJ,"FAILED OFFICIAL COPY", processingErrorAlarm, softwareError, NULL, ";224", 
      //           "REPT INIT ERROR FAILED TO GET OFFICIAL COPY OF %s", IN_LDPTAB[indx].proctag);
      printf("REPT INIT ERROR FAILED TO GET OFFICIAL COPY OF %s\n",
             IN_LDPTAB[indx].proctag);
      return(GLfail);
    }

    if(chown(IN_LDPTAB[indx].pathname, stat_ofc.st_uid, stat_ofc.st_gid) < 0){
      //CR_X733PRM(POA_TMJ, "CHANGE OWNER", processingErrorAlarm, softwareError, NULL, ";221", "REPT INIT ERROR FAILED TO CHANGE EXECUTABLE OWNER ID FOR %s, ERRNO %d",
      //           IN_LDPTAB[indx].proctag, errno);
      printf("REPT INIT ERROR FAILED TO CHANGE EXECUTABLE OWNER ID FOR %s, ERRNO %d\n",
             IN_LDPTAB[indx].proctag, errno);
    }
    if(chmod(IN_LDPTAB[indx].pathname, stat_ofc.st_mode) < 0){
      //CR_X733PRM(POA_TMJ,"CHANGE PERMISSIONS", processingErrorAlarm, softwareError, NULL, ";222", "REPT INIT ERROR FAILED TO CHANGE EXECUTABLE PERMISSIONS FOR %s, ERRNO %d",
      //           IN_LDPTAB[indx].proctag, errno);
      printf("REPT INIT ERROR FAILED TO CHANGE EXECUTABLE PERMISSIONS FOR %s, ERRNO %d\n",
             IN_LDPTAB[indx].proctag, errno);
    }
  }

  return(GLsuccess);
}
