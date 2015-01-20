//
// DESCRIPTION:
// 	This file contains INIT subsystem routines used to restart
//	and resynchronize processes which have died.
//
// FUNCTIONS:
//	INgrimreaper()	- Check for processes whose "time has come"...
//
//	INdeath()	- Handle "death of process" processing
//
//	INreqinit()	- Initialize process
//
//	INsetrstrt()	- Cycle a process through resynchronization
//
//	INdeadproc()	- Processing required when a permanent process
//			  with its restart state set to "INH_RESTART"
//			  or whenever a temporary process exceeds its
//			  restart threshold.
//
//	INkillprocs()	- Kill all procs in the process tables
//
//	INautobkout()	- Do automatic backout of updating process(es).
//                        (When software update fails)
//	INfreeshmem()	- Free shared memory for a process
//	INfreesem()	- Free semaphores for a process
//	INrun_bkout()	- Control execution of  BKOUT script
//	INsys_bk()	- System call to run BKOUT script
//	INalarm()	- Catches BKOUT script timeout alarm
//	INfinish_bkout()- Execute end of BACKOUT cleanup activities
//	INsudata_find() - Find index of a process in INsudata
//
// NOTES:
//
#include <sysent.h>		/* external declaration for "kill()" */
#include <errno.h>
#include <signal.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#ifdef CC
#include <sys/param.h>
#endif
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>

#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"

#include "cc/hdr/init/INinit.hh"
#include "cc/hdr/init/INproctab.hh"
//#include "cc/hdr/su/SUbkoutMsg.hh"
//#include "cc/hdr/su/SUversion.hh"
//#include "cc/hdr/su/SUparseName.hh"
#include "cc/init/proc/INlocal.hh"
#include "cc/init/proc/INmsgs.hh"

/*
** NAME:
**	INgrimreaper()
**
** DESCRIPTION:
**	This function scans the process table for processes which have
**	died and schedules death of process handling.
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
INgrimreaper()
{
	//INIT_DEBUG(IN_DEBUG,(POA_INF,"INgrimreaper(): routine entry"));
  printf("INgrimreaper(): routine entry\n");
	
	/* Get all the zombies */
	while(INcheck_zombie() > 0);
	
	for (U_short i = 0; i < IN_SNPRCMX; i++) {
		if(IN_INVPROC(i)){
			continue;
		}

		switch(IN_SDPTAB[i].procstate) {
		case IN_NOEXIST:
			/* Skip over processes which may have just started*/
			break;

		case IN_CREATING:
		case IN_HALTED:
		case IN_RUNNING:
			/* Now check the proc's pid */
			if ((IN_LDPTAB[i].pid <= 1 && !IN_LDPTAB[i].third_party) || 
			    (IN_LDPTAB[i].third_party &&  IN_LDPTAB[i].tid == (IN_FREEPID))) {
				/*
				 * Invalid PID...don't try to kill UNIX
				 * proc0 or proc1!!!
				 */
				//INIT_ERROR(("Invalid PID %d for process \"%s\"",IN_LDPTAB[i].pid,IN_LDPTAB[i].proctag));
        printf("Invalid PID %d for process \"%s\"\n",
               IN_LDPTAB[i].pid,IN_LDPTAB[i].proctag);
			}
			else {
				if(INkill(i, 0) == 0) {
					/*
           * Process seems to be alive and well
           */
					continue;
				}
			}

			/*
			 * Handle process death.
			 */
			INdeath(i);
			break;

		case IN_DEAD:
			/*
			 * Processes in these states are not "alive"
			 * -- should have NULL PIDs -- so don't even TRY
			 * to detect their death
			 */
			if (IN_LDPTAB[i].pid >= 0) {
				//INIT_ERROR(("Proc \"%s\", indx %d had non-null pid %d in procstate %s",IN_LDPTAB[i].proctag, i, IN_LDPTAB[i].pid,IN_PROCSTNM(IN_SDPTAB[i].procstate)));
        printf("Proc \"%s\", indx %d had non-null pid %d in procstate %s",
               IN_LDPTAB[i].proctag, i, IN_LDPTAB[i].pid,IN_PROCSTNM(IN_SDPTAB[i].procstate));

				/* Set PID to null values at this point... */
				IN_LDPTAB[i].pid = IN_FREEPID;
			}
			break;

		case IN_MAXSTATE:
		case IN_INVSTATE:
		default:
			//INIT_ERROR(("Invalid procstate %d encountered, indx %d", IN_SDPTAB[i].procstate, i));
      printf("Invalid procstate %d encountered, indx %d\n",
             IN_SDPTAB[i].procstate, i);
			/* Reset the state to IN_RUNNING and reinit the process */
			IN_SDPTAB[i].procstate = IN_RUNNING;
			INreqinit(SN_LV0,i,INBADSHMEM,IN_SOFT,"CORRUPTED SHARED MEMORY");
		}
	}

#ifdef OLD_SU
	/* Check if BKOUT process is still alive */
	if(IN_LDBKPID != IN_FREEPID){
		if(kill(IN_LDBKPID,0) < 0) {
			INautobkout(-1,FALSE);
		}
	}
#endif

	return;
}

/*
** NAME:
**	INdeath()
**
** DESCRIPTION:
**	This function handles the processing required upon the detection
**	of a process death.
**
** INPUTS:
**
** RETURNS:
**
** CALLS:
**	INSETTMR - INIT's timing library is accessed to set/clear process-
**
** CALLED BY:
**	INgrimreaper()	- When process death is detected
**
** SIDE EFFECTS:
*/

Void
INdeath(U_short indx)
{
	pid_t oldpid;
	Bool  rstrt_flg;
	IN_PERMSTATE	permstate;
	char		proctag[IN_NAMEMX];
	IN_UPDSTATE	updstate;
	CRALARMLVL a_lvl = POA_MAJ;
	SN_LVL		sn_lvl;
	Bool		send_pdeath = TRUE;

	//INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INdeath():process died: \"%s\"\n\tpid = %d indx = %d",IN_LDPTAB[indx].proctag,IN_LDPTAB[indx].pid,indx));
  printf("INdeath():process died: \"%s\"\n\tpid = %d indx = %d\n",
         IN_LDPTAB[indx].proctag,IN_LDPTAB[indx].pid,indx);

	/*
	 * In any event, change the proc's state and clear its sanity
	 * timer...restart timer is taken care of below (or in INinitptab())
	 */
	IN_SDPTAB[indx].procstate = IN_DEAD;
	INCLRTMR(INproctmr[indx].sync_tmr);
	
	// Check if pdeath should be sent
	if(IN_SDPTAB[indx].ecode == IN_KILLPROC && 
	   IN_SDPTAB[indx].ireq_lvl == SN_NOINIT){
		send_pdeath = FALSE;
	}

	if (IN_LDPTAB[indx].pid <= 1) {
		/*
		 * Invalid PID...don't try to kill UNIX
		 * proc0 or proc1!!!
		 */
		oldpid = IN_FREEPID;	/* garbage pid for INpDeath msg */
		//INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INdeath(): process \"%s\" found with invalid PID %d\n\tprevented killing it",IN_LDPTAB[indx].proctag,IN_LDPTAB[indx].pid));
    printf("INdeath(): process \"%s\" found with invalid PID %d\n\tprevented killing it\n",
           IN_LDPTAB[indx].proctag,IN_LDPTAB[indx].pid);
	}	

	else {
		/* OK, blast away */
		oldpid = IN_LDPTAB[indx].pid;	/* save pid for INpDeath msg */
		// negative pid sends the SIGKILL to all processes in the
		// process group of the pid.
		(Void)INkill(indx, -SIGKILL);
	}

	IN_LDPTAB[indx].pid = IN_FREEPID;
	IN_LDPTAB[indx].tid = IN_FREEPID;

	/* Take care of core file processing: */
	INchkcore(IN_LDPTAB[indx].proctag);


	if(IN_LDSTATE.systep == IN_CLEANUP){
		IN_SDPTAB[indx].procstep = INV_STEP;
		IN_LDPTAB[indx].syncstep = INV_STEP;
		/* Remove all temporary processes with restart inhibited */
		if(IN_LDPTAB[indx].startstate == IN_INHRESTART &&
		   IN_LDPTAB[indx].permstate == IN_TEMPPROC){
			INdeadproc(indx,TRUE);
		}
		/* Free the shared memory that was requested to be released 
		** but only if initlevel is > then 2
		*/
		if(IN_LDSTATE.sn_lvl > SN_LV2){
			INfreeshmem(indx,FALSE);
		}
		/* Ignore all process deaths if in IN_CLEANUP system step 	*/
		return;
	}


	strcpy(proctag,IN_LDPTAB[indx].proctag);
	permstate = IN_LDPTAB[indx].permstate;
	updstate = IN_LDPTAB[indx].updstate;

	/* If the process is being SU change its state to indicate 
	** it exited for the SU.
	*/

	if(IN_SDPTAB[indx].procstep == IN_SU){
		//IN_SDPTAB[indx].procstep = IN_ESU;
		//if(updstate == UPD_PRESTART){
		//	//CR_PRM(POA_INF,"REPT INIT DETECTED THAT %s EXITED FOR FIELD UPDATE",IN_LDPTAB[indx].proctag);
    //  printf("REPT INIT DETECTED THAT %s EXITED FOR FIELD UPDATE\n",
    //         IN_LDPTAB[indx].proctag);
		//} 
		///* Find out what init level is the SU of this process scheduled to be */
		//int su_idx;
		//for(su_idx = 0; su_idx < SU_MAX_OBJ; su_idx++){
		//	if(strcmp(IN_LDPTAB[indx].pathname, INsudata[su_idx].obj_path) == 0){
		//		break;
		//	}
		//}
	  //
		//if(su_idx < SU_MAX_OBJ){
		//	sn_lvl = INsudata[su_idx].sn_lvl;
		//} else {
		//	//INIT_ERROR(("Process death during SU while %s not in INsudata",IN_LDPTAB[indx].proctag));
    //  printf("Process death during SU while %s not in INsudata\n",
    //         IN_LDPTAB[indx].proctag);
		//	sn_lvl = SN_LV0;
		//}
		//if(send_pdeath){
		//	class INlpDeath pdeath(proctag,permstate, TRUE, TRUE, oldpid,sn_lvl);
		//	(Void)pdeath.broadcast(INmsgqid, INbrdcast_delay, MHallClusters);
		//}
		return;
	} else {
		IN_SDPTAB[indx].procstep = INV_STEP;
		IN_LDPTAB[indx].syncstep = INV_STEP;
	}

	/*
	 * Check to see if this process has requested a specific recovery
	 */
	if(IN_SDPTAB[indx].ireq_lvl != SN_NOINIT) {
		INescalate(IN_SDPTAB[indx].ireq_lvl,IN_SDPTAB[indx].ecode,
               IN_LDPTAB[indx].source,indx);
	} else {
		if(IN_LDPTAB[indx].startstate == IN_INHRESTART){
			/* If restart was inhibited, do not print alarms */
			a_lvl = POA_INF;
		} else if(IN_LDPTAB[indx].source == IN_MANUAL){
			a_lvl = POA_MAN;
		} else if(IN_LDPTAB[indx].proc_category == IN_CP_CRITICAL ||
              IN_LDPTAB[indx].proc_category == IN_PSEUDO_CRITICAL) {
			/* If restart allowed and process is critical print
			** critical alarm
			*/
			a_lvl = POA_CRIT;
		}
		if(send_pdeath){
			// If process was supposed to die silently, do not print this
			if(a_lvl == POA_INF || a_lvl == POA_MAN){
				//CR_PRM(a_lvl, "REPT INIT DETECTED %s DIED",IN_LDPTAB[indx].proctag);
        printf("REPT INIT DETECTED %s DIED\n",IN_LDPTAB[indx].proctag);
				IN_SDPTAB[indx].alvl = POA_INF;
			} else {
				//CR_X733PRM(a_lvl, IN_LDPTAB[indx].proctag, qualityOfServiceAlarm, 
        //           softwareProgramAbnormallyTerminated, NULL,";201", 
        //           "REPT INIT DETECTED %s DIED",IN_LDPTAB[indx].proctag);
        printf("REPT INIT DETECTED %s DIED\n",IN_LDPTAB[indx].proctag);
				IN_SDPTAB[indx].alvl = a_lvl;
			}
		}
		// Default to level 0 initialization
		INescalate(SN_LV0,INPROC_DEATH,IN_SOFT,indx);
	}

	if(INetype != EHTMRONLY) {
		/*
		 * Need to broadcast a message indicating that this
		 * process has died.
		 */
		if(IN_VALIDPROC(indx)){
			rstrt_flg = TRUE;
		} else {
			rstrt_flg = FALSE;
		}

		/* Get the level of initialization this process is scheduled for */
		if(IN_LDPTAB[indx].syncstep == INV_STEP){
			/* Process scheduled for delayed restart, processes
                                          ** will be initialized at least at level 1 following
                                          ** delayed restart.
                                          */
			sn_lvl = SN_LV1;
		} else {
			sn_lvl = IN_LDPTAB[indx].sn_lvl;
		}

		if(send_pdeath){
			class INlpDeath pdeath(proctag,permstate, rstrt_flg,
                             ((updstate == NO_UPD)?FALSE:TRUE), oldpid, sn_lvl);
			(Void)pdeath.broadcast(INmsgqid, INbrdcast_delay, MHallClusters);
		}
	}

	//INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INdeath():returning after scheduling process \"%s\" for restart",IN_LDPTAB[indx].proctag));
  printf("INdeath():returning after scheduling process \"%s\" for restart\n",
         IN_LDPTAB[indx].proctag);
	return;
}

/*
** NAME:
**	INreqinit()
**
** DESCRIPTION:
**	This function handles the processing required upon the detection
**	of a process sanity timeout.  If the process is allowed to be
**	restarted/re-initialized, then it will be cycled through a
**	process re-init.
**
** INPUTS:
**	sn_lvl	- Level of requested initialization
**	indx	- Process index in shared memory
**	err_code- error code fore the reason of initialization
**	source  - source of initialization (MANUAL, AUTO)
**	string	- descriptive string for printout
**
** RETURNS:
**
** CALLS:
**	INsetrstrt() - Called to schedule the process for re-initialization
**	INescalate() - To cause escalation if necessary
**
** CALLED BY:
**
** SIDE EFFECTS:
*/

GLretVal
INreqinit(SN_LVL sn_lvl,U_short indx, GLretVal err_code, IN_SOURCE source, const char * string)
{

	//INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INreqinit():entered for process \"%s\"\n\tsn_lvl = %d err_code = %d source = %s",IN_LDPTAB[indx].proctag,sn_lvl,err_code,IN_SNSRCNM(source)));
  printf("INreqinit():entered for process \"%s\"\n\tsn_lvl = %d err_code = %d source = %s\n",
         IN_LDPTAB[indx].proctag,sn_lvl,err_code,IN_SNSRCNM(source));

	CRALARMLVL alarm_lvl = POA_MAJ;
	
  switch(sn_lvl){
  case SN_LV0:
  case SN_LV1:
    if(IN_LDPTAB[indx].proc_category == IN_CP_CRITICAL ||
       IN_LDPTAB[indx].proc_category == IN_PSEUDO_CRITICAL){ 
      alarm_lvl = POA_CRIT;
    }
    break;
  case SN_LV2:
  case SN_LV3:
  case SN_LV4:
  case SN_LV5:
  default:        /* Do not accept invalid initialization level */
		//INIT_ERROR(("Invalid sn_lvl requested %s",IN_SNLVLNM(sn_lvl)));
    printf("Invalid sn_lvl requested %s\n",IN_SNLVLNM(sn_lvl));
    return(GLfail);
  }

	if(indx == (U_short)INIT_INDEX || indx > IN_SNPRCMX){
		//INIT_ERROR(("Invalid process index %d",indx));
    printf("Invalid process index %d\n",indx);
		return(GLfail);
	}
 
  /* Report that an initialization has been requrested */
	if(source == IN_MANUAL){
		alarm_lvl = POA_MAN;
    //CR_PRM(alarm_lvl,"REPT INIT REQUESTED %s INIT OF %s DUE TO %s",
    //       IN_SNLVLNM(sn_lvl),IN_LDPTAB[indx].proctag,string);
    printf("REPT INIT REQUESTED %s INIT OF %s DUE TO %s\n",
           IN_SNLVLNM(sn_lvl),IN_LDPTAB[indx].proctag,string);
	
	} else {
		//CR_X733PRM(alarm_lvl, IN_LDPTAB[indx].proctag, qualityOfServiceAlarm, 
    //           softwareProgramAbnormallyTerminated, NULL,";201", 
    //           "REPT INIT REQUESTED %s INIT OF %s DUE TO %s",
    //           IN_SNLVLNM(sn_lvl),IN_LDPTAB[indx].proctag,string);
    printf("REPT INIT REQUESTED %s INIT OF %s DUE TO %s\n",
           IN_SNLVLNM(sn_lvl),IN_LDPTAB[indx].proctag,string);
		IN_SDPTAB[indx].alvl = alarm_lvl;
	}
        
	// Check software check inhibits if source is not manual
	if((source != IN_MANUAL) && ((IN_LDPTAB[indx].softchk == IN_INHSOFTCHK ||
                                IN_LDSTATE.softchk == IN_INHSOFTCHK))){
		/* Indicate that the initialization failed so other processes
		** can be advanced.
		*/
		IN_LDPTAB[indx].failed_init = TRUE;
		INnext_rlvl();
		return(GLfail);
	}
	
	IN_LDPTAB[indx].source = source;

	if(IN_SDPTAB[indx].procstate == IN_DEAD){
		// Process should not be in the IN_DEAD state unless this is
		// manual initialization.  If process is DEAD go through
		// INescalate().
		if(source == IN_MANUAL){
			/* remove the process if restart inhibited */
			if(IN_LDPTAB[indx].startstate == IN_INHRESTART){
				INdeadproc(indx,TRUE);
				return(GLsuccess);
			}
			// If the process was in dead state, and it was non-critical
			// its shared memory was removed, so always set initialization
			// level to 4, otherwise it will always fail to initialize
			if(IN_LDPTAB[indx].proc_category == IN_NON_CRITICAL){
				SN_LVL	tmplvl;
				if(INevent.onLeadCC() == TRUE){
					tmplvl = SN_LV5;
				} else {
					tmplvl = SN_LV4;
				}
				//CR_PRM(POA_INF,"REPT INIT CHANGED MANUAL %s INIT OF DEAD PROCESS %s TO %s",
        //       IN_SNLVLNM(sn_lvl),IN_LDPTAB[indx].proctag, IN_SNLVLNM(tmplvl));
        printf("REPT INIT CHANGED MANUAL %s INIT OF DEAD PROCESS %s TO %s",
               IN_SNLVLNM(sn_lvl),IN_LDPTAB[indx].proctag, IN_SNLVLNM(tmplvl));
				INsetrstrt(tmplvl,indx,source);
			} else {
				INsetrstrt(sn_lvl,indx,source);
			}
		} else {
			INescalate(sn_lvl,err_code,source,indx);
		}
	} else {
		// Do not change the level on manual request, even if it
		// de-escalates the initialization.
		// On other requests ask for a level no lower then current
		// level.
		if(source != IN_MANUAL){
			if(IN_LDPTAB[indx].sn_lvl >= sn_lvl){
				sn_lvl = IN_LDPTAB[indx].sn_lvl;
			}
		}

		IN_LDPTAB[indx].sn_lvl = sn_lvl;

		// Never request more than level 1 inits
		if(sn_lvl > SN_LV1){
			sn_lvl = SN_LV1;
		}

		// Set this up as if the process requested initialization
		IN_SDPTAB[indx].ireq_lvl = sn_lvl;
		IN_SDPTAB[indx].ecode = err_code;
#ifndef EES
		pid_t	createpid;
		if(IN_procdata->debug_timer > 0 && source != IN_MANUAL){
			int	timeout = (IN_procdata->debug_timer / 20) + 1;
#ifdef __sun
      if((createpid = fork1()) == 0){
#else
        if((createpid = fork()) == 0){
#endif  
          char	pid[20];
          char	ecode[20];
          // more efficient conversion functions exist but sprintf
          // works both in linux and solaris and efficiency is not
          // a big deal here
          sprintf(pid, "%d", IN_LDPTAB[indx].pid);
          sprintf(ecode, "%d", err_code);
          execl("/sn/init/debug", "debug", IN_LDPTAB[indx].proctag, pid, ecode, (char*) 0);
          _exit(1);
        } else if(createpid != ((pid_t) -1)){
          // wait for script to finish up to timeout value
          int	pstatus;
          while(timeout > 0 && waitpid(createpid, &pstatus, WNOHANG) != createpid){
            IN_SLEEPN(0,20000000);
            timeout --;
          }
        }
      }
#endif
      INsync(indx,IN_BCLEANUP);
      INworkflg = TRUE;
      if(IN_LDSTATE.initstate != INITING){
        /* Set fast poll timer to quickly detect process death if
        ** INIT is not the parent.
        */
        INsettmr(INpolltmr,INPROCPOLL,(INITTAG | INPOLLTAG),TRUE,TRUE);
      }
    }

    //INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INreqinit():\n\treturning after scheduling process \"%s\" for initialization",IN_LDPTAB[indx].proctag));
    printf("INreqinit():\n\treturning after scheduling process \"%s\" for initialization\n",
           IN_LDPTAB[indx].proctag);
    return(GLsuccess);
  }

/*
** NAME:
**	INsetrstrt()
**
** DESCRIPTION:
**	This function handles the processing required upon the detection
**	of a process sanity timeout.  If the process is allowed to be
**	restarted/re-initialized, then it will be cycled through a
**	process re-init/restart.
**
** INPUTS:
**	sn_lvl	- requested initialization level
**	indx	- index of the process to be initialized
**
** RETURNS:
**	GLfail - if restart did not occur
**	GLsuccess - otherwise
**
** CALLS:
**	INSETTMR - INIT's timing library is accessed to set/clear process-
**
** CALLED BY:
**
** SIDE EFFECTS:
*/

  GLretVal
     INsetrstrt(SN_LVL sn_lvl, U_short indx,IN_SOURCE source)
  {
    CRALARMLVL  alvl = POA_INF;

    //INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INsetrstrt():entered indx %d, process \"%s\" sn_lvl = %s",indx, IN_LDPTAB[indx].proctag,IN_SNLVLNM(sn_lvl)));
    printf("INsetrstrt():entered indx %d, process \"%s\" sn_lvl = %s\n",
           indx, IN_LDPTAB[indx].proctag,IN_SNLVLNM(sn_lvl));

    if(IN_INVPROC(indx)){
      //INIT_ERROR(("Invalid process entry"));
      printf("Invalid process entry\n");
      return(GLfail);
    }

    if(IN_SDPTAB[indx].procstate != IN_DEAD){
      //INIT_ERROR(("Invalid process state %s",IN_PROCSTNM(IN_SDPTAB[indx].procstate)));
      printf("Invalid process state %s\n",IN_PROCSTNM(IN_SDPTAB[indx].procstate));
      return(GLfail);
    }

    // Do not try to restart processes that should not be in the system
    if(IN_LDSTATE.run_lvl < IN_LDPTAB[indx].run_lvl){
      //INIT_ERROR(("Proc %s run_lvl %d > sys run_lvl %d",IN_LDPTAB[indx].proctag,IN_LDPTAB[indx].run_lvl,IN_LDSTATE.run_lvl));
      printf("Proc %s run_lvl %d > sys run_lvl %d\n",
             IN_LDPTAB[indx].proctag,IN_LDPTAB[indx].run_lvl,IN_LDSTATE.run_lvl);
      return(GLfail);
    }

    /* Reset the source to insure no repeated manual inits 
     */
    IN_LDPTAB[indx].source = IN_SOFT;

    // If this is manual request, clear the restart counts
    // Do not count manual request in total restart count
    // Also clear next_rstrt interval
    if(source == IN_MANUAL){
      IN_LDPTAB[indx].rstrt_cnt = 0;
      IN_LDPTAB[indx].next_rstrt = 0;
      INCLRTMR(INproctmr[indx].rstrt_tmr);
      /* If software updated process is manually inited prior to
      ** commit, the new version of process should be restarted.
      */
      if (IN_LDPTAB[indx].updstate == UPD_POSTSTART) {
        IN_LDPTAB[indx].updstate = UPD_PRESTART;
      }
      alvl = POA_MAN;
    } else {
      if(INic_crit_up(indx) == FALSE && IN_LDSTATE.initstate != INITING){
        //INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INsetrstrt(): not all critical procs up"));
        printf("INsetrstrt(): not all critical procs up\n");
      }
      IN_LDPTAB[indx].tot_rstrt++;
    }

    IN_LDPTAB[indx].syncstep = IN_LDSTATE.systep;

    if(IN_LDSTATE.systep != IN_STEADY){
      if(IN_LDPTAB[indx].permstate == IN_TEMPPROC){
        //INIT_ERROR(("Temp proc %s initialized when systep is %s",IN_LDPTAB[indx].proctag,IN_SQSTEPNM(IN_LDSTATE.systep)));
        printf("Temp proc %s initialized when systep is %s\n",
               IN_LDPTAB[indx].proctag,IN_SQSTEPNM(IN_LDSTATE.systep));
        return(GLfail);
      } 
    } else if(IN_LDPTAB[indx].run_lvl > IN_LDSTATE.sync_run_lvl){
			// Do not set sync step to steady for a process not yet
			// ready to run
			IN_LDPTAB[indx].syncstep = IN_SYSINIT;
    } else if(IN_LDPTAB[indx].run_lvl == IN_LDSTATE.sync_run_lvl){
			IN_LDPTAB[indx].syncstep = IN_PROCINIT;
    }

    IN_LDPTAB[indx].sn_lvl = sn_lvl;
    IN_LDPTAB[indx].gqCnt = 0;
    IN_SDPTAB[indx].ireq_lvl = SN_NOINIT;
    IN_SDPTAB[indx].procstep = INV_STEP;
    IN_SDPTAB[indx].procstate = IN_NOEXIST;
    IN_SDPTAB[indx].error_count = 0;
    IN_SDPTAB[indx].progress_mark = 0;
    IN_SDPTAB[indx].progress_check = 0;
    IN_SDPTAB[indx].count = 0;
	
    /* Free process shared memory allocated with release flag set
    ** if sn_lvl > 0
    */
    if(sn_lvl > SN_LV0){
      INfreeshmem(indx,FALSE);
    }

    /*
     * The death of the old version of an updating process shouldn't
     * increment system error counts. However it does require that
     * the polling timer be set to quickly restart the new version.
     * Do nothing about the SU status if the process exited intentionally
     * NOTE: this code relies on IN_SDPTAB[indx].ecode not being reset
     */
    if(IN_SDPTAB[indx].ecode != IN_INTENTIONAL){
      if (IN_LDPTAB[indx].updstate == UPD_PRESTART) {
        //CR_PRM(alvl,"REPT INIT STARTING NEW VERSION OF \"%s\" ",
        //       IN_LDPTAB[indx].proctag);
        printf("REPT INIT STARTING NEW VERSION OF \"%s\" \n",
               IN_LDPTAB[indx].proctag);
      } else if (IN_LDPTAB[indx].updstate == UPD_POSTSTART) {
        //CR_PRM(alvl,"REPT INIT STARTING PREVIOUS VERSION OF \"%s\" ",
        //       IN_LDPTAB[indx].proctag);
        printf("REPT INIT STARTING PREVIOUS VERSION OF \"%s\" \n",
               IN_LDPTAB[indx].proctag);
        IN_LDPTAB[indx].updstate = NO_UPD;
      }
    } else {
      //INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INsetrstrt():received intentional exit"));
      printf("INsetrstrt():received intentional exit\n");
    }
	
    // Clear the error code
    IN_SDPTAB[indx].ecode = GLsuccess;

    if(IN_LDPTAB[indx].print_progress){
      //CR_PRM(alvl,"REPT INIT STARTING INIT OF PROCESS %s AT %s",
      //       IN_LDPTAB[indx].proctag,IN_SNLVLNM(sn_lvl));
      printf("REPT INIT STARTING INIT OF PROCESS %s AT %s\n",
             IN_LDPTAB[indx].proctag,IN_SNLVLNM(sn_lvl));
    }
    INworkflg = TRUE;

    /* Set poll timer if not already set */
    if(INpolltmr.tindx < 0){
      INsettmr(INpolltmr,INPROCPOLL,(INITTAG | INPOLLTAG),TRUE,TRUE);
    }

    //INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INsetrstrt():successful return"));
    printf("INsetrstrt():successful return\n");
    return(GLsuccess);
  }

/*
** NAME:
**	INdeadproc()
**
** DESCRIPTION:
**	This process performs the appropriate clean up actions when a process
**	dies or misses its sanity timer and is not to be restarted/reinited
**
** INPUTS:
**	indx	- process index
**	remove	- true if process should be permamently removed
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
     INdeadproc(U_short indx,Bool remove)
  {
    //INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INdeadproc(): entered w/indx %d, proc %s, remove = %d",indx,IN_LDPTAB[indx].proctag,remove));
    printf("INdeadproc(): entered w/indx %d, proc %s, remove = %d\n",
           indx,IN_LDPTAB[indx].proctag,remove);

    /*
     * Process is not to be reinited, make sure it's
     * dead:
     */
    IN_SDPTAB[indx].procstate = IN_DEAD;
    INCLRTMR(INproctmr[indx].sync_tmr);
    INCLRTMR(INproctmr[indx].gq_tmr);
    IN_LDPTAB[indx].gqsync = IN_MAXSTEP;

    /*
     * Don't try to kill UNIX
     * proc0 or proc1!!!
     */
    if (IN_LDPTAB[indx].pid > 1) {
      /* OK, blast away */
      (Void)INkill(indx, -SIGKILL);
    }

    IN_LDPTAB[indx].pid = IN_FREEPID;
    IN_LDPTAB[indx].rstrt_cnt = 0;
    IN_SDPTAB[indx].error_count = 0;
    IN_LDPTAB[indx].sn_lvl = SN_NOINIT;
    INCLRTMR(INproctmr[indx].rstrt_tmr);

    // Deallocate shared memory only for NON-CRITICAL processes

    if (IN_LDPTAB[indx].proc_category == IN_NON_CRITICAL) {
      /*
       * Automatically deallocate shared memory segments and
       * semaphores associated with non-critical processes
       * 
       */
      INfreeshmem(indx,TRUE);
      INfreesem(indx);
    } else {

      /* For all other processes, release any memory they allocated with
      ** the release option.
      */
      INfreeshmem(indx,FALSE);
    }

    if(remove == TRUE ||
       (IN_LDPTAB[indx].proc_category == IN_NON_CRITICAL && 
        IN_LDPTAB[indx].permstate == IN_TEMPPROC && (!IN_LDPTAB[indx].third_party))){
      /*
       * Now, "delete" the process from the
       * process tables. Also, unconditonally delete all temporary
       * non-critical processes. 
       */
      if(IN_LDPTAB[indx].print_progress){
        //CR_PRM(POA_INF,"REPT INIT %s REMOVED FROM SYSTEM", IN_LDPTAB[indx].proctag);
        printf("REPT INIT %s REMOVED FROM SYSTEM\n",
               IN_LDPTAB[indx].proctag);
      }
      INinitptab(indx);
    } else {
      // Schedule the process for delayed restart
      if(IN_LDPTAB[indx].next_rstrt  < 300) {
        IN_LDPTAB[indx].next_rstrt = 300;  // Wait 5 minutes
      } else if(IN_LDPTAB[indx].next_rstrt < 900){
        IN_LDPTAB[indx].next_rstrt = 900;  // Wait 15 minutes
      } else if(IN_LDPTAB[indx].next_rstrt != IN_NO_RESTART){
        IN_LDPTAB[indx].next_rstrt = 1800;  // Keep trying every 30 minutes
      } 
		
      // Indicate that we should give up trying to bring up this
      // process for a while
      IN_LDPTAB[indx].syncstep = INV_STEP;
      if(IN_LDPTAB[indx].next_rstrt != IN_NO_RESTART){
        INSETTMR(INproctmr[indx].sync_tmr,IN_LDPTAB[indx].next_rstrt,
                 (INPROCTAG | INSYNCTAG | indx), FALSE);

        //CR_PRM(POA_INF,"REPT INIT %s SCHEDULED FOR RESTART IN %d MIN", IN_LDPTAB[indx].proctag,IN_LDPTAB[indx].next_rstrt/60);
        printf("REPT INIT %s SCHEDULED FOR RESTART IN %d MIN\n",
               IN_LDPTAB[indx].proctag,IN_LDPTAB[indx].next_rstrt/60);
      } else {
        INCLRTMR(INproctmr[indx].sync_tmr);
        //CR_PRM(POA_INF,"REPT INIT %s WILL NOT BE RESTARTED", IN_LDPTAB[indx].proctag);
        printf("REPT INIT %s WILL NOT BE RESTARTED\n", IN_LDPTAB[indx].proctag);
      }
    }


    //INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INdeadproc(): returned successfully"));
    printf("INdeadproc(): returned successfully\n");
    return;
  }

  int
     INkill(U_short indx, int sig)
  {
    if(IN_LDPTAB[indx].third_party){
      if(IN_LDPTAB[indx].pid > 1){
        // If third party process is in the middle of execution
        // kill it too
        if(sig < 0){
          kill(-IN_LDPTAB[indx].pid, -sig);
        } else {
          kill(IN_LDPTAB[indx].pid, sig);
        }
        if(IN_LDPTAB[indx].tid == IN_FREEPID && sig == 0){
          return(kill(IN_LDPTAB[indx].pid, 0));
        }
      }
      if(IN_LDPTAB[indx].tid == IN_FREEPID){
        return(GLsuccess);
      }
      if(sig == 0){
        if(IN_SDPTAB[indx].procstep != IN_ECLEANUP){
          return(thr_kill(IN_LDPTAB[indx].tid, 0));
        } else {
          return(-1);
        }
      } else {
        IN_SDPTAB[indx].procstate = IN_RUNNING;
        IN_SDPTAB[indx].procstep = IN_ECLEANUP;
        return(thr_kill(IN_LDPTAB[indx].tid, SIGTERM));
      }
    } else {
      if(sig < 0){
        return(kill(-IN_LDPTAB[indx].pid, -sig));
      } else {
        return(kill(IN_LDPTAB[indx].pid, sig));
      }
    }	
  }

#define IN_SHUT_WAIT	(IN_procdata->shutdown_timer) 
  extern  GLretVal	INthirdPartyExec(U_short, IN_SYNCSTEP);

  extern int INsnstop;
/*
** NAME:
**	INkillprocs()
**
** DESCRIPTION:
**	This routine kills all the processes in the process table with
**	a SIGKILL.
**
** INPUTS:
**	sendSigterm - TRUE if SIGTERM should be sent
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
     INkillprocs(Bool sendSigterm)
  {
    U_short i;

    //INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INkillprocs(): entered"));
    printf("INkillprocs(): entered\n");

    // Ignore signals
    (Void) signal(SIGCLD,SIG_IGN);

    /* If backout process was running, kill it too */
    if(IN_LDBKPID != IN_FREEPID && IN_LDBKPID > 1){
      kill(IN_LDBKPID,SIGKILL);
      IN_LDBKPID = IN_FREEPID;
    }

    Bool	found_alive;

    for(int nTry = 0; nTry < 2; nTry++){
      int	waittime;
      found_alive = TRUE;

      if(nTry > 0){
        waittime = 80;
        // Do shutdown
#ifndef EES
        if(sendSigterm && !INcmd && IN_LDCURSTATE == S_LEADACT){  
#ifdef __sun
          INShut = FALSE;
          thr_create(NULL, thr_min_stack()+32000, INshutOracle, NULL, THR_BOUND, NULL);

          /* Wait for a while for Oracle shutdown to complete */
          for(i = 0; i < IN_SHUT_WAIT; i++){
            INsanityPeg++;
            IN_SLEEP(1);  
            if(INShut == TRUE){
              //CR_PRM(POA_INF,"REPT INIT SHUTDOWN SCRIPT COMPLETED");
              printf("REPT INIT SHUTDOWN SCRIPT COMPLETED\n");
              break;
            }       
          }       
                        
          if(i >= IN_SHUT_WAIT){
            //CR_X733PRM(POA_TMN,"SHUTDOWN", qualityOfServiceAlarm, softwareProgramError, NULL, ";207", "REPT INIT SHUTDOWN SCRIPT TIMED OUT");
            printf("REPT INIT SHUTDOWN SCRIPT TIMED OUT\n");
          } 
#endif
#ifdef __linux
          pid_t     ret;
          if((ret = fork()) == 0){
            static int type = INscriptsStop;
            (Void)setpgrp();
            //mutex_unlock(&CRlockVarible);
            (Void) sigset(SIGCLD,INsigcld);
            //CR_PRM(POA_INF, "REPT INIT RUNNING STOP SCRIPTS");
            printf("REPT INIT RUNNING STOP SCRIPTS\n");
            IN_LDSCRIPTSTATE = INscriptsRunning;
            INrunScriptList(&type);
            exit(0);
          } else if(ret < 0){
            //CR_PRM(POA_INF, "REPT INIT ERROR FAILED TO FORK STOP PROCESS, RET %d", ret);
            printf("REPT INIT ERROR FAILED TO FORK STOP PROCESS, RET %d\n", ret);
            IN_LDSCRIPTSTATE = INscriptsFailed;
          }


          /* Wait for a while for stop scripts to complete */
          for(i = 0; i < ((IN_SHUT_WAIT) * 4); i++){
            INsanityPeg++;
            IN_SLEEPN(0,250000000);  
            if(IN_LDSCRIPTSTATE != INscriptsRunning){
              if(IN_LDSCRIPTSTATE == INscriptsFinished){
                //CR_PRM(POA_INF,"REPT INIT STOP SCRIPTS COMPLETED SUCCESSFULLY");
                printf("REPT INIT STOP SCRIPTS COMPLETED SUCCESSFULLY\n");
              } else {
                //CR_PRM(POA_INF,"REPT INIT STOP SCRIPTS FAILED");
                printf("REPT INIT STOP SCRIPTS FAILED\n");
              }
              break;
            }       
          }       
                        
          if(i >= IN_SHUT_WAIT){
            //CR_X733PRM(POA_TMN,"SHUTDOWN", qualityOfServiceAlarm, softwareProgramError, NULL, ";207", "REPT INIT STOP SCRIPTS TIMED OUT");
            printf("REPT INIT STOP SCRIPTS TIMED OUT\n");
            kill(-ret, SIGKILL);
          }
#endif
        }
#endif  

			
      } else {
        if(INsnstop){
          waittime = 15;
        } else {
          if(INvhostMate >= 0 && INevent.isVhostActive()){
            waittime = 3;
          } else {
            waittime = 20;
          }
        }
      }

      /* Send SIGTERM to all processes in case some of them catch it.
      ** Do it for the processes in above first run lvl fist
      ** then send it to the proceses below first runlvl
      */
      for (i = 0; i < IN_SNPRCMX && sendSigterm; i++) {
        if (IN_VALIDPROC(i) && (IN_LDPTAB[i].pid > 1 || IN_LDPTAB[i].tid != IN_FREEPID) && 
            IN_SDPTAB[i].procstate != IN_DEAD && (nTry > 0 ||
                                                  IN_LDPTAB[i].run_lvl > IN_LDSTATE.first_runlvl)) {
          if(INcmd && nTry > 0){
            // This is /etc/Astop do not allow
            // disks to be unmounted
            if(IN_LDPTAB[i].third_party == FALSE){	
              INkill(i,SIGKILL);
            } else if(IN_LDPTAB[i].tid != IN_FREEPID){
              INthirdPartyExec(i, IN_CLEANUP);
              IN_LDPTAB[i].tid = IN_FREEPID;
            }
          } else {
            if(IN_LDPTAB[i].third_party == FALSE){
              INkill(i,SIGTERM);
            } else if(IN_LDPTAB[i].tid != IN_FREEPID && (!INcmd)){
              IN_SDPTAB[i].procstep = IN_BCLEANUP;
              thr_kill(IN_LDPTAB[i].tid, SIGTERM);
              IN_LDPTAB[i].tid = IN_FREEPID;
            }
          }
        }
      }


      // Wait for processes to die 
      for(int j = 0; j < waittime && found_alive == TRUE && sendSigterm; j++){
        /* Get all the zombies */
        while(INcheck_zombie() > 0);
        INsanityPeg++;
        IN_SLEEP(1); 
        found_alive = FALSE;
        for(i = 0; i < IN_SNPRCMX; i++){
          if (IN_VALIDPROC(i) && IN_SDPTAB[i].procstate != IN_DEAD &&
              IN_LDPTAB[i].pid > 1) {
            if(INkill(i,0) < 0){
              // Do not clear pid since we will want to
              // kill all members of this process's process group
              IN_SDPTAB[i].procstate = IN_DEAD;
            } else if((nTry > 0 && IN_LDPTAB[i].run_lvl <= IN_LDSTATE.first_runlvl) 
                      || (nTry == 0 &&IN_LDPTAB[i].run_lvl > IN_LDSTATE.first_runlvl)){
              found_alive = TRUE;
              break;
            }
          }
        }
      }

      /* blast any process still alive and all processes in their process group */
      for (i = 0; i < IN_SNPRCMX; i++) {

        if (IN_VALIDPROC(i)) {
	
          if (IN_LDPTAB[i].pid <= 1) {
            /*
             * Invalid PID...don't try to kill UNIX
             * proc0 or proc1!!!
             */
            //INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INkillprocs(): proc %s already dead",IN_LDPTAB[i].proctag));
            printf("INkillprocs(): proc %s already dead\n",
                   IN_LDPTAB[i].proctag);
          } else {
            // If the process did not exit, report on it
            // Do not print this PRM if Astop, it can hang in MSGH
            if(!INcmd && sendSigterm && IN_SDPTAB[i].procstate != IN_DEAD){
              /*		CR_PRM(POA_INF,"REPT INIT KILLED %s DURING SHUTDOWN",IN_LDPTAB[i].proctag); */
            }
            if(nTry > 0 || IN_LDPTAB[i].run_lvl > IN_LDSTATE.first_runlvl){
              //INIT_DEBUG((IN_DEBUG | IN_RSTRTR),
              //           (POA_INF,"INkillprocs(): killing proc group %s pid %d",
              //            IN_LDPTAB[i].proctag,IN_LDPTAB[i].pid));
              printf("INkillprocs(): killing proc group %s pid %d\n",
                     IN_LDPTAB[i].proctag,IN_LDPTAB[i].pid);

              /* OK, blast away */
              (Void)INkill(i, -SIGKILL);
            }
          }
        }
      }
    }
	
    CRALARMLVL alarm_lvl;
    INsanityPeg ++;
    if(found_alive == TRUE){
      if(sendSigterm){
        IN_SLEEP(5);
      } else {
        IN_SLEEP(2);
      }
      /* Get all the zombies */
      while(INcheck_zombie() > 0);
    }

    for (i = 0; i < IN_SNPRCMX; i++) {
      if(IN_VALIDPROC(i) && (IN_LDPTAB[i].pid > 1) &&
         (INkill(i,0) >= 0)){
        // Process still did not die, all we can do here
        // is report.  UNIX boot may be called for in the
        // future, for now just print a message and leave
        // it for manual recovery
        if(IN_LDPTAB[i].proc_category == IN_NON_CRITICAL){
          alarm_lvl = POA_MAJ;
        } else {
          alarm_lvl = POA_CRIT;
        }
        if(!INcmd){
          //CR_PRM(alarm_lvl,"REPT INIT %s DID NOT DIE DURING SYSTEM SHUTDOWN",IN_LDPTAB[i].proctag);
          printf("REPT INIT %s DID NOT DIE DURING SYSTEM SHUTDOWN\n",
                 IN_LDPTAB[i].proctag);
        }
      }
      IN_LDPTAB[i].pid = IN_FREEPID;
    }

#ifdef __sun
    // Clear out all the allocated prosessor sets so they do not accumulate
    // during system restarts
    for(i = 0; i < INmaxPsets; i++){
      if(IN_LDPSET[i] >= 0){
        pset_destroy(IN_LDPSET[i]);
        IN_LDPSET[i] = -1;
      }
    }
#endif

    //INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INkillprocs(): exited"));
    printf("INkillprocs(): exited\n");
  }

#ifdef OLD_SU
/*
** NAME:
**	INautobkout()
**
** DESCRIPTION:
**	This function does an automatic backout of updating process(es)
**	when a software update of permanent process(es) fails. It is
**	called in two specific cases:
**	1. If an updating process in the UPD_POSTSTART state dies.
**	2. If a system-wide reset occurs while process(es) are updating.
**
**	INPUTS:
**	manual 	  - TRUE if manual backout is requested, FALSE otherwise
**	sys_reset - TRUE if system reset is causing the backout, FALSE otherwise.
**	INsupresent is set to TRUE if SU is in progress, it is cleared during 
**	commit part of the SU or when backout is completed.
**
** CALLED BY:
**	INsetrstrt() for case 1.
**	INsysreset() for case 2.
*/
  Void
     INautobkout(Bool manual, Bool sys_reset)
  {
    U_short i;
    static CRALARMLVL a_lvl;

    //INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INautobkout(): entered"));
    printf("INautobkout(): entered\n");

    if(INsupresent == FALSE || IN_LDBKOUT == TRUE){	
      /* This function is called out of INescalate after every process
      ** death, so this may happen when SU backout is already in
      ** progress. INautobkout() should only run once per backout.
      ** Also, this function should not run when no SU is present.
      */
      return;	
    }

    /* Change the value of alarm level only if manual is TRUE or FALSE,
    ** leave a_lvl unchanged for any other input.
    */
    if(manual == TRUE){
      a_lvl = POA_MAN;
    } else if (manual == FALSE) {
      a_lvl = POA_MAJ;
    }

    /* Run INrun_bkout() until BKOUT script succeeds or it is abandoned.
    ** If sys_reset is TRUE, INrun_bkout() executes in blocking mode and
    ** returns success when BKOUT script finishes.
    ** If sys_reset is FALSE, INrun_bkout() forks a copy of INIT which
    ** runs the BKOUT script in background and returns success when it 
    ** completes.
    */ 

    if(INrun_bkout(sys_reset) != GLsuccess){
      return;
    }
	
    //CR_PRM(a_lvl,"REPT INIT TERMINATING PROCESS UPDATES - REVERTING TO ORIGINAL PROCESSES");
    printf("REPT INIT TERMINATING PROCESS UPDATES - REVERTING TO ORIGINAL PROCESSES\n");
    if(sys_reset == TRUE){
      /* Make all image files consistent and change
      ** update state to no update.
      */
      SN_LVL  sn_lvl;
      int	j;
      (void) INmvsufiles(IN_SNPRCMX, sn_lvl,INSU_BKOUT);
      for(i = 0; i < IN_SNPRCMX; i++){
        if(IN_VALIDPROC(i)){
          IN_LDPTAB[i].updstate = NO_UPD;
          /* Delete the process from process table if new */
          j = INsudata_find(IN_LDPTAB[i].pathname);
          if(j < SU_MAX_OBJ && INsudata[j].new_obj == TRUE){
            INinitptab(i);
          }
				
        }
      }
		
      if(INinit_su_idx != -1){
        INmvsuinit(INSU_BKOUT);
      }

      IN_LDBKOUT = FALSE;
      IN_LDAQID = MHnullQ;
      IN_LDBQID = MHnullQ;

      (void)INfinish_bkout();
      /* Reread the initlist since it may have changed */
      if (INrdinls(TRUE,FALSE) == GLfail){
        INescalate(SN_LV2,INBADINITLIST,IN_SOFT,INIT_INDEX);
      }

      return;
    }

    Bool	proc_initialized;

    for (i = 0; i < IN_SNPRCMX; i++) {
      /* Skip empty process table entries */
      if (IN_INVPROC(i)){
        continue;
      } 
      /* The process itself may not have been part of the SU
      ** but may have been initialized as part of the SU.
      ** If that already happened, then make sure that this
      ** process gets reinitialized again and the files associated with
      ** it get backed out.
      */
      proc_initialized = FALSE;
		
      int c;

      if((c = INsudata_find(IN_LDPTAB[i].pathname)) < SU_MAX_OBJ){
        /* Only restart the process if already restarted */
        if(INsudata[c].changed == FALSE && 
           IN_LDPTAB[i].run_lvl <= IN_LDSTATE.sync_run_lvl){
          proc_initialized = TRUE;
        }
        SN_LVL sn_lvl;
        if(INsudata[c].new_obj == TRUE){
          /* Update any related files */
          (void)INmvsufiles(i,sn_lvl,INSU_BKOUT);
          /* Delete that process */
          INdeadproc(i,TRUE);	
          continue;
        }
      }
		

      if(IN_LDPTAB[i].updstate != NO_UPD || proc_initialized == TRUE){
        IN_LDBKOUT = TRUE;
        // Print a message if overriding inhibit restart
        if(IN_LDPTAB[i].startstate == IN_INHRESTART){
          //CR_PRM(POA_INF,"REPT INIT AUTOBACKOUT ALLOWING RESTART FOR PROCESS %s",IN_LDPTAB[i].proctag);
          prinf("REPT INIT AUTOBACKOUT ALLOWING RESTART FOR PROCESS %s\n",
                IN_LDPTAB[i].proctag);
        }
        IN_LDPTAB[i].startstate = IN_ALWRESTART;
        /* Clear restart count to prevent escalations */
        IN_LDPTAB[i].rstrt_cnt = 0;
        INCLRTMR(INproctmr[i].rstrt_tmr);
        /* If manual backout requested, set reason to
        ** manual to avoid alarmed messages.
        */
        if(a_lvl == POA_MAN){
          IN_LDPTAB[i].source = IN_MANUAL;
        } else {
          IN_LDPTAB[i].source = IN_SOFT;
        }
			
        if(IN_LDPTAB[i].updstate == UPD_PRESTART && 
           IN_SDPTAB[i].procstep == IN_BSU){
          /* Simply undo the SU info for processes that did not
          ** yet get SUexitMsg sent to them.
          */
          IN_SDPTAB[i].procstep = IN_STEADY;
          IN_LDPTAB[i].syncstep = IN_STEADY;
          IN_LDPTAB[i].updstate = NO_UPD;
        } else {
          IN_LDPTAB[i].syncstep = IN_BSU;
          if(proc_initialized == FALSE){
            /* Modify updstate to UPD_POSTART for cases were
            ** processes are still in UPD_PRESTART but
            ** already received SUexitMsg message.
            */
            IN_LDPTAB[i].updstate = UPD_POSTSTART;
          }

          /* If a process was in the middle of the apply
          ** just back it out immediately regardless of
          ** any sequencing. This may create dependencies but
          ** the alternative is to let the process wait dead
          ** until it's sequencing order and that would
          ** cause more downtime.
          */
          if(IN_SDPTAB[i].procstep != IN_STEADY){
            INsync(i,IN_SU);
          } else {
            IN_SDPTAB[i].procstep = IN_BSU;
          }
        }
      } 

    }

    if(IN_LDBKOUT == FALSE && INinit_su_idx < 0){
      //INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INautobkout found no processes that are being SU'd"));
      printf("INautobkout found no processes that are being SU'd\n");
      /* In case SU only had CEPs (no permanent process objects)
      ** insure that backout completes correctly.
      */
      IN_LDBKOUT = TRUE;
      INinitover();
      return;
    }

    /* Make sure that the old INIT process gets restarted */
    if(INinit_su_idx >= 0){
      INmvsuinit(INSU_BKOUT);
      IN_LDBKOUT = TRUE;
      IN_LDEXIT = TRUE;	/* Causes INIT to exit from main loop */
    }

    /* Schedule restart work */
    INworkflg = TRUE;
    INsettmr(INpolltmr,INPROCPOLL,(INITTAG|INPOLLTAG), TRUE, TRUE);
    IN_LDSTATE.sync_run_lvl = 0;
    INnext_rlvl();
    return;
  }
#endif

/*
** NAME:
**	INfreeshmem()
**
** DESCRIPTION:
**	This function releases shared memory for a process.  
**
** INPUTS:
**	indx		- process index
**	release_ucl 	- true if all shared memory should be released
**
** RETURNS:
**	None
**
**
*/
  Void
     INfreeshmem(U_short indx, Bool release_ucl)
  {
    int seg_id;
    struct shmid_ds	membuf;

    mutex_lock(&IN_SDSHMLOCK);
    IN_SDSHMLOCKCNT++;

    for (int i = 0; i < INmaxSegs; i++) {
      /* Release allocated shared memory segment only if unconditional
      ** release has been requested or process has allocated this 
      ** segment with the release option enabled.
      */
      INsanityPeg++;
      if (((IN_SDSHMDATA[i].m_pIndex == indx) && (seg_id = IN_SDSHMDATA[i].m_shmid) >= 0) && 
          (release_ucl == TRUE || IN_SDSHMDATA[i].m_rel == TRUE)) {
        if (shmctl(seg_id, IPC_RMID, &membuf) < 0) {
          //INIT_ERROR(("Return %d from \"shmctl()\" deallocating \"%s\"s shared memory segment ID %d, indx %d",errno,IN_LDPTAB[indx].proctag, seg_id, i));
          printf("Return %d from \"shmctl()\" deallocating \"%s\"s shared memory segment ID %d, indx %d\n",
                 errno,IN_LDPTAB[indx].proctag, seg_id, i);
        } else {
          //INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INfreeshmem(): successfully deallocated \"%s\"s shmem seg, ID %d",IN_LDPTAB[indx].proctag, seg_id ));
          printf("INfreeshmem(): successfully deallocated \"%s\"s shmem seg, ID %d\n",
                 IN_LDPTAB[indx].proctag, seg_id );
        }
        memmove(&IN_SDSHMDATA[i], &IN_SDSHMDATA[i + 1], sizeof(INshmemInfo) * (INmaxSegs - i - 1));
        // Adjust the index, otherwise may miss one
        i--;
        IN_SDSHMDATA[INmaxSegs - 1].m_pIndex = -1;
      }
    }
    mutex_unlock(&IN_SDSHMLOCK);

  }

/*
** NAME:
**	INfreesem()
**
** DESCRIPTION:
**	This function releases semaphores for a process.  
**
** INPUTS:
**	indx		- process index
**
** RETURNS:
**	None
**
**
*/
  Void
     INfreesem(U_short indx)
  {
    int sem_id;

    for (int i = 0; i < IN_NUMSEMIDS; i++) {
      if ((sem_id = IN_SDPTAB[indx].semids[i]) >= 0) {
        if (semctl(sem_id, 0, IPC_RMID, 0) < 0) {
          //INIT_ERROR(("Return %d from \"semctl()\" deallocating \"%s\"s semaphore ID %d, indx %d", errno, IN_LDPTAB[indx].proctag, sem_id, i));
          printf("Return %d from \"semctl()\" deallocating \"%s\"s semaphore ID %d, indx %d\n",
                 errno, IN_LDPTAB[indx].proctag, sem_id, i);
        }
        else {
          //INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INfreesem(): successfully deallocated \"%s\"s semaphore set, ID %d",IN_LDPTAB[indx].proctag, sem_id ));
          printf("INfreesem(): successfully deallocated \"%s\"s semaphore set, ID %d\n",
                 IN_LDPTAB[indx].proctag, sem_id);
        }
        IN_SDPTAB[indx].semids[i] = -1;
      }
    }
  }
#ifdef OLD_SU
/*
** NAME:
**	INrun_bkout()
**
** DESCRIPTION:
**	This function controls execution of the BKOUT script
**
** INPUTS:
**	sys_reset	- TRUE if SU backout is part of the system reset
**
** RETURNS:
**	GLsuccess 	- when BKOUT script completes and rest of SU backout can contiue
** 	GLfail		- when BKOUT script is still running
**
**
*/
  GLretVal
     INrun_bkout(Bool sys_reset)
  {


    if(sys_reset == TRUE){
      /* If system reset in progress, execute INsys_bk() in line
      ** without doing concurrent processing.
      ** Disable system monitor since don't really know how long
      ** the backup script will be
      */
      INsanset(0L);
      INsys_bk();

      if((IN_LDARUINT & 0x1) == 0){
        INsanset(IN_LDARUINT * 1000L);
      }
      IN_LDBKPID = IN_FREEPID;
      return(GLsuccess);
    } else {
      /* Execute a backout script in a forked process if
      ** one is not currently executing.
      */

      if(IN_LDBKPID != IN_FREEPID){
        /* make sure that the backout script is truly dead */
        if(kill(IN_LDBKPID,0) < 0){
          /* Backout script completed, return success */
          IN_LDBKPID = IN_FREEPID;
          return(GLsuccess);
        } else {
          return(GLfail);
        }
      } else {
        /* Start executing the backout script */
Short bk_pid;
			if((bk_pid = fork()) > 0){
				/* Old process */
				IN_LDBKPID = bk_pid;
				return(GLfail);
			} else if(bk_pid < 0){
				/* Failed to fork, print a message and 
				** continue the backout anyway.
				*/
				//CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO EXECUTE BKOUT SCRIPT ERRNO = %d",errno);
        printf("REPT INIT ERROR FAILED TO EXECUTE BKOUT SCRIPT ERRNO = %d\n",
               errno);
				return(GLsuccess);
			} else {
				/* Drop priority of the child process */
				nice(5);
				INsys_bk();
				/* Kill all processes in this process group.
				** This is needed to cleanup any shell scripts
				** that still may be running after timeout.
				*/
				if(IN_LDBKPID > 0){
					kill(-IN_LDBKPID,SIGKILL);
				}
				exit(0);
			}
		}
	}
}

#endif

/*
** NAME:
**	INalarm()
**
** DESCRIPTION:
**	This function catches timer alarm
**
** INPUTS:
**
** RETURNS:
**	None
**
**
*/
void
INalarm(int)
{
	//CR_PRM(POA_INF,"REPT INIT ERROR BKOUT SCRIPT TIMED OUT");
  printf("REPT INIT ERROR BKOUT SCRIPT TIMED OUT\n");
	IN_LDBKRET = EINTR;
	kill(-getpid(),SIGKILL); 
	return;
}

#ifdef OLD_SU
/*
** NAME:
**	INsys_bk()
**
** DESCRIPTION:
**	This function executes the BKOUT script
**
** INPUTS:
**	None
**
** RETURNS:
**	None
**
**
*/
Void
INsys_bk()
{
static itimerval timeVal = {{0, 0}, {0, 0}};
char	bkout_time[12];
int	ret;

	char  bkout_file[IN_PATHNMMX];
	strncpy(bkout_file,INsupath,IN_PATHNMMX-6);
	strcat(bkout_file,"/BKOUT");

	//INIT_DEBUG((IN_DEBUG | IN_MSGHTR), (POA_INF,"Entered INsys_bk"));
  prinf("Entered INsys_bk\n");

	int fd;
	if((fd = open(bkout_file,O_RDONLY)) < 0){
		//CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO OPEN BKOUT FILE %s",bkout_file);
    printf("REPT INIT ERROR FAILED TO OPEN BKOUT FILE %s\n",
           bkout_file);
		IN_LDBKRET = GLfail;
		return;
	}

	memset(bkout_time,0,sizeof(bkout_time));

	if(read(fd,(void*)bkout_time,10) < 3 || bkout_time[0] != '#'){
		//CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO READ BKOUT FILE %s",bkout_file);
    printf("REPT INIT ERROR FAILED TO READ BKOUT FILE %s\n",
           bkout_file);
		close(fd);
		IN_LDBKRET = GLfail;
		return;
	}

	signal(SIGALRM,INalarm);

	long	timeout = atol(&bkout_time[1]);
	
	if(timeout == 0 || timeout > 7200){
		//CR_PRM(POA_MIN,"REPT INIT BKOUT TIME OUT VALUE OF %ld",timeout);
    printf("REPT INIT BKOUT TIME OUT VALUE OF %ld\n",
           timeout);
	}

	timeVal.it_value.tv_sec = timeout;
	timeVal.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &timeVal, NULL);

	//INIT_DEBUG((IN_DEBUG | IN_MSGHTR), (POA_INF,"INsys_bk: bkout_file = %s, bkout_time = %s",bkout_file,bkout_time));
  printf("INsys_bk: bkout_file = %s, bkout_time = %s\n",
         bkout_file,bkout_time);

	/* This is necessary so that processes created through "system" call
	** have the out process group id and therefore get killed properly
	** if timeout occurs.
	*/
	setpgrp();
	ret = system(bkout_file);
	ret = WEXITSTATUS(ret);

	if(ret != 0){
		if(errno == EINTR){
			//CR_PRM(POA_INF,"REPT INIT ERROR BKOUT SCRIPT TIMED OUT");
      printf("REPT INIT ERROR BKOUT SCRIPT TIMED OUT\n");
		} else {
			//CR_PRM(POA_INF,"REPT INIT ERROR BKOUT SCRIPT RETURNED %d",ret);
      printf("REPT INIT ERROR BKOUT SCRIPT RETURNED %d\n",ret);
		}
		IN_LDBKRET = ret;
		return;
	}

	IN_LDBKRET = GLsuccess;
	//INIT_DEBUG((IN_DEBUG | IN_MSGHTR), (POA_INF,"Exited INsys_bk"));
  printf("Exited INsys_bk\n");
}

/*
** NAME:
**	INfinish_bkout()
**
** DESCRIPTION:
**	This function performes activities at the end of SU backout.
**
** INPUTS:
**	None		- process index
**
** RETURNS:
**	GLsuccess	- if backout succeeded
**	GLfail		- otherwise
**
**
*/

GLretVal
INfinish_bkout()
{

	FILE 	*file_ptr;
	time_t	seconds;
	char 	*time_ptr;
	char	*pkg_name;
	GLretVal ret = GLfail;
	char	p_n[128],verb[128];

	//INIT_DEBUG((IN_DEBUG | IN_MSGHTR), (POA_INF,"Entered INfinish_bkout"));
  printf("Entered INfinish_bkout\n");

	/* Do not finish backout unless backout script was successful or
	** unconditonal backout was requested
	*/
	
	if(IN_LDBKRET != GLsuccess && IN_LDBKUCL != TRUE){
		//INIT_DEBUG((IN_DEBUG | IN_MSGHTR), (POA_INF,"INfinish_bkout: BKOUT script failed during conditional backout"));
    printf("INfinish_bkout: BKOUT script failed during conditional backout\n");
		/* This condition is not a failure in this script */
		return(GLsuccess);
	}

	if(IN_LDSTATE.isactive == FALSE){
		//CR_PRM(POA_INF,"REPT INIT COMPLETED SU BACKOUT");
    printf("REPT INIT COMPLETED SU BACKOUT\n");

		INsupresent = FALSE;
		if(unlink(INsufile) < 0){
			//INIT_ERROR(("INfinish_bkout: failed to remove %s, errno = %d",INsufile, errno));
      printf("INfinish_bkout: failed to remove %s, errno = %d\n",
             INsufile, errno);
		}
		return(GLsuccess);
	}

	if ((file_ptr = fopen(INhistfile, "r")) == NULL) {
		//CR_PRM(POA_INF,"REPT INIT ERROR COULD NOT OPEN %s, BACKOUT FAILED",INhistfile);
    printf("REPT INIT ERROR COULD NOT OPEN %s, BACKOUT FAILED\n",
           INhistfile);
		return(ret);
	}
 
	/* Retrieve package name from INsupath */
	pkg_name = &INsupath[IN_PATHNMMX-1];
	while(pkg_name >= INsupath && *(pkg_name-1) != '/') {
		pkg_name--;
	} 

	int apply_count = 0;
	int commit_count = 0;
	int bkout_count = 0;
	while (fscanf(file_ptr, "%s %s %*24c\n", p_n, verb) != EOF) {
		if ((strcmp(p_n, (const char *)pkg_name) == 0) &&
		(strcmp(verb, "APPLIED") == 0)) apply_count++;
		if ((strcmp(p_n, (const char *)pkg_name) == 0) &&
		(strcmp(verb, "COMMITTED") == 0)) commit_count++;
		if ((strcmp(p_n, (const char *)pkg_name) == 0) &&
		(strcmp(verb, "BACKED-OUT") == 0)) bkout_count++;
	}

	fclose(file_ptr);
 
	if ((file_ptr = fopen(INhistfile, "a")) == NULL) {
		//CR_PRM(POA_INF,"REPT INIT ERROR COULD NOT OPEN %s, BACKOUT FAILED",INhistfile);
    prinf("REPT INIT ERROR COULD NOT OPEN %s, BACKOUT FAILED\n",
          INhistfile);
		return(ret);
	}

	(void) deleteInProgress();

	time(&seconds);
	time_ptr = ctime(&seconds);

	if( IN_LDBKUCL == TRUE || 
	     !(apply_count == (commit_count + bkout_count))){
		fprintf(file_ptr, "%s BACKED-OUT %s\n", pkg_name, time_ptr);
	}

	fclose(file_ptr);

	/* Send a message to update the version file */
	SUversion versionmsg;
	(void)versionmsg.broadcast(INmsgqid,0L);
	
	//CR_PRM(POA_INF,"REPT INIT COMPLETED SU BACKOUT");
  printf("REPT INIT COMPLETED SU BACKOUT\n");

	INsupresent = FALSE;
	if(unlink(INsufile) < 0){
		//INIT_ERROR(("INfinish_bkout: failed to remove %s, errno = %d",INsufile, errno));
    printf("INfinish_bkout: failed to remove %s, errno = %d\n",
           INsufile, errno);
	}
 
	//INIT_DEBUG((IN_DEBUG | IN_MSGHTR), (POA_INF,"Exited INfinish_bkout"));
  printf("Exited INfinish_bkout\n");
	return(GLsuccess);
}

/*
** NAME:
**	INsudata_find()
**
** DESCRIPTION:
**	This function scans the INsudata table and returns the index
**	of the matching process or IN_SNPRCMX otherwise.
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
int
INsudata_find(char * pathname)
{
	/* Do not search this list if SU is not present */
	if(INsupresent != TRUE){
		return(SU_MAX_OBJ);
	}

	int	c;
	for(c = 0; c < SU_MAX_OBJ ; c++){
		if(strcmp(INsudata[c].obj_path,pathname) == 0){
			break;
		}
	}

	return(c);
}
#endif
