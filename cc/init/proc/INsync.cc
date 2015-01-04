//
// DESCRIPTION:
// 	This file contains INIT routines used to synchronize processes
//	through various stages of initialization.
//
// FUNCTIONS:
//	INgqsequence()	- Sequence global queues during gq failover
//	INsequence()	- Check for current processing to be done
//
//	INscanstep()	- This function checks to see if all the valid
//			  processes in the process table are in a given
//			  process state.
//
//	INsync()	- Step a single process into its next phase of
//			  initialization
//	INpkill()	- kill a process and update sequencing info
//	
//	INmvsufiles()	- move files during SU
//	
//	INmvsuinit()	- move INIT file during SU
//	
//	INscanstate()	- Verify all processes are in this state
//
//	INnext_rlvl()	- Find next run level 
//
//	INinitover()	- check if initialization is completed
//
// NOTES:
//

#include <sysent.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/times.h>
#include <sys/stat.h>

#ifdef CC
#include <time.h>
#include <string.h>
#else
#include <sys/timeb.h>
#include <strings.h>
#endif

#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"

#include "cc/hdr/eh/EHhandler.hh" /* EHboth definition */
#include "cc/hdr/init/INinit.hh"	/* Client process shared memory data structures */
#include "cc/hdr/init/INmtype.hh"
#include "cc/hdr/init/INrunLvl.hh"

#include "cc/hdr/init/INproctab.hh" /* INIT proc's shared memory data */
#include "cc/init/proc/INlocal.hh" /* Main loop polling timer values, etc. */
#include "cc/init/proc/INmsgs.hh"
#include "cc/hdr/init/INinitialize.hh"
#include "cc/hdr/msgh/MHgq.hh"
//#include "cc/hdr/ft/FTbladeMsg.hh"


/*
** NAME:
**	INgqsequence()
**
** DESCRIPTION:
**	This function checks to see if any global queues need to be moved.
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
INgqsequence()
{
  printf("INsync::INgqsequence() enter\n");
	U_short	i;
	U_char	l_runlvl = IN_LDSTATE.run_lvl;

	if(IN_LDSTATE.gq_run_lvl == IN_LDSTATE.final_runlvl){
		return;
	}

	// Check to see if any sequencing is needed for global queues
	for(i = 0; i < IN_SNPRCMX; i++){
		if(IN_INVPROC(i) || IN_LDPTAB[i].gqsync == IN_MAXSTEP){
			continue;
		}
		if(IN_LDPTAB[i].gqsync == IN_GQ){
			// still waiting for one to complete
			// Not new ones can be sent
			return;
		}
		// Find the next global queue to assign if ready
		// at this point gqsynch is IN_BGQ 
		if(IN_LDPTAB[i].run_lvl < l_runlvl){
			l_runlvl = IN_LDPTAB[i].run_lvl;
		}
	}
	
	if(l_runlvl == IN_LDSTATE.final_runlvl){
		// No processes in IN_BGQ state
		IN_LDSTATE.gq_run_lvl = IN_LDSTATE.final_runlvl;
		if(INgqfailover){
			//CR_PRM(POA_INF, "REPT INIT FINISHED TRANSITIONING GLOBAL QUEUES");
      printf("REPT INIT FINISHED TRANSITIONING GLOBAL QUEUES\n");
			INgqfailover = FALSE;
                        if(!IN_LDSTATE.issimplex){
                                // Transition to Lead
                                INrm_check_state(S_LEADACT);
                        }
		}
	} else {
		IN_LDSTATE.gq_run_lvl = l_runlvl;
		// Start all the processes at that runlvl
		for(i = 0; i < IN_SNPRCMX; i++){
			if(IN_INVPROC(i) || IN_LDPTAB[i].run_lvl != l_runlvl ||
				IN_LDPTAB[i].gqsync != IN_BGQ){
				continue;
			}
			// process in IN_BGQ state
			int j;
			for(j = 0; j < INmaxgQids; j++){
				if(IN_LDPTAB[i].gqid[j] != MHnullQ){
					INsync(i, IN_GQ, IN_LDPTAB[i].realQ, IN_LDPTAB[i].gqid[j]);
				}
			}
		}
	}
  printf("INsync::INgqsequence() exit\n");
}


/*
** NAME:
**	INsequence()
**
** DESCRIPTION:
**	This function checks the current state of the INIT work and
**	synchronization flags and takes appropriate actions.
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
INsequence() {
	U_short	i;
	IN_SYNCSTEP next_step;
	register IN_PROCESS_DATA *ldp;
	register IN_PTAB *sdp;

	//INIT_DEBUG((IN_DEBUG),(POA_INF,"INsequence: entering"));
  printf("INsequence: entering\n");

	INgqsequence();

	// Return in no work to do
	if(INworkflg == FALSE){
		return;
	}
  printf("IN_LDSCRIPTSTATE=%d, IN_LDSTATE.final_runlvl=%d, IN_LDSTATE=%d\n",
         IN_LDSCRIPTSTATE, IN_LDSTATE.final_runlvl, IN_LDSTATE.run_lvl);
	if(IN_LDSCRIPTSTATE != INscriptsNone && IN_LDSTATE.final_runlvl == IN_LDSTATE.run_lvl){
		switch(IN_LDSCRIPTSTATE){
		case INscriptsRunning:
			return;
		case INscriptsFinished:
			//CR_PRM(POA_INF, "REPT INIT FINISHED FAILOVER SCRIPTS");
      printf("REPT INIT FINISHED FAILOVER SCRIPTS\n");
			IN_LDSCRIPTSTATE = INscriptsNone;
			break;
		case INscriptsFailed:
			INescalate(SN_LV2, IN_SCRIPTSFAILED, IN_SOFT, INIT_INDEX);
			IN_LDSCRIPTSTATE = INscriptsNone;
			break;
		}
	}
	printf("INsync::INsequence() IN_LDSTATE.systep=%d\n", IN_LDSTATE.systep);
	switch(IN_LDSTATE.systep){
	case IN_CLEANUP:
		// Wait until all processes have died 		
		if(INscanstate(IN_DEAD) == GLfail){
			return;
		}
		// Initialize the next step at which all processes must be synchronized
		// to IN_SYSINIT.
		for(i = 0; i < IN_SNPRCMX; i++){
			if(IN_VALIDPROC(i)){
				IN_SDPTAB[i].error_count = 0;
				IN_SDPTAB[i].progress_check = 0;
				IN_SDPTAB[i].progress_mark = 0;
				IN_SDPTAB[i].procstep = INV_STEP;
				IN_SDPTAB[i].procstate = IN_NOEXIST;
				IN_SDPTAB[i].count = 0;
				IN_LDPTAB[i].pid = IN_FREEPID;
				IN_LDPTAB[i].failed_init = FALSE;
				IN_LDPTAB[i].syncstep = IN_SYSINIT;
				IN_LDPTAB[i].updstate = NO_UPD;
				IN_LDPTAB[i].rstrt_cnt = 0;
				IN_LDPTAB[i].tot_rstrt = 0;
				IN_LDPTAB[i].next_rstrt = 0;
				IN_LDPTAB[i].sn_lvl = IN_LDSTATE.sn_lvl;
				IN_LDPTAB[i].source = IN_SOFT;
				INCLRTMR(INproctmr[i].rstrt_tmr);
			}
		}
		IN_LDSTATE.systep = IN_SYSINIT;
		break;
	case IN_SYSINIT:
		// Sequence all processes through SYSINIT			 
		// Wait until all processes have completed IN_SYSINIT 		
		if(INscanstep(IN_ESYSINIT) == GLfail){
			break;
		}
		INnext_rlvl();  
		IN_LDSTATE.systep = IN_STEADY;
		break;
	case IN_STEADY:
		// Sequence the processes through the rest of the initialization 
		break;
	default:
		INescalate(SN_LV2,ININVSYSTEP,IN_SOFT,INIT_INDEX);
		break;
	}
	
	sdp = IN_SDPTAB;
	ldp = IN_LDPTAB;
	SN_LVL snlvl;
	// Get all processes synchronized to their maximum synchronization step
	for(i=0; i < IN_SNPRCMX; sdp++,ldp++,i++){
		// Skip empty table entries and processes that are not to be
		// synchronized.
		if(ldp->syncstep == IN_MAXSTEP ||
		   sdp->procstep == IN_STEADY ||
		   ldp->syncstep == INV_STEP){
			continue;
		}
    printf("INsync::INsequence() set next_step to entriy %s, current procstep is %d\n", ldp->proctag, sdp->procstep);
		switch(sdp->procstep){
		case INV_STEP:
			next_step = IN_READY;
			break;
		case IN_READY:
			next_step = IN_BHALT;
			break;
		case IN_EHALT:
			// skip SYSINIT step if sn_lvl is < SN_LVL2
			if(ldp->sn_lvl < SN_LV2){
				// Clear the creation timer
				INCLRTMR(INproctmr[i].sync_tmr);
				next_step = IN_BPROCINIT;
			} else {
				// Skip startup for now, it is not	
				// implemented anyway
				next_step = IN_BSYSINIT;
			}
			break;
		case IN_ESTARTUP:
			next_step = IN_BSYSINIT;
			break;
		case IN_ESYSINIT:
			// Clear the creation timer
			INCLRTMR(INproctmr[i].sync_tmr);
			next_step = IN_BPROCINIT;
			break;
		case IN_EPROCINIT:
			// Clear PROCINIT timer
			INCLRTMR(INproctmr[i].sync_tmr);
			// Check if all processes at current run level 
			// completed PROCINIT
			INnext_rlvl();
			next_step = IN_BPROCESS;
			break;
		case IN_EPROCESS:
			next_step = IN_CPREADY;
			break;
		case IN_ECPREADY:
			/* Initialization complete for this process */
			sdp->procstep = IN_STEADY;
			
			/* Only set the timer if restart interval is non zero */
			if(ldp->rstrt_intvl > 0){
				/* Count this for at least one restart, to keep audit happy 	*/
				if(ldp->rstrt_cnt == 0){
					ldp->rstrt_cnt = 1;
				}
				/* Set restart timer 	*/
				INSETTMR(INproctmr[i].rstrt_tmr,ldp->rstrt_intvl,
						  (INPROCTAG | INRSTRTAG | i), FALSE);
			} else {
				/* Setup the process info as if restart timer expired */
				ldp->sn_lvl = SN_NOINIT;
				INCLRTMR(INproctmr[i].rstrt_tmr);
			}

			ldp->sent_missedsan = FALSE;
			ldp->time_missedsan = 0;
			INCLRTMR(INproctmr[i].sync_tmr);
			continue;
		case IN_BHALT:
		case IN_HALT:
		case IN_BSTARTUP:
		case IN_STARTUP:
		case IN_BSYSINIT:
		case IN_SYSINIT:
		case IN_BPROCINIT:
		case IN_PROCINIT:
		case IN_BPROCESS:
		case IN_PROCESS:
		case IN_CPREADY:
		case IN_BSTEADY:
		case IN_BCLEANUP:
		case IN_CLEANUP:
		case IN_ECLEANUP:
			// No synchronization work to do for this process
			//INIT_DEBUG((IN_DEBUG | IN_SSEQTR),(POA_INF,"INsequence: waiting.. proc %s step %s", ldp->proctag,IN_SQSTEPNM(sdp->procstep)));
      printf("INsequence: waiting.. proc %s step %s\n",
             ldp->proctag,IN_SQSTEPNM(sdp->procstep));
			continue;
		case IN_STEADY:
		case IN_MAXSTEP:
		default:
			//INIT_ERROR(("Invalid state %s for proc %s",sdp->procstep,ldp->proctag));
      printf("Invalid state %s for proc %s\n",
             sdp->procstep,ldp->proctag);
			continue;
		}
		printf("process %s has next_step to %d, and syncstep is %d\n", ldp->proctag, sdp->procstep, ldp->syncstep);
		if(next_step <= ldp->syncstep){
			INsync(i,next_step);
		}
	}

	// If not yet registered with MSGH than attach to MSGH
	if(INetype != EHBOTH){
		INprocinit();
	}
	// Check if initialization is over or if there is any work left
	// to do
	INinitover();
}


/*
** NAME:
**	INscanstep()
**
** DESCRIPTION:
**	This function scans the process table and compares the current
**	SYNC_STATE of all valid entries against the value passed to the
**	routine. If all processes at least reached that state, GLsuccess is returned.
**
** INPUTS:
**	step	- Synchronization step to compare with current process
**		  sync steps.
**
** RETURNS:
**	GLsuccess - All valid processes in the process table are in the
**		    sync state "step" passed to the function
**
**	GLfail	- NOT all valid processes in process table are in the
**		    sync state "step" passed to the function
**
** SIDE EFFECTS:
*/

GLretVal
INscanstep(IN_SYNCSTEP step)
{
	//INIT_DEBUG((IN_DEBUG | IN_SSEQTR),(POA_INF,"INscanstep: entering, step is %s...", IN_SQSTEPNM(step)));
  printf("INscanstep: entering, step is %s...\n",
         IN_SQSTEPNM(step));

	for(int i = 0; (i < IN_SNPRCMX); i++) {
		if ((IN_VALIDPROC(i)) && 
	   	    (IN_SDPTAB[i].procstep < step) &&
		    (IN_LDPTAB[i].failed_init == FALSE)) {
			/* Not "in sync", return GLfail */
			//INIT_DEBUG((IN_DEBUG | IN_SSEQTR),(POA_INF,"INscanstep: returning w/GLfail\n\tproc \"%s\" in step %s, not requested step %s", IN_LDPTAB[i].proctag, IN_SQSTEPNM(IN_SDPTAB[i].procstep), IN_SQSTEPNM(step)));
      printf("INscanstep: returning w/GLfail\n\tproc \"%s\" in step %s, not requested step %s\n",
             IN_LDPTAB[i].proctag, IN_SQSTEPNM(IN_SDPTAB[i].procstep), IN_SQSTEPNM(step));
			return(GLfail);
		}
	}

	//INIT_DEBUG((IN_DEBUG | IN_SSEQTR),(POA_INF,"INscanstep: returning w/GLsuccess for sync step %s", IN_SQSTEPNM(step)));
  printf("INscanstep: returning w/GLsuccess for sync step %s\n",
         IN_SQSTEPNM(step));
	return(GLsuccess);
}

/*
** NAME:
**	INscanstate()
**
** DESCRIPTION:
**	This function scans the process table and compares the current
**	SYNC_STATE of all valid entries against the value passed to the
**	routine.
**
** INPUTS:
**	state	- Synchronization step to compare with current process
**		  states.
**
** RETURNS:
**	GLsuccess - All valid processes in the process table are in the
**		    state "state" passed to the function
**
**	GLfail	- NOT all valid processes in process table are in the
**		    sync state "step" passed to the function
**
** SIDE EFFECTS:
*/

GLretVal
INscanstate(IN_PROCSTATE state)
{
	//INIT_DEBUG((IN_DEBUG | IN_SSEQTR),(POA_INF,"INscanstate: entering, state is %s...", IN_PROCSTNM(state)));
  printf("INscanstate: entering, state is %s...\n",
         IN_PROCSTNM(state));

	for(int i = 0; (i < IN_SNPRCMX); i++) {
		if ((IN_VALIDPROC(i)) && (IN_SDPTAB[i].procstate != state)) {
			/* Not in right state, return GLfail */
			//INIT_DEBUG((IN_DEBUG | IN_SSEQTR),(POA_INF,"INscanstate: returning w/GLfail\n\tproc \"%s\" in state %s, not requested state %s", IN_LDPTAB[i].proctag, IN_PROCSTNM(IN_SDPTAB[i].procstate), IN_PROCSTNM(state)));
      printf("INscanstate: returning w/GLfail\n\tproc \"%s\" in state %s, not requested state %s\n",
             IN_LDPTAB[i].proctag, IN_PROCSTNM(IN_SDPTAB[i].procstate), IN_PROCSTNM(state));
			return(GLfail);
		}
  }
      
  //INIT_DEBUG((IN_DEBUG | IN_SSEQTR),(POA_INF,"INscanstep: returning w/GLsuccess"));
  printf("INscanstep: returning w/GLsuccess\n");
      
	return(GLsuccess);
}

/*
** NAME:
**	INinitover()
**
** DESCRIPTION:
**	This function scans the process table and determines if all initialization
**	work has been completed.  It also checks if any more synchronization 
**	work is needed. 
**
** INPUTS:
**
** RETURNS:
**
** SIDE EFFECTS:
**	Clear INworkflg if no more synchronization work is needed
*/

Void
INinitover()
{
register IN_PROCESS_DATA *ldp,*eldp;
register IN_PTAB *sdp;

Bool work_to_do = FALSE;

//INIT_DEBUG((IN_DEBUG | IN_SSEQTR),(POA_INF,"INinitover: entering"));
printf("INinitover: entering\n");

if(IN_LDSTATE.sync_run_lvl != IN_LDSTATE.run_lvl){ 
		/* Initialization not yet complete	*/
		if(INworkflg == TRUE){
			//INIT_DEBUG((IN_DEBUG | IN_SSEQTR),(POA_INF,"INinitover: init in progress"));
      printf("INinitover: init in progress\n");
		} else {
			/* INworkflg has to be TRUE here, report problem and
			** set INworkflg to TRUE.
			*/
			//INIT_ERROR(("Invalid INworkflg = %d",INworkflg));
      printf("Invalid INworkflg = %d",INworkflg);
			INworkflg = TRUE;
			INsettmr(INpolltmr,INITPOLL,(INITTAG|INPOLLTAG),TRUE, TRUE);
		}
		return;
	}

	ldp = IN_LDPTAB;
	eldp = &IN_LDPTAB[IN_SNPRCMX];
	sdp = IN_SDPTAB;
	for(; ldp < eldp; sdp++,ldp++) {
		if (ldp->syncstep != IN_MAXSTEP && (sdp->procstep != IN_STEADY)){  
      if(ldp->failed_init == FALSE) {
				// This process has not yet reached IN_STEADY and it also
				// has not yet been initialized.
        //INIT_DEBUG((IN_DEBUG | IN_SSEQTR),(POA_INF,"INinitover: not over because of %s procstep %s",ldp->proctag,IN_SQSTEPNM(sdp->procstep)));
        printf("INinitover: not over because of %s procstep %s\n",
               ldp->proctag,IN_SQSTEPNM(sdp->procstep));
				return;
			} else { 
				// Process already failed an init at least once
				if(ldp->syncstep != INV_STEP){
					// This process was not given up on
					// Continue trying to initialize
					work_to_do = TRUE;
					//INIT_DEBUG((IN_DEBUG | IN_SSEQTR),(POA_INF,"INinitover: work to do because of %s procstep %s",ldp->proctag,IN_SQSTEPNM(sdp->procstep)));
          printf("INinitover: work to do because of %s procstep %s\n",
                 ldp->proctag,IN_SQSTEPNM(sdp->procstep));
				}
			}
		}
	}

	INworkflg = work_to_do;

  U_short last_indx = IN_LDINFO.ld_indx;
	if(INworkflg == FALSE){
		// See if we should reset the run level to the next stage
		if(IN_LDSTATE.final_runlvl > IN_LDSTATE.run_lvl){
			// If start scripts not yet run, start the start scripts thread
			if(IN_LDSTATE.run_lvl == IN_LDSTATE.first_runlvl){
				INworkflg = TRUE;
				switch(IN_LDSCRIPTSTATE){
				case INscriptsNone:
           {
             int	ret;
             IN_LDSCRIPTSTATE = INscriptsRunning;
             if((ret = fork()) == 0){
               static int type = INscriptsStart;
               //mutex_unlock(&CRlockVarible);
               //CR_PRM(POA_INF, "REPT INIT RUNNING STARTUP SCRIPTS");
               printf("REPT INIT RUNNING STARTUP SCRIPTS\n");
               INrunScriptList(&type);
               exit(0);
             } else if(ret < 0){
               //CR_PRM(POA_INF, "REPT INIT ERROR FAILED TO FORK STARTUP PROCESS, RET %d", ret);
               printf("REPT INIT ERROR FAILED TO FORK STARTUP PROCESS, RET %d", ret);
               IN_LDSCRIPTSTATE = INscriptsNone;
               INescalate(SN_LV4, IN_SCRIPTSFAILED, IN_SOFT, INIT_INDEX);
             }
             
             return;
           }
				case INscriptsRunning:
					return;
				case INscriptsFinished:
					//CR_PRM(POA_INF, "REPT INIT FINISHED STARTUP SCRIPTS");
          printf("REPT INIT FINISHED STARTUP SCRIPTS\n");
					IN_LDSCRIPTSTATE = INscriptsNone;
					break;
				case INscriptsFailed:
					INescalate(SN_LV2, IN_SCRIPTSFAILED, IN_SOFT, INIT_INDEX);
					IN_LDSCRIPTSTATE = INscriptsNone;
					break;
				}
			}

			IN_LDSTATE.run_lvl = IN_LDSTATE.final_runlvl;
			Short num_procs = 0;
			if(IN_ISACTIVE(IN_LDCURSTATE)){
				num_procs = INrdinls(FALSE,FALSE);
			}
			if(num_procs > 0){
				IN_LDSTATE.systep = IN_SYSINIT;
 				IN_LDINFO.init_data[last_indx].num_procs += num_procs;
			} else if(num_procs == 0){
				IN_LDSTATE.sync_run_lvl = IN_LDSTATE.run_lvl;
			} else {
				// bad initlist
				sleep(90);
				INescalate(SN_LV2,INBADINITLIST,IN_SOFT,INIT_INDEX);
			}
			if((!INevent.onLeadCC()) && (IN_LDCURSTATE == S_ACT)){
				// Ask for temporary processes again
				INSETTMR(INcheckleadtmr,INCHECKLEADTMR + 5, INCHECKLEADTAG, TRUE);
			}
			INworkflg = TRUE;
			return;
		}
		INCLRTMR(INpolltmr);
	}

	if(IN_LDSTATE.initstate == INITING){
		// Transition to postinit interval
		// Completed system initialization
		IN_LDSTATE.initstate = IN_CUINTVL;
		IN_LDSTATE.systep = IN_STEADY;
		// Set post init interval timer
    INSETTMR(INinittmr,IN_procdata->safe_interval, (INITTAG|INSEQTAG), FALSE);
		//FTbladeStChgMsg msg(S_LEADACT, IN_LDSTATE.initstate, IN_LDSTATE.softchk);
		//if(INvhostMate >= 0){
		//	msg.setVhostMate(IN_procdata->vhost[INvhostMate]);
		//	if(INevent.isVhostActive()){
		//		msg.setVhostState(INactive);
		//	} else {
		//		msg.setVhostState(INstandby);
		//	}
		//}
		//msg.send();
		/*
		 * Store information relating to the just completed
		 * system reset:
		 */
		IN_LDINFO.init_data[IN_LDINFO.ld_indx].end_time = time((time_t *)0);
    /*
     * If INIT is using MSGH and if we've just finished
     * increasing the system's run level then broadcast
     * the new run level:
     */
    if ((IN_LDINFO.init_data[IN_LDINFO.ld_indx].source == IN_RUNLVL) &&
        (INetype != EHTMRONLY)) {
      U_short oindx = (IN_LDINFO.ld_indx == 0) ?
         (IN_NUMINITS - 1) : (IN_LDINFO.ld_indx - 1);
      if (IN_LDSTATE.run_lvl != IN_LDINFO.init_data[oindx].prun_lvl) {
        class INlrunLvl lrlvl(IN_LDSTATE.run_lvl);
        (Void)lrlvl.broadcast(INmsgqid,INbrdcast_delay);
      }
    }

    /*
     * Update INIT data load and unload indices so
     * they'll be correct for the next init:
     */
    IN_LDINFO.ld_indx = IN_NXTINDX(IN_LDINFO.ld_indx, IN_NUMINITS);
    if (IN_LDINFO.ld_indx == IN_LDINFO.uld_indx) {
      IN_LDINFO.uld_indx = IN_NXTINDX(IN_LDINFO.uld_indx, IN_NUMINITS);
    }
		/*
		 * Output the "INITIALIZATION COMPLETE" message
		 */
		char stime[25], etime[25];

		time_t str_time = (time_t)IN_LDINFO.init_data[last_indx].str_time;
		time_t end_time = (time_t)IN_LDINFO.init_data[last_indx].end_time;
	
		strncpy(stime, asctime(localtime(&str_time)), 24);
		strncpy(etime, asctime(localtime(&end_time)), 24);
		stime[24] = 0;
		etime[24] = 0;

		/*
		 * Print message which includes the
		 * name of the process associated
		 * with the last system-wide init.
		 */
		//CR_PRM(POA_INF + POA_MAXALARM, "***** REPT INITIALIZATION COMPLETE *****\n   START TIME:  %s\n   FINISH TIME: %s\n   %2d PROCS, %s, RUNLVL: %2d, SOURCE: %s/\"%s\", err: %d\n",stime, etime, IN_LDINFO.init_data[last_indx].num_procs,IN_SNLVLNM(IN_LDINFO.init_data[last_indx].psn_lvl),IN_LDINFO.init_data[last_indx].prun_lvl,IN_SNSRCNM(IN_LDINFO.init_data[last_indx].source), IN_LDINFO.init_data[last_indx].msgh_name,IN_LDINFO.init_data[last_indx].ecode);
    printf("***** REPT INITIALIZATION COMPLETE *****\n   START TIME:  %s\n   "
           "FINISH TIME: %s\n   %2d PROCS, %s, RUNLVL: %2d, SOURCE: %s/\"%s\", "
           "err: %d\n",stime, etime, IN_LDINFO.init_data[last_indx].num_procs,
           IN_SNLVLNM(IN_LDINFO.init_data[last_indx].psn_lvl),
           IN_LDINFO.init_data[last_indx].prun_lvl,IN_SNSRCNM(IN_LDINFO.init_data[last_indx].source),
           IN_LDINFO.init_data[last_indx].msgh_name,IN_LDINFO.init_data[last_indx].ecode);

		if(INdidFailover){
			INdidFailover = FALSE;
			//CR_X733PRM(INfailover_alarm, "FAILOVER INITIALIZATION", processingErrorAlarm,
      //           softwareError, "Failover has Occured", ";236", "REPT INIT FAILOVER INITIALIZATION COMPLETED");
      printf("REPT INIT FAILOVER INITIALIZATION COMPLETED\n");
		}
	}  else {
		//INIT_DEBUG((IN_DEBUG | IN_SSEQTR),(POA_INF,"INinitover: Not in initialization IN_LDBKOUT %d, IN_LDBQID %s",IN_LDBKOUT,IN_LDBQID.display()));
    printf("INinitover: Not in initialization IN_LDBKOUT %d, IN_LDBQID %s\n",
           IN_LDBKOUT,IN_LDBQID.display());
#ifdef OLD_SU
		/* Handle completion of SU apply/backout */
		if(IN_LDBKOUT == TRUE){
			/* Make sure there are no processes not yet backed out */
			for(int i = 0; i < IN_SNPRCMX; i++){
				if(IN_VALIDPROC(i)  && IN_LDPTAB[i].updstate != NO_UPD){
					//INIT_DEBUG((IN_DEBUG | IN_SSEQTR),(POA_INF,"INinitover: backout still not completed for %s",IN_LDPTAB[i].proctag));
          printf("INinitover: backout still not completed for %s\n",
                 IN_LDPTAB[i].proctag);
					IN_LDBKOUT = FALSE;
					INautobkout(FALSE,FALSE);
					return;
				}
			}
			IN_LDBKOUT = FALSE;
			/* Remove all the other obj.new or image.old files that
			** may not have been present in the process table
			*/
			SN_LVL sn_lvl;
			INmvsufiles(IN_SNPRCMX,sn_lvl,INSU_BKOUT);
			GLretVal ret = INfinish_bkout();
			if(IN_LDBQID != MHnullQ){	/* Send ack/nack to backout CEP if present*/
				if(IN_LDBKRET == GLsuccess && ret == GLsuccess){
					SUbkoutAck bkackmsg;
					(Void)bkackmsg.send(IN_LDBQID,INmsgqid,0);
				} else {
					if(ret == GLsuccess){
						SUbkoutFail bkfailmsg(SU_BKOUT_SCRIPT,IN_LDBKRET);
						(Void)bkfailmsg.send(IN_LDBQID,INmsgqid,0);
					} else {
						SUbkoutFail bkfailmsg(SU_HIST_FAIL);
						(Void)bkfailmsg.send(IN_LDBQID,INmsgqid,0);
					}
				}
				IN_LDBQID = MHnullQ;
			}
			if(IN_LDAQID != MHnullQ){	/* Send ack to apply CEP if present */
				SUapplyFail applyfailmsg(SU_AUTO_BKOUT);
				(Void)applyfailmsg.send(IN_LDAQID,INmsgqid,0);
				IN_LDAQID = MHnullQ;
			}
		} else if(IN_LDAQID != MHnullQ){
			//CR_PRM(POA_INF,"REPT INIT COMPLETED SU APPLY");
      printf("REPT INIT COMPLETED SU APPLY\n");
			SUapplyAck appackmsg;
			(void)appackmsg.send(IN_LDAQID,INmsgqid,0);
			IN_LDAQID = MHnullQ;
		}
#endif
	}
	//INIT_DEBUG((IN_DEBUG | IN_SSEQTR),(POA_INF,"INinitover: returning , work_to_do = %d",work_to_do));
  printf("INinitover: returning , work_to_do = %d\n",
         work_to_do);
	return;
}

/*
** NAME:
**	INsync()
**
** DESCRIPTION:
**	This function steps the process referred to by the index passed
**	to this routine into the desired sync state.
**
** INPUTS:
**	indx	- Process table index of the process which is to be
**		  acted upon
**
**	step	- Next sync state for processes
**
** RETURNS:
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
*/
GLretVal
INsync(U_short indx, IN_SYNCSTEP step, MHqid realQ, MHqid gqid)
{
	GLretVal ret;

	//INIT_DEBUG((IN_DEBUG | IN_SSEQTR | IN_RSTRTR),(POA_INF,"INsync(): entered w/indx %d, step %s", indx, IN_SQSTEPNM(step)));
  printf("INsync(): entered w/indx %d, step %s\n",
         indx, IN_SQSTEPNM(step));

	if (indx >= IN_SNPRCMX) {
		//INIT_ERROR(("Proc indx %d out of range, max %d",indx, (IN_SNPRCMX - 1)));
    printf("Proc indx %d out of range, max %d\n",
           indx, (IN_SNPRCMX - 1));
		return(GLfail);
	}

	INworkflg = TRUE;
	switch(IN_SDPTAB[indx].procstate) {
	case IN_CREATING:
		if (step == IN_BCLEANUP){
			INpkill(indx,IN_BCLEANUP,IN_ECLEANUP);
#ifdef OLD_SU
		} else if(step == IN_SU){
			/* This should only be possible in backout case 
			** since apply is not allowed when the system is
			** not in steady state.
			** This will cause an escalating error, however
			** the process will be killed anyway.
			*/
			if(IN_LDPTAB[indx].updstate != UPD_POSTSTART){
				//INIT_ERROR(("INsync called to backout SU while %s in %d state",IN_LDPTAB[indx].proctag,IN_LDPTAB[indx].updstate));
        printf("INsync called to backout SU while %s in %d state",
               IN_LDPTAB[indx].proctag,IN_LDPTAB[indx].updstate);
			}
			INpkill(indx,IN_SU,IN_ESU);
#endif
		} else if(step == IN_READY){
			//INIT_DEBUG((IN_DEBUG | IN_SSEQTR | IN_RSTRTR),(POA_INF,"INsync(): waiting for process %s creation to complete", IN_LDPTAB[indx].proctag));
      printf("INsync(): waiting for process %s creation to complete\n",
             IN_LDPTAB[indx].proctag);
			return(GLsuccess);
		} else {
			//INIT_ERROR(("INsync called with step %d while %s being created",step, IN_LDPTAB[indx].proctag));
      printf("INsync called with step %d while %s being created\n",
             step, IN_LDPTAB[indx].proctag);
			return(GLfail);
		}
		break;

	case IN_HALTED:
	case IN_RUNNING:

		if (IN_LDPTAB[indx].pid <= 1 && IN_LDPTAB[indx].tid == IN_FREEPID) {
			//INIT_ERROR(("Proc \"%s\", indx %d, pid %d",IN_LDPTAB[indx].proctag,indx, IN_LDPTAB[indx].pid));
      printf("Proc \"%s\", indx %d, pid %d\n",
             IN_LDPTAB[indx].proctag,indx, IN_LDPTAB[indx].pid);
			IN_SDPTAB[indx].procstate = IN_DEAD;
			if(step == IN_BCLEANUP){
				return(GLsuccess);
			}

			return(GLfail);
		}


		switch(step){
		case IN_BCLEANUP:
			Long clean_tmr;
			// Set different timers if in initialization then if a single
			// process is restarted during initialization creation time may be
			// extended if many cleanups are requested
			if(IN_LDSTATE.systep == IN_CLEANUP){
				clean_tmr = INCLEANUPTMR;
			} else {
				clean_tmr = INPCLEANUPTMR + 1;
			}
			INSETTMR(INproctmr[indx].sync_tmr,clean_tmr,(INPROCTAG | INSYNCTAG | indx),FALSE);
			break;
		case IN_BPROCINIT:
			// Set PROCINIT timer
			INSETTMR(INproctmr[indx].sync_tmr,IN_LDPTAB[indx].procinit_timer,(INPROCTAG | INSYNCTAG | indx),FALSE);
			// If this is level 3 and process runs on active
			// just send a message instead of initing the process
			if(IN_LDPTAB[indx].sn_lvl == SN_LV3){
				if(IN_LDPTAB[indx].on_active){
					if(IN_SDPTAB[indx].procstate == IN_HALTED) {
						INreqinit(SN_LV0, indx, INVPROCSTATE, IN_SOFT, "PROCESS HALTED");
						return(GLfail);
					}
					IN_SDPTAB[indx].procstep = step;
					IN_SDPTAB[indx].progress_mark = 0;
					char 	name[MHmaxNameLen+1];
					INgetRealName(name, IN_LDPTAB[indx].proctag);
					INinitialize	imsg;
					if((ret = imsg.send(name, INmsgqid, sizeof(imsg), 0L)) != GLsuccess){
						//INIT_ERROR(("INsync failed to send initalize message to %s ret %d",IN_LDPTAB[indx].proctag, ret));
            printf("INsync failed to send initalize message to %s ret %d\n",
                   IN_LDPTAB[indx].proctag, ret);
						INreqinit(SN_LV0, indx, ret, IN_SOFT, "FAILED TO SEND ININITIALIZE");
						return(GLfail);
					}
					IN_SDPTAB[indx].procstep = IN_PROCINIT;
					return(GLsuccess);
				}
			}
			break;
		case IN_BPROCESS:
			INSETTMR(INproctmr[indx].sync_tmr,IN_LDPTAB[indx].init_complete_timer,(INPROCTAG | INSYNCTAG | indx),FALSE);
			break;
		case IN_BHALT:
		case IN_BSTARTUP:
		case IN_BSYSINIT:
			break;
		case IN_READY:
			// Process transitioned to HALTED state but it did
			// not yet change syncstep to READY before preempted
			//INIT_DEBUG((IN_DEBUG | IN_SSEQTR | IN_RSTRTR),(POA_INF,"INsync(): waiting for process %s to become READY", IN_LDPTAB[indx].proctag));
      printf("INsync(): waiting for process %s to become READY\n",
             IN_LDPTAB[indx].proctag);
			return(GLsuccess);
		case IN_CPREADY:
			/* No signal is sent for this step, just return 
			** Also, make sure that we do not go backwards.
			** This can happen if the process transitions from 
			** IN_EPROCESS to IN_ECPREADY and INIT gets preempted.
			*/
			if(IN_SDPTAB[indx].procstep < step){
				IN_SDPTAB[indx].procstep = step;
			}
			return(GLsuccess);
#ifdef OLD_SU
		case IN_SU:
			Long su_tmr;
			
			/* Set the SU exit timer to be at least as long as
			** sanity timer but not smaller than cleanup timer.
			*/
			if(IN_LDPTAB[indx].peg_intvl > INCLEANUPTMR){
				su_tmr = IN_LDPTAB[indx].peg_intvl;
			} else {
				su_tmr = INCLEANUPTMR;
			}

			/* This step is used for both apply and backout except 
			** during backout processes are just killed
			*/
			if(IN_SDPTAB[indx].procstate != IN_RUNNING && 
			   IN_LDPTAB[indx].updstate == UPD_PRESTART){
				//INIT_ERROR(("INsync() called to start SU while %s not running",IN_LDPTAB[indx].proctag));
        printf("INsync() called to start SU while %s not running\n",
               IN_LDPTAB[indx].proctag);
				INautobkout(FALSE,FALSE);
				return(GLfail);
			}
			/* Find the index of the process in INsudata */
			int su_idx;
			su_idx = INsudata_find(IN_LDPTAB[indx].pathname);

			if(IN_LDPTAB[indx].updstate == NO_UPD && !(su_idx < SU_MAX_OBJ && 
                                                 INsudata[su_idx].changed == FALSE)){
				/* Process must have updstate indicating  update
				** unless it is in SU tables with changed field set
				** to FALSE
				*/
				//INIT_ERROR(("INsync called to do SU while %s in NO_UPD state",IN_LDPTAB[indx].proctag));
        printf("INsync called to do SU while %s in NO_UPD state\n",
               IN_LDPTAB[indx].proctag);
			}

			Bool	kill_proc;

			if(su_idx < SU_MAX_OBJ){
				kill_proc = INsudata[su_idx].kill;
			} else {
				//INIT_ERROR(("INsync called to do SU while %s not in INsudata",IN_LDPTAB[indx].proctag));
        printf("INsync called to do SU while %s not in INsudata\n",
               IN_LDPTAB[indx].proctag)
				kill_proc = FALSE;
			}

			/* Only send SUexitMsg if process kill not requested */
			if((IN_LDPTAB[indx].updstate == UPD_PRESTART ||
			    (IN_LDBKOUT == FALSE && INsudata[su_idx].changed == FALSE)) 
         && kill_proc == FALSE){
				SUexitMsg exitmsg;
				// #?%& If process uses global queue must send to _proctag
				if((ret = exitmsg.send(IN_LDPTAB[indx].proctag,INmsgqid,0)) != GLsuccess){
				
					//INIT_ERROR(("INsync: failed to send a SUexit to %s retval = %d",IN_LDPTAB[indx].proctag,ret));
          printf("INsync: failed to send a SUexit to %s retval = %d\n",
                 IN_LDPTAB[indx].proctag,ret);
					INautobkout(FALSE,FALSE);
					return(GLfail);
				}
				INSETTMR(INproctmr[indx].sync_tmr,su_tmr,(INPROCTAG | INSYNCTAG | indx),FALSE);
				IN_SDPTAB[indx].procstep = IN_SU;
			} else {
				/* Kill the process for backout */
				INpkill(indx,IN_SU,IN_ESU);
			}

			return(GLsuccess);
#endif
		case IN_GQ:
       {	
         SN_LVL sn_lvl;
         if(INgqfailover){
           sn_lvl = SN_LV3;
         } else {
           sn_lvl = SN_LV1;
         }
         MHgqRcv	rcvMsg(sn_lvl, gqid);
         IN_LDPTAB[indx].gqsync = step;
         if((gqid == MHnullQ) || (realQ == MHnullQ) ||
            (ret = rcvMsg.send(realQ, INmsgqid)) != GLsuccess){
           IN_LDPTAB[indx].gqsync = IN_MAXSTEP;
           IN_LDPTAB[indx].gqCnt = 0;
           //INIT_ERROR(("INsync: Invalid queues %s %s or failed to send  MHgqRcv to %s retval = %d",realQ.display(), gqid.display(), IN_LDPTAB[indx].proctag,ret));
           printf("INsync: Invalid queues %s %s or failed to send  MHgqRcv to %s retval = %d\n",
                  realQ.display(), gqid.display(), IN_LDPTAB[indx].proctag,ret);
           INreqinit(SN_LV0, indx, ret, IN_SOFT, "FAILED TO SEND GQRCV MSG");
           return(GLfail);
         }
         // In the future, may need to id a specific other global queue
         //CR_PRM(POA_INF, "REPT INIT %s STARTING GLOBAL QUEUE TRANSITION", IN_LDPTAB[indx].proctag);
         printf("REPT INIT %s STARTING GLOBAL QUEUE TRANSITION\n",
                IN_LDPTAB[indx].proctag);
         IN_LDPTAB[indx].gqCnt++;
         INSETTMR(INproctmr[indx].gq_tmr,IN_LDPTAB[indx].global_queue_timer,(INPROCTAG | INGQTAG | indx),FALSE);
         return(GLsuccess);
       }
		default:
			//INIT_ERROR(("Proc \"%s\", pid %d, step = %s",IN_LDPTAB[indx].proctag, IN_LDPTAB[indx].pid,IN_SQSTEPNM(step)));
      printf("Proc \"%s\", pid %d, step = %s\n",
             IN_LDPTAB[indx].proctag, IN_LDPTAB[indx].pid,IN_SQSTEPNM(step));
			return(GLfail);
		}

		IN_SDPTAB[indx].procstep = step;
		IN_SDPTAB[indx].procstate = IN_RUNNING;
		if (step == IN_BCLEANUP){ 
			if(IN_LDPTAB[indx].third_party == FALSE){
				if(kill(IN_LDPTAB[indx].pid, SIGUSR2) < 0) {
					/*
           * Didn't successfully send signal, process may
           * be dead or access may be denied, generate
           * error message and return:
           */
					//INIT_ERROR(("Could not send signal to \"%s\" errno = %d",IN_LDPTAB[indx].proctag,errno));
          printf("Could not send signal to \"%s\" errno = %d\n",
                 IN_LDPTAB[indx].proctag,errno);
					return(GLfail);
				}
			} else if(thr_kill(IN_LDPTAB[indx].tid, SIGUSR2) != 0) {
				IN_SDPTAB[indx].procstep = IN_ECLEANUP;
				return(GLsuccess);
			}
		}
		break;

	case IN_NOEXIST:
		// If process still did not complete creation but its state is
		// IN_NOEXIST, kill it if cleanup is requested
		if (step == IN_BCLEANUP || step == IN_SU){
			if(IN_LDPTAB[indx].pid > 1) {
				(void)kill(IN_LDPTAB[indx].pid,SIGKILL);
			}
			/* Update process states here because INgrimreaper() will
			** ignore this process since it is in IN_NOEXIST state
			*/
			IN_SDPTAB[indx].procstate = IN_DEAD;
			IN_LDPTAB[indx].pid = IN_FREEPID;
			if(step == IN_BCLEANUP){
				IN_SDPTAB[indx].procstep = IN_ECLEANUP;
			} else {
				if(IN_LDPTAB[indx].updstate != UPD_POSTSTART){
					//INIT_ERROR(("INsync called to backout SU while %s in %d state",IN_LDPTAB[indx].proctag,IN_LDPTAB[indx].updstate));
          printf("INsync called to backout SU while %s in %d state\n",
                 IN_LDPTAB[indx].proctag,IN_LDPTAB[indx].updstate);
				}
				IN_SDPTAB[indx].procstep = IN_ESU;
			}
			break;
		} else  if(step != IN_READY){
			/* Create a process only if the reqested step is IN_READY */
			//INIT_ERROR(("Proc \"%s\", step = %s",IN_LDPTAB[indx].proctag,IN_SQSTEPNM(step)));
      printf("Proc \"%s\", step = %s\n",
             IN_LDPTAB[indx].proctag,IN_SQSTEPNM(step));
			return(GLfail);
		} else if(IN_LDPTAB[indx].pid > 1 || IN_LDPTAB[indx].tid != IN_FREEPID){
			// There is a small window where state is IN_NOEXIST but
			// the process is already created but not yet running
			return(GLsuccess);
		}
		Long creat_tmr;
		// Set different timers if in initialization then if a single
		// process is restarted during initialization creation time may be
		// extended if many creats are requested
		if((IN_LDSTATE.initstate == INITING) && 
       (IN_LDPTAB[indx].create_timer < IN_procdata->default_create_timer * 3)){
			creat_tmr = IN_procdata->default_create_timer * 3;
		} else {
			creat_tmr = IN_LDPTAB[indx].create_timer;
		}

		// Create this process
		if((ret = INcreate(indx)) != GLsuccess){
			IN_SDPTAB[indx].procstate = IN_DEAD;
			INescalate(SN_LV0,IN_CREAT_FAIL,IN_SOFT,indx);
			// If software checks are inhibited, return failure code
			return(ret);
		}
		INSETTMR(INproctmr[indx].sync_tmr,creat_tmr,(INPROCTAG | INSYNCTAG | indx),FALSE);
		break;
	case IN_DEAD:
		if(step == IN_BCLEANUP){
			return(GLsuccess);
#ifdef OLD_SU
		} else  if(step == IN_SU){
			IN_SDPTAB[indx].procstep = IN_ESU;
			return(GLsuccess);
#endif
		}
	case IN_INVSTATE:
	case IN_MAXSTATE:
	default:
		//INIT_ERROR(("Process \"%s\" invalid procstate %s step %s",IN_LDPTAB[indx].proctag, IN_PROCSTNM(IN_SDPTAB[indx].procstate),IN_SQSTEPNM(step)));
    printf("Process \"%s\" invalid procstate %s step %s\n",
           IN_LDPTAB[indx].proctag, IN_PROCSTNM(IN_SDPTAB[indx].procstate),IN_SQSTEPNM(step));
		return(GLfail);

	}

	//INIT_DEBUG((IN_DEBUG | IN_SSEQTR | IN_RSTRTR),(POA_INF,"INsync(): returned successfully proc %s step %s", IN_LDPTAB[indx].proctag, IN_SQSTEPNM(step)));
  printf("INsync(): returned successfully proc %s step %s\n",
         IN_LDPTAB[indx].proctag, IN_SQSTEPNM(step));
	return(GLsuccess);
}

/*
** NAME:
**	INpkill	- Kill a process at a selected process index.
**
** DESCRIPTION:
**	This routine kills a process at the selected index in the
**	process table. It also updates process syncsteps based on
**	success or failure of the kill() system call.
**
** INPUTS:
**	indx 	- index in process table
**	success_step - value process procstep should be set to when
**			kill was successful
**	failure_step - value process procstep should be set to when 
**			kill failed
**
** RETURNS:
**
** CALLS:
**
** CALLED BY:
*	INsync()
**
** SIDE EFFECTS:
*/

Void
INpkill(U_short indx, IN_SYNCSTEP success_step, IN_SYNCSTEP failure_step){
	int ret = 0;

	if(IN_LDPTAB[indx].third_party == FALSE){ 
		if( IN_LDPTAB[indx].pid > 1){
			ret = kill(IN_LDPTAB[indx].pid,SIGKILL);
		}
	} else if(IN_LDPTAB[indx].tid != IN_FREEPID){
		IN_SDPTAB[indx].procstep = IN_ECLEANUP;
		ret = thr_kill(IN_LDPTAB[indx].tid,SIGTERM);
	}

	if(ret < 0 || IN_LDPTAB[indx].pid < 1) {
		/* If kill unsuccessful act as if process
		** was killed.  Otherwise rely on INdeath()
		** to discover that the process died and handle
		** things in standard manner.
		** If kill() was successfull but the process
		** never dies, the timer on IN_SU step will
		** expire, and an old version of the process
		** will be restarted even if it is a duplicate.
		*/
		//INIT_ERROR(("INpkill: kill failed for %s errno = %d",IN_LDPTAB[indx].proctag,errno));
    printf("INpkill: kill failed for %s errno = %d\n",
           IN_LDPTAB[indx].proctag,errno);
		IN_SDPTAB[indx].procstate = IN_DEAD;
		IN_SDPTAB[indx].procstep = failure_step;
		IN_LDPTAB[indx].pid = IN_FREEPID;
		return;
	}

	INSETTMR(INproctmr[indx].sync_tmr,INPCLEANUPTMR,(INPROCTAG | INSYNCTAG | indx),FALSE);
	IN_SDPTAB[indx].procstep = success_step;
}


/*
** NAME:
**	INnext_rlvl	- Select the next group of processes to be cycled
**			  through their PROCINIT steps.
**
** DESCRIPTION:
**	This routine sets up the next set of processes which are to be
**	run through their PROCINIT routines.  The function first scans
**	the processes in the current initialization to get the lowest
**	run level with processes which have yet to be cycled through
**	their PROCINIT routines.  After determining the next run level
**	for PROCINIT processing, all of the processes at that run level
**	are marked for further initialization.
**
** INPUTS:
**
** RETURNS:
**
** CALLS:
**
** CALLED BY:
*	INsequence()
**
** SIDE EFFECTS:
*/

Void
INnext_rlvl()
{
	U_short	i;

	//INIT_DEBUG((IN_DEBUG | IN_SSEQTR | IN_RSTRTR),(POA_INF,"INnext_rlvl(): entered"));
  printf("INnext_rlvl(): entered\n");

	for(i = 0; i < IN_SNPRCMX; i++){
		// Skip empty table entries and processes that already failed inits
		// Failed processes may be non critical and simply failed initialization
		// and may never reach PROCINIT
		if(IN_VALIDPROC(i) && (IN_LDPTAB[i].failed_init == FALSE)){
			if((IN_LDPTAB[i].run_lvl == IN_LDSTATE.sync_run_lvl) &&
			   (IN_SDPTAB[i].procstep < IN_EPROCINIT)){ 
				// Not all processes at current run level completed
				// PROCINIT yet.
				return;
			}
		}
	}

	// At this point all processes at current sync_run_lvl completed PROCINIT
	// Sequence them to steady and find next run level

	
	U_char run_lvl = IN_LDSTATE.run_lvl;	/* start at max value... */

	for(i = 0; i < IN_SNPRCMX; i++){
		if(IN_VALIDPROC(i) && (IN_LDPTAB[i].syncstep != INV_STEP)){
      if(IN_LDPTAB[i].run_lvl == IN_LDSTATE.sync_run_lvl){
				IN_LDPTAB[i].syncstep = IN_STEADY;
				//INIT_DEBUG((IN_DEBUG | IN_SSEQTR | IN_RSTRTR),(POA_INF,"INnext_rlvl(): proc %s released to STEADY",IN_LDPTAB[i].proctag));
        printf("INnext_rlvl(): proc %s released to STEADY",
               IN_LDPTAB[i].proctag);
			} else if((IN_LDPTAB[i].run_lvl < IN_LDSTATE.sync_run_lvl) &&
                (IN_LDSTATE.initstate == INITING) &&
                (IN_LDPTAB[i].syncstep != IN_STEADY)){
				//INIT_ERROR(("%s != IN_STEADY proc %s, runlvl %d < sync_run_lvl %d",IN_SQSTEPNM(IN_LDPTAB[i].syncstep),IN_LDPTAB[i].proctag,IN_LDPTAB[i].run_lvl,IN_LDSTATE.sync_run_lvl));
        printf("%s != IN_STEADY proc %s, runlvl %d < sync_run_lvl %d\n",
               IN_SQSTEPNM(IN_LDPTAB[i].syncstep),IN_LDPTAB[i].proctag,IN_LDPTAB[i].run_lvl,IN_LDSTATE.sync_run_lvl);
			}
			// Sync processes that were steady through procinit
			// during level 3 initialization
			if((IN_LDPTAB[i].run_lvl > IN_LDSTATE.sync_run_lvl) && 
			   (IN_LDPTAB[i].syncstep <= IN_PROCINIT || 
          (IN_LDPTAB[i].syncstep == IN_STEADY && 
           IN_LDSTATE.sn_lvl == SN_LV3 && IN_LDPTAB[i].on_active)) &&
			   (IN_LDPTAB[i].run_lvl <= run_lvl)){
				run_lvl = IN_LDPTAB[i].run_lvl;
			}
		}
	}

	if(IN_LDSTATE.sync_run_lvl == IN_LDSTATE.run_lvl){
		return;
	}

	IN_LDSTATE.sync_run_lvl = run_lvl;
	//INIT_DEBUG((IN_DEBUG | IN_SSEQTR | IN_RSTRTR),(POA_INF,"INnext_rlvl(): new run_lvl %d",run_lvl));
  printf("INnext_rlvl(): new run_lvl %d\n",
         run_lvl);

	/* Go through all the processes with the run level equal to current
	** run level and update process syncsteps.
	** Do not try to synchronize processes that are already in IN_STEADY
	** since we are allowing partial process sequencing during SUs
	** If this is level 3, revert processes already running to IN_PROCINIT.
	*/

	for(i = 0; i < IN_SNPRCMX; i++){
		if(IN_VALIDPROC(i) && (IN_LDPTAB[i].syncstep != INV_STEP) &&
       (IN_LDPTAB[i].run_lvl == IN_LDSTATE.sync_run_lvl) &&
       (IN_LDPTAB[i].syncstep != IN_STEADY || (IN_LDSTATE.sn_lvl == SN_LV3 && IN_LDPTAB[i].on_active))){
			IN_LDPTAB[i].syncstep = IN_PROCINIT;
			if(IN_SDPTAB[i].procstep == IN_STEADY){
				IN_SDPTAB[i].procstep = IN_ESYSINIT;
			}
			//INIT_DEBUG((IN_DEBUG | IN_SSEQTR | IN_RSTRTR),(POA_INF,"INnext_rlvl(): proc %s released to PROCINIT",IN_LDPTAB[i].proctag));
      printf("INnext_rlvl(): proc %s released to PROCINIT\n",
             IN_LDPTAB[i].proctag);
		}
	}

}

#ifdef OLD_SU
/*
** NAME:
**	INmvsufiles	- Make associated SU files consistent with objects
**
** DESCRIPTION:
**	This routine manipulates files associated with a given object file 
**	to make them consistent, i.e. new files/images when new objects
**	are used and old files when old objects are used.
**
** INPUTS:
**	indx - process table index of the process that must be made consistent
**		if indx == SN_PRCMX update all objects in the SU
** 	sn_lvl - level of the initiaization needed to update this process
** 	suaction - action to be taken: apply, backout or commit
**
** RETURNS:
**	GLsuccess - if all files successfully moved
**	GLfail	-   if failures encountered
**
** CALLS:
**
** CALLED BY:
*	INsequence(), INautobkout, INrcvmsg()
**
** SIDE EFFECTS:
** 	Files associated with a given object are updated.
*/

GLretVal
INmvsufiles(U_short indx, SN_LVL &sn_lvl, INSUACTION suaction) 
{
	int	i;
	int	su_max_indx = SU_MAX_OBJ - 1;
	int	su_indx = 0;
	GLretVal  ret = GLsuccess;
	struct stat stbuf;
	Bool	found;

	//INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INmvsufiles(): entered"));
  printf("INmvsufiles(): entered\n");

	if(indx < IN_SNPRCMX){
		/* Action to be taken on a single process.
		** Find it's index into INsudata[].
		*/

		if((i = INsudata_find(IN_LDPTAB[indx].pathname)) == SU_MAX_OBJ){
			//INIT_ERROR(("INmvsufiles: %s not in INsudata",IN_LDPTAB[indx].proctag));
      printf("INmvsufiles: %s not in INsudata\n",
             IN_LDPTAB[indx].proctag);
			return(GLfail);
		} else {
			su_max_indx = i;
			su_indx = i;
			sn_lvl = INsudata[i].sn_lvl;
		}
	}

	char tmp_pathname[IN_PATHNMMX+4];

	for(;su_indx <= su_max_indx && INsudata[su_indx].obj_path[0] != '\0'; su_indx++){
		switch(suaction){
		case INSU_APPLY:
			/* If all files were selected during apply, that means that
			** only non-object associated files should be moved,
			** i.e. object_path == "*".
			*/
			if(indx == IN_SNPRCMX && strcmp(INsudata[su_indx].obj_path,"*") != 0){
				continue;
			}
			
			/* Ignore INIT's entry */
			if(su_indx == INinit_su_idx){
				continue;
			}

			/* For objects that are common to several permement processes,
			** i.e. lmt, move associated files only once.
			*/
			if(INsudata[su_indx].applied == TRUE){
				continue;
			}

			for(i = 0; i < SU_MAX_OFILES && INsudata[su_indx].file_path[i][0] != '\0'; i++){
				/* Move current file to file.old if this is not a new file */
				if(INsudata[su_indx].new_file[i] == FALSE){
					strcpy(tmp_pathname,INsudata[su_indx].file_path[i]);
					strcat(tmp_pathname,".old");
					if(rename(INsudata[su_indx].file_path[i],tmp_pathname) < 0){
						//CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO CREATE %s, errno %d",tmp_pathname,errno);
            printf("REPT INIT ERROR FAILED TO CREATE %s, errno %d\n",
                   tmp_pathname,errno);
						return(GLfail);
					}
				} else {
					if(INgetpath(INsudata[su_indx].file_path[i],FALSE) == GLsuccess){
						//CR_PRM(POA_INF,"REPT INIT ERROR FILE %s MARKED AS NEW ALREADY EXISTS",INsudata[su_indx].file_path[i]);
            printf("REPT INIT ERROR FILE %s MARKED AS NEW ALREADY EXISTS",
                   INsudata[su_indx].file_path[i]);
						return(GLfail);
					}
				}

				/* Move file.new to current file */
				strcpy(tmp_pathname,INsudata[su_indx].file_path[i]);
				strcat(tmp_pathname,".new");
				if(rename(tmp_pathname,INsudata[su_indx].file_path[i]) < 0){
					//CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO CREATE %s, errno %d",tmp_pathname,errno);
          printf("REPT INIT ERROR FAILED TO CREATE %s, errno %d\n",
                 tmp_pathname,errno);
					return(GLfail);
				}
			}

			INsudata[su_indx].applied = TRUE;

			break;
		case INSU_BKOUT:
			/* Ignore INIT's entry */
			if(su_indx == INinit_su_idx){
				continue;
			}
			for(i = 0; i < SU_MAX_OFILES && INsudata[su_indx].file_path[i][0] != '\0'; i++){
				/* Move file.old to current file if file.old existed*/
				strcpy(tmp_pathname,INsudata[su_indx].file_path[i]);
				strcat(tmp_pathname,".old");
				if(stat(tmp_pathname,&stbuf) < 0){
					found = FALSE;
				} else {
					found = TRUE;
				}

				if(found == TRUE && rename(tmp_pathname,INsudata[su_indx].file_path[i]) < 0){
					//CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO CREATE %s, errno %d",tmp_pathname,errno);
          printf("REPT INIT ERROR FAILED TO CREATE %s, errno %d",
                 tmp_pathname,errno);
					ret = GLfail;
				}

				/* In many cases, the file for the process that is being
				** updated is not in the process table or we did not yet
				** create file.old, instead file.new is still here.
				** If that is the case, remove file.new
				*/ 
				strcpy(tmp_pathname,INsudata[su_indx].file_path[i]);
				strcat(tmp_pathname,".new");
				if(stat(tmp_pathname,&stbuf) >= 0 && unlink(tmp_pathname) < 0){
					//CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO REMOVE %s, errno %d",tmp_pathname,errno);
          printf("REPT INIT ERROR FAILED TO REMOVE %s, errno %d\n",
                 tmp_pathname,errno);
					ret = GLfail;
				}

				if(INsudata[su_indx].new_file[i] == TRUE){
					/* This file was added in this SU, delete it */
					if(stat(INsudata[su_indx].file_path[i],&stbuf) >= 0
					   && unlink(INsudata[su_indx].file_path[i]) < 0){
						//CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO REMOVE %s, errno %d",tmp_pathname,errno);
            printf("REPT INIT ERROR FAILED TO REMOVE %s, errno %d\n",
                   tmp_pathname,errno);
						ret = GLfail;
					}
				}
			}

			/* Check for pseudo object files */
			if(strcmp(INsudata[su_indx].obj_path,"*") == 0){
				break;
			}

			/* Remove the obj if it was new in this SU */
			if(INsudata[su_indx].new_obj == TRUE){
				if(stat(INsudata[su_indx].obj_path,&stbuf) >=0 &&
				   unlink(INsudata[su_indx].obj_path) < 0){
					//CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO REMOVE %s, errno %d",INsudata[su_indx].obj_path,errno);
          printf("REPT INIT ERROR FAILED TO REMOVE %s, errno %d\n",
                 INsudata[su_indx].obj_path,errno);
				}
			}
			/* Remove obj.new if exists */
			strcpy(tmp_pathname,INsudata[su_indx].obj_path);
			strcat(tmp_pathname,".new");
			if(stat(tmp_pathname,&stbuf) < 0){
				break;
			}
			if(unlink(tmp_pathname) < 0){
				//CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO REMOVE %s, errno %d",tmp_pathname,errno);
        printf("REPT INIT ERROR FAILED TO REMOVE %s, errno %d\n",
               tmp_pathname,errno);
				ret = GLfail;
			}
			break;
		case INSU_COMMIT:
			/* Ignore INIT's entry */
			if(su_indx == INinit_su_idx){
				continue;
			}
			for(i = 0; i < SU_MAX_OFILES && INsudata[su_indx].file_path[i][0] != '\0'; i++){
				/* Check for existence of *.new and if exists that
				** means that that file was never applied.  Simply
				** rename it to official.
				*/
				strcpy(tmp_pathname,INsudata[su_indx].file_path[i]);
				strcat(tmp_pathname,".new");

				if(stat(tmp_pathname,&stbuf) >= 0){
					if(rename(tmp_pathname,INsudata[su_indx].file_path[i]) < 0){
						//CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO CREATE %s, errno %d",INsudata[su_indx].obj_path,errno);
            printf("REPT INIT ERROR FAILED TO CREATE %s, errno %d\n",
                   INsudata[su_indx].obj_path,errno);
						ret = GLfail;
					}
					/* There should not be both .new and .old files present */
					continue;
				}

				/* If this is a new file do nothing */
				if(INsudata[su_indx].new_file[i] == TRUE){
					continue;
				}
				/* Remove file.old */
				strcpy(tmp_pathname,INsudata[su_indx].file_path[i]);
				strcat(tmp_pathname,".old");
				if(unlink(tmp_pathname) < 0){
					//CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO REMOVE %s, errno %d",tmp_pathname,errno);
          printf("REPT INIT ERROR FAILED TO REMOVE %s, errno %d\n",
                 tmp_pathname,errno);
					ret = GLfail;
				}
			}
			
			/* Check for pseudo object files, or files that are
			** not really part of this SU
			*/
			if(strcmp(INsudata[su_indx].obj_path,"*") == 0 ||
			   INsudata[su_indx].changed == FALSE ){
				break;
			}

			/* Move obj.new to current file */
			strcpy(tmp_pathname,INsudata[su_indx].obj_path);
			strcat(tmp_pathname,".new");
			if(rename(tmp_pathname,INsudata[su_indx].obj_path) < 0){
				//CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO CREATE %s, errno %d",INsudata[su_indx].obj_path,errno);
        printf("REPT INIT ERROR FAILED TO CREATE %s, errno %d\n",
               INsudata[su_indx].obj_path,errno);
				ret = GLfail;
			}
			break;
		default:
			//INIT_ERROR(("INmvsufiles: invalid suaction %d",suaction));
      printf("INmvsufiles: invalid suaction %d\n",
             suaction);
			return(GLfail);
		}
	}

	return(ret);
}

/*
** NAME:
**	INmvsuinit	- Move INIT executable during SUs
**
** DESCRIPTION:
**	This routine manipulates INIT executable if it is part of the SU.
**
** INPUTS:
** 	suaction - action to be taken: apply, backout or commit
**
** RETURNS:
**	GLsuccess - if all files successfully moved
**	GLfail	-   if failures encountered
**
** CALLS:
**
** CALLED BY:
*	INsequence(), INautobkout, INrcvmsg()
**
** SIDE EFFECTS:
**	INIT executable is updated
**	There is no ability to add new files that are associated with
**	INIT.  They must already exist.
*/

GLretVal
INmvsuinit(INSUACTION suaction) 
{
	int	i;
	int	su_indx = INinit_su_idx;
	GLretVal  ret = GLsuccess;
	struct stat stbuf;
	Bool	found;

	//INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INmvsuinit(): entered"));
  printf("INmvsuinit(): entered\n");

	char tmp_pathname[IN_PATHNMMX+4];
	
	/* Make sure INinit_su_idx points to correct entry */
	if(strncmp(INsudata[su_indx].obj_path,INinitpath,IN_PATHNMMX) != 0){
		//INIT_ERROR(("INinit_su_idx %d points to %s object",su_indx,
    //            INsudata[su_indx].obj_path));
    printf("INinit_su_idx %d points to %s object\n",su_indx,
           INsudata[su_indx].obj_path);
		return(GLfail);
	}

	switch(suaction){
	case INSU_APPLY:
		for(i = 0; i < SU_MAX_OFILES && INsudata[su_indx].file_path[i][0] != '\0'; i++){
			/* Move current file to file.old */
			strcpy(tmp_pathname,INsudata[su_indx].file_path[i]);
			strcat(tmp_pathname,".old");
			if(rename(INsudata[su_indx].file_path[i],tmp_pathname) < 0){
				//CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO CREATE %s, errno %d",tmp_pathname,errno);
        printf("REPT INIT ERROR FAILED TO CREATE %s, errno %d\n",
               tmp_pathname,errno);
				return(GLfail);
			}

			/* Move file.new to current file */
			strcpy(tmp_pathname,INsudata[su_indx].file_path[i]);
			strcat(tmp_pathname,".new");
			if(rename(tmp_pathname,INsudata[su_indx].file_path[i]) < 0){
				//CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO CREATE %s, errno %d",tmp_pathname,errno);
        printf("REPT INIT ERROR FAILED TO CREATE %s, errno %d\n",
               tmp_pathname,errno);
				return(GLfail);
			}
		}
		if(INsudata[su_indx].changed == FALSE){
			/* INIT has not changed, simply cause it to get restarted.
			** That will also result in reading initlist.
			*/
			return(GLsuccess);
		}

		/* Move current init file to init.old */
		strcpy(tmp_pathname,INsudata[su_indx].obj_path);
		strcat(tmp_pathname,".old");
		if(rename(INsudata[su_indx].obj_path,tmp_pathname) < 0){
			//CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO CREATE %s, errno %d",tmp_pathname,errno);
      printf("REPT INIT ERROR FAILED TO CREATE %s, errno %d\n",
             tmp_pathname,errno);
			return(GLfail);
		}

		/* Move file.new to current file */
		strcpy(tmp_pathname,INsudata[su_indx].obj_path);
		strcat(tmp_pathname,".new");
		if(rename(tmp_pathname,INsudata[su_indx].obj_path) < 0){
			CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO CREATE %s, errno %d",tmp_pathname,errno);
			return(GLfail);
		}

		break;
  case INSU_BKOUT:
    for(i = 0; i < SU_MAX_OFILES && INsudata[su_indx].file_path[i][0] != '\0'; i++){
      /* Move file.old to current file if file.old existed*/
      strcpy(tmp_pathname,INsudata[su_indx].file_path[i]);
      strcat(tmp_pathname,".old");
      if(stat(tmp_pathname,&stbuf) < 0){
        found = FALSE;
      } else {
        found = TRUE;
      }

      if(found == TRUE && rename(tmp_pathname,INsudata[su_indx].file_path[i]) < 0){
        //CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO CREATE %s, errno %d",tmp_pathname,errno);
        printf("REPT INIT ERROR FAILED TO CREATE %s, errno %d\n",
               tmp_pathname,errno);
        ret = GLfail;
      }

      /* In many cases, the file for the process that is being
      ** updated is not in the process table or we did not yet
      ** create file.old, instead file.new is still here.
      ** If that is the case, remove file.new
      */ 
      strcpy(tmp_pathname,INsudata[su_indx].file_path[i]);
      strcat(tmp_pathname,".new");
      if(stat(tmp_pathname,&stbuf) < 0){
        continue;
      }
      if(unlink(tmp_pathname) < 0){
        //CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO REMOVE %s, errno %d",tmp_pathname,errno);
        printf("REPT INIT ERROR FAILED TO REMOVE %s, errno %d\n",
               tmp_pathname,errno);
        ret = GLfail;
      }
    }

    if(INsudata[su_indx].changed == FALSE){
      /* INIT has not changed, simply reread initlist */
      return(GLsuccess);
    }

    /* Remove obj.new if exists */
    strcpy(tmp_pathname,INsudata[su_indx].obj_path);
    strcat(tmp_pathname,".new");
    if(stat(tmp_pathname,&stbuf) >= 0){
      if(unlink(tmp_pathname) < 0){
        //CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO REMOVE %s, errno %d",tmp_pathname,errno);
        printf("REPT INIT ERROR FAILED TO REMOVE %s, errno %d\n",
               tmp_pathname,errno);
        ret = GLfail;
      }
      break;
    }

    /* Move init.old to init file if init.old existed*/
    strcpy(tmp_pathname,INsudata[su_indx].obj_path);
    strcat(tmp_pathname,".old");
    if(stat(tmp_pathname,&stbuf) < 0){
      found = FALSE;
    } else {
      found = TRUE;
    }

    if(found == TRUE && rename(tmp_pathname,INsudata[su_indx].obj_path) < 0){
      //CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO CREATE %s, errno %d",tmp_pathname,errno);
      printf("REPT INIT ERROR FAILED TO CREATE %s, errno %d\n",
             tmp_pathname,errno);
      ret = GLfail;
    }

    break;
  case INSU_COMMIT:
    for(i = 0; i < SU_MAX_OFILES && INsudata[su_indx].file_path[i][0] != '\0'; i++){
      /* If this is a new file do nothing */
      if(INsudata[su_indx].new_file[i] == TRUE){
        continue;
      }

      /* Remove file.old */
      strcpy(tmp_pathname,INsudata[su_indx].file_path[i]);
      strcat(tmp_pathname,".old");
      if(unlink(tmp_pathname) < 0){
        //CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO REMOVE %s, errno %d",tmp_pathname,errno);
        printf("REPT INIT ERROR FAILED TO REMOVE %s, errno %d",tmp_pathname,errno);
        ret = GLfail;
      }
    }
			
    /* If init file was not changed, skip the remove code */
    if(INsudata[su_indx].changed == FALSE){
      break;
    }

    /* Remove init.old */
    strcpy(tmp_pathname,INsudata[su_indx].obj_path);
    strcat(tmp_pathname,".old");
    if(unlink(tmp_pathname) < 0){
      //CR_PRM(POA_INF,"REPT INIT ERROR FAILED TO REMOVE %s, errno %d",tmp_pathname,errno);
      printf("REPT INIT ERROR FAILED TO REMOVE %s, errno %d\n",
             tmp_pathname,errno);
      ret = GLfail;
    }
    break;
  default:
    //INIT_ERROR(("INmvsufiles: invalid suaction %d",suaction));
    printf("INmvsufiles: invalid suaction %d\n",
           suaction);
    return(GLfail);
  }

	return(ret);
}

#endif

/*
** NAME:
**	INgetRealName	- Return a real name for a process that may use GQ
**
** DESCRIPTION:
**	This function checks to see if process may be using global queue
**	and returns that name instead of standard process name
**
** INPUTS:
** 	name 		- actual queue name to be filled in
**	procname 	- process name as defined in initlist
**
** RETURNS:
**	TRUE if process uses global queue, FALSE otherwise
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
*/
Bool
INgetRealName(char* name, char* procname)
{
	MHqid	qid;
	// Check if process may be using global queue
	name[0] = '_';
	name[1] = 0;
	strncat(name, procname, IN_NAMEMX - 2);
	if(INevent.getMhqid(name, qid) == GLsuccess){
		return(TRUE);
	} else {
		// Process does not use global queue
		strcpy(name, procname);
		return(FALSE);
	}
}
