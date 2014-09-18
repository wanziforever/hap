//
// DESCRIPTION:
// 	This file contains INIT subsystem audits
//
// FUNCTIONS:
//	INaudit()	- Run INIT audits
//	INescalate()	- Determine system escalation level
//	INsysreset()	- Initiate a system-wide reset activity
//	INsysboot()	- Call for a UNIX boot...
//	INcheck_progress() - Verify that a process is making progress
//	INwait_exit()	- Wait before exiting so INIT does not die too much
//	INic_crit_up()	- Check if IC critical processes are up
//	INcheck_sanity()- Check process sanity
//	INvmem_check()	- Check status of virtual memory
// NOTES:
//

#include <stdlib.h>
#include <sysent.h>

#include <string.h>
#include <time.h>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/param.h>
#include <thread.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <sys/sysinfo.h>
#include <fcntl.h>

#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "cc/hdr/init/INinit.hh"
#include "cc/hdr/init/INproctab.hh"
#include "cc/init/proc/INmsgs.hh"
#include "cc/init/proc/INlocal.hh"
#include "cc/hdr/init/INinitialize.hh"
//#include "cc/hdr/cr/CRindStatus.hh"
//#include "cc/hdr/ft/FTdr.H"
//#include "cc/hdr/ft/FTreturns.H"
#include "cc/hdr/init/INdr.hh"
//#include "cc/hdr/ft/FTbladeMsg.H"

extern Bool INissimplex;

/*
** NAME:
**	INaudit()
**
** DESCRIPTION:
**	This routine audits INIT's shared memory data.
**
** INPUTS:
**
**	init_flg: TRUE  - Check for consistency of timer settings
**		 FALSE - Don't check timer settings against data in
**			  shared memory since the audit is run during init.
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
INaudit(Bool init_flg)
{
	GLretVal ret = GLsuccess;
	static	int entry_count = 0;
	register IN_PTAB * sdp;
	register IN_PROCESS_DATA *ldp,*eldp;

	//INIT_DEBUG((IN_DEBUG | IN_AUDTR),(POA_INF,"INaudit(): entered"));
  printf("INaudit(): entered\n");

	if(init_flg == FALSE){
		entry_count ++;
		/* Activity every 8 seconds	*/
		/* Check for process deaths	*/
		INgrimreaper();
		/* Check process sanity 	*/
		INcheck_sanity();
		/* Check virtual memory status.  This call of INvmem_check()
		** will only generate alarmed messages if status changed since
		** last check.  If alarmed condition exists INvmem_check() will
		** also be called from a timer to periodically report persisting
		** conditions.
		*/
		INvmem_check(FALSE,FALSE);

		/* Write current time in a timestamp file */
		int timefd = open(INtimeFile, O_CREAT|O_WRONLY|O_SYNC, 0600);
		if(timefd >= 0 ){
			time_t curtime = time(NULL);
			write(timefd, &curtime, sizeof(curtime));
			close(timefd);
		} else {
			//CRDEBUG_PRINT(1, ("Failed to open/create timestamp file, errno %d", errno));
      printf("Failed to open/create timestamp file, errno %d\n", errno);
		}

		/* Check to see if messaging mutex was cleared */
		if(INmhmutex_cleared){
			int q;
			char	message[3000];
			extern  char MHmutexList[][MHmaxNameLen + 1];
			memset(message, 0x0, sizeof(message));
			sprintf(message, "Cleared messaging mutex\n");
			for(q = 0; MHmutexList[q][0] != 0; q++){
				sprintf(&message[strlen(message)], "%s\n", MHmutexList[q]);
				MHmutexList[q][0] = 0;
			}
			//INIT_DEBUG((IN_ALWAYSTR), (POA_INF,message));
      printf("Cleared messaging mutex\n");
			INmhmutex_cleared = FALSE;
		}

		if(INwarnMissed > 0){
			//CR_PRM(POA_INF, "REPT INIT ERROR MAIN THREAD DID NOT RUN FOR %d SEC", INwarnMissed/2);
      printf("REPT INIT ERROR MAIN THREAD DID NOT RUN FOR %d SEC\n",
             INwarnMissed/2);
			INwarnMissed = 0;
		}

		if(INticksMissed > 0){
			//CR_PRM(POA_INF, "REPT INIT ERROR AUDIT THREAD DID NOT RUN FOR %d TICKS", INticksMissed);
      printf("REPT INIT ERROR AUDIT THREAD DID NOT RUN FOR %d TICKS\n",
             INticksMissed);
			INticksMissed = 0;
		}
		
		/* Make sure check lead timer set if we do not yet have lead
       if(INetype == EHBOTH && INevent.getLeadCC() < 0 &&  INcheckleadtmr.tindx == -1 && IN_LDSTATE.sn_lvl == SN_LV4 && IN_LDSTATE.initstate == INITING){
       INIT_ERROR(("INcheckleadtmr.tindx == -1 but lead not present"));
       INSETTMR(INcheckleadtmr, INCHECKLEADTMR, INCHECKLEADTAG, TRUE);
       }
       /* Activity every 16 seconds	*/
		if((entry_count & 0x1) == 0){
			/* Check forward progress 	*/
			INcheck_progress();
		}

		char qInUse[MHmaxQid];
		memset(qInUse, 0x0, sizeof(qInUse));

		/* Activity every 56 seconds	*/
		if((entry_count % 7) == 0){
			// Decrement system error counts
			if((IN_SDERR_COUNT =- IN_LDE_DECRATE) < 0){
				IN_SDERR_COUNT = 0;
			}
	
			// Decrement the number of INIT deaths
			if(IN_LDSTATE.init_deaths > 0){
				IN_LDSTATE.init_deaths --;
			}

			// Decrement INIT assert count
			if(--INerr_cnt < 0){
				INerr_cnt = 0;	
			}
	
			sdp = IN_SDPTAB;
			ldp = IN_LDPTAB;
			eldp = &IN_LDPTAB[IN_SNPRCMX];
			for(; ldp < eldp; sdp++,ldp++){

        if(ldp->msgh_qid >= 0 && ldp->msgh_qid < MHmaxQid){
          qInUse[ldp->msgh_qid] = 1;
        }

				if(ldp->syncstep == IN_MAXSTEP){
					continue;
				} 
	
				if((sdp->error_count -= ldp->error_dec_rate) < 0){
					sdp->error_count = 0;
				}
			}
			int i;
			for(i = MHminTempProc; i < MHmaxQid; i++){
				if(INqInUse[i] != qInUse[i]){
					//CRDEBUG_PRINT(1, ("INqInUse[%d] = %d does not match actual %d", i, INqInUse[i], qInUse[i]));
          printf("INqInUse[%d] = %d does not match actual %d\n",
                 i, INqInUse[i], qInUse[i]);
					INqInUse[i] = qInUse[i];
				}
			}
		}
		

		/* Run the rest of the audit every 256 sec */
		if((entry_count & 0x1f) != 0){
			return(GLsuccess);
		}

		if((entry_count & 0x3f) == 0 && IN_ISACTIVE(IN_LDCURSTATE)){
			/* Audit initlist every 512 sec */
			(void)INrdinls(FALSE,TRUE);
			INcheckDirSize();
		}

	}

	/* The remainder of this function runs every 256 sec,
	** unless it has been called during init.
	*/

	// Update INworkflg and init timers
	Bool old_flg = INworkflg;
	
	(Void) INinitover();

	if(INworkflg != old_flg){
		//INIT_ERROR(("INworkflg %d != old_flg %d",INworkflg,old_flg));
    printf("INworkflg %d != old_flg %d\n",
           INworkflg,old_flg);
	}

	if (IN_LDILIST[IN_PATHNMMX-1] != 0) {
		IN_LDILIST[IN_PATHNMMX-1] = (Char)0;

		//INIT_ERROR(("\"%s\" not null terminated", IN_LDILIST));
    printf("\"%s\" not null terminated\n", IN_LDILIST);
		ret = GLfail;
	}

#ifdef CC
	/* Force permissions of initlist to be always correct to prevent
	** failed boots.
	*/
	chmod(IN_LDILIST,0644);
#endif

	if ((IN_LDINFO.ld_indx >= IN_NUMINITS) ||
	    (IN_LDINFO.uld_indx >= IN_NUMINITS)) {
		//INIT_ERROR(("IN_LDINFO ld idx %d, unld idx %d",IN_LDINFO.ld_indx, IN_LDINFO.uld_indx ));
    printf("IN_LDINFO ld idx %d, unld idx %d\n",
           IN_LDINFO.ld_indx, IN_LDINFO.uld_indx );
		ret = GLfail;
		IN_LDINFO.ld_indx = 0;
		IN_LDINFO.uld_indx = 0;
	}


	if (init_flg == FALSE) {
		/*
		 * Check timers which should always be set independent of
		 * the current system initialization state:
		 */
		if (INpolltmr.tindx < 0 && INworkflg == TRUE) {
			//INIT_ERROR(("Null polltmr idx while INworkflg = %d",INworkflg));
      printf("Null polltmr idx while INworkflg = %d\n",
             INworkflg);
			ret = GLfail;
			INsettmr(INpolltmr,INITPOLL,(INITTAG|INPOLLTAG), TRUE, TRUE);
		}

		if (INarutmr.tindx < 0) {
			//INIT_ERROR(("Null arutmr idx in %s state", IN_STATENM(IN_LDSTATE.initstate) ));
      printf("Null arutmr idx in %s state\n",
             IN_STATENM(IN_LDSTATE.initstate));
			ret = GLfail;
			INSETTMR(INarutmr, IN_LDARUINT, INARUTAG, TRUE);
		}
	}

	if ((IN_LDSTATE.initstate == IN_NOINIT) ||
	    (IN_LDSTATE.initstate == IN_CUINTVL)) {
		/*
		 * No system-wide reset in progress...
		 */

		if (IN_LDSTATE.initstate == IN_NOINIT && IN_LDSTATE.sn_lvl != SN_NOINIT) {

			//INIT_ERROR(("Initstate %s with sn_lvl of %s",IN_STATENM(IN_LDSTATE.initstate), IN_SNLVLNM(IN_LDSTATE.sn_lvl)));
      printf("Initstate %s with sn_lvl of %s\n",
             IN_STATENM(IN_LDSTATE.initstate), IN_SNLVLNM(IN_LDSTATE.sn_lvl));
			ret = GLfail;
			IN_LDSTATE.sn_lvl = SN_NOINIT;
		}

		if (IN_LDSTATE.systep != IN_STEADY) {
			//INIT_ERROR(("Initstate %s with systep %s",IN_STATENM(IN_LDSTATE.initstate), IN_SQSTEPNM(IN_LDSTATE.systep)));
      printf("Initstate %s with systep %s\n",
             IN_STATENM(IN_LDSTATE.initstate), IN_SQSTEPNM(IN_LDSTATE.systep));
			IN_LDSTATE.systep = IN_STEADY;
			INworkflg = TRUE;
			INsettmr(INpolltmr,INITPOLL,(INITTAG|INPOLLTAG), TRUE, TRUE);
			ret = GLfail;
		}

		if (init_flg == FALSE) {
			if ((IN_LDSTATE.initstate == IN_CUINTVL) &&
			    (INinittmr.tindx < 0)) {
				//INIT_ERROR(("Null inittmr idx in IN_CUINTVL state"));
        printf("Null inittmr idx in IN_CUINTVL state\n");
				ret = GLfail;
				INSETTMR(INinittmr,IN_procdata->safe_interval, (INITTAG|INSEQTAG), FALSE);
			}

			if ((IN_LDSTATE.initstate == IN_NOINIT) &&
			    (INinittmr.tindx >= 0)) {
				//INIT_ERROR(("Non-null inittmr idx in IN_NOINIT state"));
        printf("Non-null inittmr idx in IN_NOINIT state\n");
				ret = GLfail;
				INCLRTMR(INinittmr);
			}
		}
	}
	else {
		/*
		 * System-wide reset is currently in progress...
		 */
		if (init_flg == FALSE && INworkflg == FALSE && IN_LDSTATE.initstate == INITING) {
			//INIT_ERROR(("INworkflg %d initstate %s", INworkflg, IN_STATENM(IN_LDSTATE.initstate)));
      printf("INworkflg %d initstate %s\n",
             INworkflg, IN_STATENM(IN_LDSTATE.initstate));
			ret = GLfail;
			INworkflg = TRUE;
		}

		/* Make sure that run_lvl and sync_run_lvl are consistent 	*/
		if((IN_LDSTATE.run_lvl < IN_LDSTATE.sync_run_lvl) && (IN_LDSTATE.sn_lvl != SN_LV3)){
			//INIT_ERROR(("sync_run_lvl %d > run_lvl %d",IN_LDSTATE.sync_run_lvl,IN_LDSTATE.run_lvl));
      printf("sync_run_lvl %d > run_lvl %d\n",
             IN_LDSTATE.sync_run_lvl,IN_LDSTATE.run_lvl);
			ret = GLfail;
			IN_LDSTATE.sync_run_lvl = IN_LDSTATE.run_lvl;
		}

		if (IN_LDSTATE.sn_lvl == SN_NOINIT) {
			//INIT_ERROR(("initstate %s with sn_lvl %s",IN_STATENM(IN_LDSTATE.initstate), IN_SNLVLNM(IN_LDSTATE.sn_lvl)));
      printf("initstate %s with sn_lvl %s\n",
             IN_STATENM(IN_LDSTATE.initstate), IN_SNLVLNM(IN_LDSTATE.sn_lvl));
			ret = GLfail;
			INescalate(SN_LV2,IN_INVSNLVL,IN_SOFT,INIT_INDEX);
		}

		if (init_flg == FALSE) {
			if (INinittmr.tindx > 0) {
				//INIT_ERROR(("non-null inittmr idx in initstate %s", IN_STATENM(IN_LDSTATE.initstate)));
        printf("non-null inittmr idx in initstate %s\n",
               IN_STATENM(IN_LDSTATE.initstate));
				ret = GLfail;
			}
		}

	}


	/*
	 * Now check process-specific data:
	 */
	Short	msgh_index = -1;
	sdp = IN_SDPTAB;
	ldp = IN_LDPTAB;

	for (int indx = 0; indx < IN_SNPRCMX; indx++,sdp++,ldp++) {
		if (ldp->syncstep == IN_MAXSTEP) {
			/* Audit some of the data */
			if(sdp->procstate != IN_INVSTATE ||
			   sdp->procstep != INV_STEP ||
			   sdp->ireq_lvl != SN_NOINIT || 
			   sdp->count != 1){
				//INIT_ERROR(("Bad sdata for empty proc procstate %s,procstep %s,ireq_lvl %s, count %d",IN_PROCSTNM(sdp->procstate),IN_SQSTEPNM(sdp->procstep),IN_SNLVLNM(sdp->ireq_lvl),sdp->count));
        printf("Bad sdata for empty proc procstate %s,procstep %s,ireq_lvl %s, count %d\n",
               IN_PROCSTNM(sdp->procstate),IN_SQSTEPNM(sdp->procstep),
               IN_SNLVLNM(sdp->ireq_lvl),sdp->count);
				sdp->procstate = IN_INVSTATE;
        sdp->procstep = INV_STEP;
        sdp->ireq_lvl = SN_NOINIT; 
        sdp->count = 1;
				ret = GLfail;
			}
			if(ldp->permstate != INVPROC ||
			   ldp->source != IN_SOFT ||
			   ldp->pid != IN_FREEPID ||
			   ldp->time_missedsan != 0){
				//INIT_ERROR(("Bad pdata for empty proc permstate %d, source %s, pid %d, time_missedsan %d",ldp->permstate,IN_SNSRCNM(ldp->source),ldp->pid,ldp->time_missedsan));
        printf("Bad pdata for empty proc permstate %d, source %s, pid %d, time_missedsan %d\n",
               ldp->permstate,IN_SNSRCNM(ldp->source),ldp->pid,ldp->time_missedsan);
				ldp->permstate = INVPROC;
        ldp->source = IN_SOFT;
        ldp->pid = IN_FREEPID;
        ldp->time_missedsan = 0;
				ret = GLfail;
			}
			continue;
		}

		if ((ldp->proctag[IN_NAMEMX-1] != 0) ||
		    (ldp->proctag[0] == 0)) {
			/*
			 * We force this to be corrected so we don't
			 * pass the end of the array while outputting
			 * subsequent messages:
			 */
			ldp->proctag[IN_NAMEMX-1] = (Char)0;

			//INIT_ERROR(("Non-null terminated or empty proctag  %s",ldp->proctag));
      printf("Non-null terminated or empty proctag  %s\n",
             ldp->proctag);
			ret = GLfail;
			// Should we just escalate at lvl 3??
		}

		if (strcmp(ldp->proctag,"MSGH") == 0) {
			msgh_index = indx;
		}

		if ((ldp->pathname[IN_PATHNMMX-1] != 0) ||
		    (ldp->pathname[0] == 0)) {
			/*
			 * Force this to be corrected...
			 */
			ldp->pathname[IN_PATHNMMX-1] = (Char)0;

			//INIT_ERROR(("Non-null terminated or empty pathname %s",ldp->pathname));
      printf("Non-null terminated or empty pathname %s\n",
             ldp->pathname);
			ret = GLfail;
			// Should we just escalate at lvl3 ??
		}
 
		if ((ldp->run_lvl > IN_LDSTATE.run_lvl) && (IN_LDSTATE.sn_lvl != SN_LV3)) {
			// During failover, processes at higher run level can exist
			//INIT_ERROR(("%s at higher run lvl %d than system run lvl %d",ldp->proctag, ldp->run_lvl, IN_LDSTATE.run_lvl));
      printf("%s at higher run lvl %d than system run lvl %d\n",
             ldp->proctag, ldp->run_lvl, IN_LDSTATE.run_lvl);
			ret = GLfail;
			ldp->run_lvl = IN_LDSTATE.run_lvl;
			// Either update the data from initlist file or escalate
			// to full application boot
		}

		if ((ldp->startstate != IN_ALWRESTART) &&
		    (ldp->startstate != IN_INHRESTART)) {
			//INIT_ERROR(("%s has an invalid startstate: %d",ldp->proctag, ldp->startstate));
      printf("%s has an invalid startstate: %d\n",
             ldp->proctag, ldp->startstate);
			ret = GLfail;
			ldp->startstate = IN_ALWRESTART;
		}

		if((ldp->startstate == IN_ALWRESTART) &&
		   (ldp->syncstep == INV_STEP) &&
		   (ldp->next_rstrt >= IN_NO_RESTART) &&
		   (sdp->procstate == IN_DEAD)){
			//INIT_ERROR(("%s has an invalid next_rstrt: %d",ldp->proctag, ldp->next_rstrt));
      printf("%s has an invalid next_rstrt: %d\n",
             ldp->proctag, ldp->next_rstrt);
			ret = GLfail;
		}

		if (ldp->uid < 0) {
			//INIT_ERROR(("%s has negative UID %d",ldp->proctag, ldp->uid));
      printf("%s has negative UID %d\n",ldp->proctag, ldp->uid);
			ret = GLfail;
			ldp->uid = 0; 
			// Ask for system init ?
		}

		if (ldp->priority >= IN_MAXPRIO) {
			//INIT_ERROR(("%s has invalid priority %d",ldp->proctag, ldp->priority));
      printf("%s has invalid priority %d",ldp->proctag, ldp->priority);
			ret = GLfail;
			ldp->priority = IN_MAXPRIO - 1;
		}

		// Check restart data for consistency

		if (init_flg == FALSE){ 
      if ((ldp->rstrt_cnt == 0) &&
		      (INproctmr[indx].rstrt_tmr.tindx >= 0)) {
        //INIT_ERROR(("%s has a non-null timer index with zero rstrt_cnt, procstep %s",ldp->proctag, IN_SQSTEPNM(sdp->procstep)));
        printf("%s has a non-null timer index with zero rstrt_cnt, procstep %s\n",
               ldp->proctag, IN_SQSTEPNM(sdp->procstep));
        ret = GLfail;
        INCLRTMR(INproctmr[indx].rstrt_tmr);
			}

      if(sdp->procstep == IN_STEADY && INproctmr[indx].sync_tmr.tindx >= 0){
        //INIT_ERROR(("Non-null sanity timer idx for %s with state of IN_STEADY",ldp->proctag));
        printf("Non-null sanity timer idx for %s with state of IN_STEADY\s",
               ldp->proctag);
        ret = GLfail;
        INCLRTMR(INproctmr[indx].sync_tmr);
			}
		}
		// Audit shared memory for the process
	} /* end of "for" loop */

	if (msgh_index != IN_LDMSGHINDX) {
		//INIT_ERROR(("MSGH index %d expected %d", msgh_index, IN_LDMSGHINDX));
    printf("MSGH index %d expected %d\n", msgh_index, IN_LDMSGHINDX);
		ret = GLfail;
		IN_LDMSGHINDX = msgh_index;
	}

	if (init_flg == FALSE && ((IN_LDSTATE.initstate == IN_NOINIT) ||
                            (IN_LDSTATE.initstate == IN_CUINTVL))) {
		if ((IN_LDMSGHINDX >= 0) && (IN_SDPTAB[IN_LDMSGHINDX].procstep == IN_STEADY) &&
		    (INetype == EHTMRONLY)) {
			//INIT_ERROR(("In initstate %s with MSGH index %d, state %s but event type is EHTMRONLY",IN_STATENM(IN_LDSTATE.initstate), IN_LDMSGHINDX,IN_SQSTEPNM(IN_SDPTAB[IN_LDMSGHINDX].procstep)));
      printf("In initstate %s with MSGH index %d, state %s but event type is EHTMRONLY\n",
             IN_STATENM(IN_LDSTATE.initstate), IN_LDMSGHINDX,
             IN_SQSTEPNM(IN_SDPTAB[IN_LDMSGHINDX].procstep));
			ret = GLfail;
			INprocinit();
		}
	}

	if(init_flg == FALSE && IN_LDSTATE.initstate == INITING){
		// Cannot really audit to see if run level was changed
		// because of race condition with INsequence()
		INnext_rlvl();
	}

	//INIT_DEBUG((IN_DEBUG | IN_AUDTR),(POA_INF,"INaudit(): returned %d", ret));
  printf("INaudit(): returned %d\n", ret);
	return(ret);
}


/*
** NAME:
**	INescalate() - Escalate to system-wide initialization
**
** DESCRIPTION:
**	This routine determines the next highest level of system
**	reset activity and then either invokes INsysreset() or
**	INsysboot() as appropriate.
**
** INPUTS:
**	err_code - Error code associated with this recovery action
**	source	 - Source of recovery (MANUAL, SOFTWARE, etc.)
**	sn_lvl	 - Requested initial level of initialization
**	indx 	- process index in process table
**
** RETURNS:
**
** CALLS:
*	INsysreset() - Call for a system-wide reset
**	INsysboot()  - Call for a UNIX boot
**
** CALLED BY:
**
** SIDE EFFECTS:
*/
Void
INescalate(SN_LVL sn_lvl,GLretVal err_code, IN_SOURCE source, Short indx)
{
#ifndef EES
	long 	lv4_count;
#endif
	//INIT_DEBUG((IN_DEBUG|IN_IREQTR),(POA_INF,"INescalate():Entered w/source %s current system recovery level %s \n\t init state %s, process %s, err_code %d, sn_lvl = %s",IN_SNSRCNM(source), IN_SNLVLNM(IN_LDSTATE.sn_lvl), IN_STATENM(IN_LDSTATE.initstate),(indx == INIT_INDEX ? IN_MSGHQNM:IN_LDPTAB[indx].proctag), err_code, IN_SNLVLNM(sn_lvl)));
  printf("INescalate():Entered w/source %s current system recovery level %s \n\t init "
         "state %s, process %s, err_code %d, sn_lvl = %s", IN_SNSRCNM(source),
         IN_SNLVLNM(IN_LDSTATE.sn_lvl), IN_STATENM(IN_LDSTATE.initstate),
         (indx == INIT_INDEX ? IN_MSGHQNM:IN_LDPTAB[indx].proctag), err_code,
         IN_SNLVLNM(sn_lvl));

	// Indicate that a process failed its init
	if(indx != INIT_INDEX){
		IN_LDPTAB[indx].failed_init = TRUE;
    /* Check if more processes should be scheduled in case this was the
    ** only process at this level and it failed its init. 
    ** If the process was critical, we will escalate anyway, otherwise
    ** if its restart is deferred, more processes can be scheduled
		*/
    INnext_rlvl();
		/* If process exited before it could access its shared memory
		** print an init request for it so we know the reason 
		** for its init.
		*/
		CRALARMLVL	alarm_lvl = POA_MAJ;
    if(IN_LDPTAB[indx].proc_category == IN_CP_CRITICAL ||
       IN_LDPTAB[indx].proc_category == IN_PSEUDO_CRITICAL){
      alarm_lvl = POA_CRIT;
    }
		switch(err_code){
		case INBADSHMEM:
			//CR_X733PRM(INadjustAlarm(alarm_lvl),IN_LDPTAB[indx].proctag,
      //           qualityOfServiceAlarm, softwareProgramAbnormallyTerminated, NULL, ";204",
      //           "REPT INIT %s REQUESTED %s INIT DUE TO INCOMPATIBLE OR CORRUPTED INIT SHARED MEMORY",
      //           IN_LDPTAB[indx].proctag,IN_SNLVLNM(sn_lvl));
      printf("REPT INIT %s REQUESTED %s INIT DUE TO INCOMPATIBLE OR CORRUPTED INIT SHARED MEMORY\n",
             IN_LDPTAB[indx].proctag,IN_SNLVLNM(sn_lvl));
			break;
		default:
			break;
		}
	}

	/* If the source of initialization is manual, perform it without escalation
	** or any status of software checks.  Only process level escalation requests
	** should have manual source.
	*/
	if(source == IN_MANUAL){
    if(IN_LDPTAB[indx].startstate == IN_INHRESTART){ 
			INdeadproc(indx,TRUE);
			return;
		}
		/* Clear the manual indication so we do not roll in manual inits */
		IN_LDPTAB[indx].source = IN_SOFT;
		if(sn_lvl < SN_LV2 && sn_lvl > SN_NOINIT){
			INsetrstrt(sn_lvl,indx,IN_MANUAL);
		} else {
			//INIT_ERROR(("Invalid sn_lvl = %d for manual process init",sn_lvl));
      printf("Invalid sn_lvl = %d for manual process init\n",sn_lvl);
		}
		return;
	}

#ifdef OLD_SU
	/* Trigger SU backout, regardless if process was part of SU or not.
	** Also, backout will not occur if the process was manually initialized.
	** If the process that has died will not be restarted, then the
	** backout will not be restarted. This applies equally to temporary
	** and permanent processes.
	** Note that backout occurs even if softchks are inhibited.
	*/

	Bool	proc_in_su = FALSE;
	if(indx != INIT_INDEX && err_code != IN_INTENTIONAL && 
     IN_LDPTAB[indx].startstate != IN_INHRESTART) {
		int	k;
		k = INsudata_find(IN_LDPTAB[indx].pathname);
		if(k < SU_MAX_OBJ){
			proc_in_su = TRUE;
		}
		INautobkout(FALSE,FALSE);
		if(IN_SDPTAB[indx].procstep == IN_BSU || 
		   (k < SU_MAX_OBJ && INsudata[k].new_obj == TRUE)){
			/* This process will either be restarted as part of 
			** autobackout code or it will be deleted.
			*/
			return;
		}
	}
#endif

	/* Check for system or process software check inhibits */
	if((IN_LDSTATE.softchk == IN_INHSOFTCHK) || 
	   ((indx != INIT_INDEX) && (IN_LDPTAB[indx].softchk == IN_INHSOFTCHK))){
		/* If software checks are inhibited and the process died
		** treat it as non-critical.
		*/
		if(indx != INIT_INDEX && IN_SDPTAB[indx].procstate == IN_DEAD){
			INdeadproc(indx,FALSE);
		}
		return;
	}


	/* Perform escalation on INIT only */
	if(indx == INIT_INDEX){
		if(IN_LDCURSTATE == S_OFFLINE){
			return;
		}
		if(sn_lvl < SN_LV2 && IN_LDSTATE.sn_lvl <  SN_LV2 ){ 
			/* Exit. UNIX /etc/init will recreate the INIT process	*/
			INwait_exit();
			//CR_PRM(INadjustAlarm(POA_CRIT),"REPT INIT REQUESTED %s INIT OF INIT DUE TO %d",
			//CR_X733PRM(INadjustAlarm(POA_CRIT),
      //           "INIT Subsystem",
      //           qualityOfServiceAlarm,
      //           thresholdCrossed,
      //           "Process Initialization",
      //           ";241",
      //           "REPT INIT REQUESTED %s INIT OF INIT DUE TO %d", IN_SNLVLNM(sn_lvl),err_code);
      printf("REPT INIT REQUESTED %s INIT OF INIT DUE TO %d\n",
             IN_SNLVLNM(sn_lvl),err_code);
			exit(0);
		} else if(sn_lvl < IN_LDSTATE.sn_lvl){
			sn_lvl = IN_LDSTATE.sn_lvl;
		}
	}


	/* Set sn_lvl to be at least current initialization level
	** if a process initialization requested during initialization 
	** by someone other than non-critical process.
	** After UNIX boot IN_LDSTATE is initialized to SN_LV4 so we will not
	** roll in UNIX boots unless another one is requested directly
	*/
	if((IN_LDSTATE.initstate == INITING) && (sn_lvl < IN_LDSTATE.sn_lvl) &&
	   (err_code != IN_INTENTIONAL) &&
	   (IN_LDPTAB[indx].proc_category != IN_NON_CRITICAL) && 
	   (IN_LDPTAB[indx].startstate != IN_INHRESTART)){ 
		sn_lvl = IN_LDSTATE.sn_lvl;
	}

	if((IN_LDSTATE.initstate == IN_CUINTVL) && (sn_lvl < IN_LDSTATE.sn_lvl) &&
	   (err_code != IN_INTENTIONAL) &&
	   (IN_LDPTAB[indx].proc_category != IN_NON_CRITICAL) && 
	   (IN_LDPTAB[indx].proc_category != IN_INIT_CRITICAL) && 
	   (IN_LDPTAB[indx].startstate != IN_INHRESTART)){ 
		sn_lvl = IN_LDSTATE.sn_lvl;
	}


	if(sn_lvl < SN_LV2){
		if(IN_LDPTAB[indx].startstate == IN_INHRESTART){
			/* Stop this process from running and leave it dead for forever */
			if(IN_LDPTAB[indx].permstate == IN_TEMPPROC){
				INdeadproc(indx,TRUE);
			} else {
				IN_LDPTAB[indx].next_rstrt = IN_NO_RESTART;
				INdeadproc(indx,FALSE);
			}
			return;
		}
		/* Make initialization to be at least at the process's current
		** initialization level unless exit was intentional 
		*/
		if((err_code != IN_INTENTIONAL || IN_LDPTAB[indx].sn_lvl < SN_LV2) && 
       sn_lvl < IN_LDPTAB[indx].sn_lvl){
			sn_lvl = IN_LDPTAB[indx].sn_lvl;
		}

		/* Non call processing processes cannot escalate the system
		** if they fail their level 4 inits.
		*/
		if((sn_lvl > SN_LV1) &&
       ((IN_LDPTAB[indx].proc_category == IN_NON_CRITICAL) || 
        (IN_LDPTAB[indx].proc_category == IN_INIT_CRITICAL))){ 
			sn_lvl = SN_LV1;
		}
	}
	
	if(IN_LDCURSTATE == S_OFFLINE){
		sn_lvl = SN_LV0;
	}
	switch(sn_lvl){
	case SN_LV0:
		/* Cause level 1 initialization if restart count exceeded */
		if(++IN_LDPTAB[indx].rstrt_cnt >= IN_LDPTAB[indx].rstrt_max){
			INsetrstrt(SN_LV1,indx,source);
		} else {
			INsetrstrt(SN_LV0,indx,source);
		}
		break;
	case SN_LV1:
    if(IN_LDPTAB[indx].sn_lvl >= SN_LV1 && err_code != IN_INTENTIONAL){

      /* This is already a second level 1 or higher initialization
      ** or we do not have all IC critical processes.
      ** If a process is critical, escalate directly to SN_LV4
      */
      if((IN_LDPTAB[indx].proc_category == IN_PSEUDO_CRITICAL) ||
         (IN_LDPTAB[indx].proc_category == IN_CP_CRITICAL)){
        /* Go directly to level 4 since process was already
        ** reading its data an a lower level wil not help
        */
        INsysboot(err_code, FALSE);
      } else {
        /* Non-critical process, stop restarting it */
        INdeadproc(indx,FALSE);
      }
    } else {
      INsetrstrt(SN_LV1,indx,source);
    }
    break;
  case SN_LV2:
  case SN_LV4:
    INsysboot(err_code, FALSE);
    exit(0);
    break;
  case SN_LV3:
  case SN_LV5:
    // Reboot all nodes if other side is not ACTIVE
    if(INgetaltstate() != S_ACT){
      sn_lvl = SN_LV5;
    }
    lv4_count = INlv4_count(FALSE,FALSE);
    if((lv4_count >= INsys_init_threshold && (INmax_boots == 0 || lv4_count < (INsys_init_threshold + INmax_boots))) || ((sn_lvl == SN_LV5) && (!IN_LDSTATE.issimplex))){
      INsysboot(err_code, TRUE);
    } else {
      if(INmax_boots > 0 && lv4_count >= (INsys_init_threshold + INmax_boots)){
        //CR_X733PRM(POA_CRIT, "INIT escalation", processingErrorAlarm, 
        //           applicationSubsystemFailure, "The threshold for maximum number of boots has been reached",  ";235",
        //           "REPT INIT MAXIMUM BOOT THRESHOLD %d EXCEEDED", INmax_boots );
        printf("REPT INIT MAXIMUM BOOT THRESHOLD %d EXCEEDED\n",
               INmax_boots);
      }

      INsysboot(err_code, FALSE);
    }
    break;
  case IN_MAXSNLVL:  
    /* Do a UNIX boot.
    ** We will never escalate to this point, it will
    ** only be done on direct request 
    */
    INsysboot(err_code, TRUE);
    break;
  case SN_INV:
  case SN_NOINIT:
  default:
    /* Perform no initializations	*/
    //INIT_ERROR(("Invalid sn_lvl = %d",sn_lvl));
    printf("Invalid sn_lvl = %d",sn_lvl);
    return;
  }
}


/*
** NAME:
**	INsysreset
**
** DESCRIPTION:
**	Set up a system-wide recovery
**
** INPUTS:
**	sn_lvl	- Level of system recovery action to be initiated (either
**		  SN_LV2 or SN_LV3)
**	err_code - Error code associated with this system reset
**	source	- Source of initialization request
**	indx	- index of the process requesting intialization
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
INsysreset(SN_LVL sn_lvl, GLretVal err_code, IN_SOURCE source, Short indx)
{
  if(sn_lvl == SN_LV2){
    //CR_PRM(INadjustAlarm(POA_CRIT),"REPT INIT REQUESTING RESET %s ERROR %d PROCESS %s",IN_SNLVLNM(sn_lvl),err_code,(indx == INIT_INDEX ? IN_MSGHQNM : IN_LDPTAB[indx].proctag));
    printf("REPT INIT REQUESTING RESET %s ERROR %d PROCESS %s\n",
           IN_SNLVLNM(sn_lvl),err_code,
           (indx == INIT_INDEX ? IN_MSGHQNM : IN_LDPTAB[indx].proctag));
  } else {
    //INIT_ERROR(("Invalid sn_lvl = %d",sn_lvl));
    printf("Invalid sn_lvl = %d\n",sn_lvl);
    return;
  }
	
  /* Make sure we are not dying too often, so that /etc/init does
  ** not stop to respawn us.
  */
  INwait_exit();

  if(sn_lvl == SN_LV5){
    /*
     * At this point, we will kill all the process
     * under INIT's control, remove our shared memory
     * segments, and exit.  This will effectively
     * escalate us to a SN_LV4 recovery level (without UNIX boot) 
     * when UNIX restarts INIT and INIT considers this
     * to be the "first" initialization due to the
     * fact that its shared memory does not exist:
     */
    INkillprocs();
    /* Free shared memory	*/
    INfreeres();
    exit(0);
  }
  /*
   * If we're currently in a system initialization then we need
   * to update the information for the "previous" initialization:
   */
  if ((IN_LDSTATE.initstate != IN_CUINTVL) &&
      (IN_LDSTATE.initstate != IN_NOINIT)) {
    IN_LDINFO.init_data[IN_LDINFO.ld_indx].end_time = time((time_t *)0);

    /*
     * Update INIT data load and unload indices so
     * they'll be correct for this system reset:
     */
    IN_LDINFO.ld_indx = IN_NXTINDX(IN_LDINFO.ld_indx, IN_NUMINITS);

    if (IN_LDINFO.ld_indx == IN_LDINFO.uld_indx) {
      IN_LDINFO.uld_indx = IN_NXTINDX(IN_LDINFO.uld_indx, IN_NUMINITS);
    }
  }

  /*
   * Store info. re: this system reset activity:
   */
  IN_LDINFO.init_data[IN_LDINFO.ld_indx].psn_lvl = sn_lvl;
  IN_LDINFO.init_data[IN_LDINFO.ld_indx].prun_lvl = IN_LDSTATE.run_lvl;
  IN_LDINFO.init_data[IN_LDINFO.ld_indx].str_time = time((time_t *)0);
  IN_LDINFO.init_data[IN_LDINFO.ld_indx].num_procs = INcountprocs();
  IN_LDINFO.init_data[IN_LDINFO.ld_indx].source = source;
  IN_LDINFO.init_data[IN_LDINFO.ld_indx].ecode = err_code;
  strcpy(IN_LDINFO.init_data[IN_LDINFO.ld_indx].msgh_name, (indx == INIT_INDEX ? IN_MSGHQNM : IN_LDPTAB[indx].proctag));


  IN_LDSTATE.sn_lvl = sn_lvl;
  IN_LDSTATE.sync_run_lvl = 0;
  IN_LDSTATE.initstate = INITING2;

  /*
   * Kill off any updating processes and make sure
   * that their "old" versions will be restarted.
   */
  Bool manual;
  if(source == IN_MANUAL){
    manual = TRUE;
  } else {
    manual = FALSE;
  }

#ifdef OLD_SU
  INautobkout(manual, TRUE);
#endif

  exit(0);
}

#define IN_BOOT_TIMEOUT		900000L		/* Boot timeout 15 min. */

const char*     INrebootFile = "/sn/init/reboot";


/*
** NAME:
**	INsysboot()
**
** DESCRIPTION:
**	This routine checks the current user ID of this process and, if
**	we're running as "root" (uid = 0) it causes UNIX to reboot.
**	Otherwise (uid != 0) this routine cleans up system resources and
**	exits.  Before taking either of the above actions this routine
**	outputs a message indicating the error code associated with this
**	recovery action.
**
** INPUTS:
**	err_code - Code identifying the point from which this routine is
**		   is invoked
**
** RETURNS:
**	The routine does not return!
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
**	UNIX boots, system resources are cleaned up and we start over!
*/
Void
INsysboot(GLretVal err_code, Bool bAllNodes, Bool printPRM)
{
  if(IN_procdata == (IN_PROCDATA *) -1){
    //CR_PRM(POA_CRIT, "REPT INIT REQUESTING NODE REBOOT ERROR CODE %d", err_code);
    printf("REPT INIT REQUESTING NODE REBOOT ERROR CODE %d\n", err_code);
    reboot(RB_AUTOBOOT);
  }
  //INIT_DEBUG(IN_DEBUG,(POA_INF,"INsysboot() entered w/err_code %d:\n\tCALLING FOR UNIX BOOT", err_code));
  printf("INsysboot() entered w/err_code %d:\n\tCALLING FOR UNIX BOOT\n",
         err_code);

  MHenvType	env;
  INevent.getEnvType(env);
  // Do not reboot all nodes in cluster CSN only in non clustered environments
  if(bAllNodes && env == MH_standard){
    if(printPRM){
      //CR_X733PRM(POA_TCR, "RESTART", qualityOfServiceAlarm,
      //           unspecifiedReason, NULL, ";205", "REPT INIT REQUESTING INITIALIZATION OF ALL NODES ERROR %d", err_code);
      printf("REPT INIT REQUESTING INITIALIZATION OF ALL NODES ERROR %d\n",
             err_code);
    }
    INinitialize	initmsg;
    initmsg.sn_lvl = SN_LV5;
    INevent.sendToAllHosts("INIT", (char*)&initmsg, sizeof(initmsg), 0L, FALSE);
    IN_SLEEP(1);
  } else if(IN_LDCURSTATE == S_LEADACT && (!IN_LDSTATE.issimplex)){
    if(printPRM){
      //CR_PRM(POA_CRIT,"REPT INIT REQUESTING SWITCHOVER ERROR CODE %d",err_code);
      printf("REPT INIT REQUESTING SWITCHOVER ERROR CODE %d\n",
             err_code);
    }
  } else {
    if(printPRM){
      //CR_X733PRM(POA_TCR, "RESTART", qualityOfServiceAlarm,
      //           unspecifiedReason, NULL, ";206", "REPT INIT REQUESTING RESTART ERROR CODE %d",err_code);
      printf("REPT INIT REQUESTING RESTART ERROR CODE %d\n",err_code);
    }
  }
  if(env != MH_peerCluster){
    INswcc	msg;
    msg.srcQue = INmsgqid;
    char mateName[MHmaxNameLen+1];
    INevent.send("MHRPROC", (char*)&msg, sizeof(INswcc), 0L);
    INgetMateCC(mateName);
    strcat(mateName, ":MSGH");
    INevent.send(mateName, (char*)&msg, sizeof(INswcc), 0L, FALSE);
    INevent.send("MSGH", (char*)&msg, sizeof(INswcc), 0L);
  }

  INsanset(IN_BOOT_TIMEOUT);

  //FTbladeStChgMsg msg(S_INIT, INITING, IN_LDSTATE.softchk);
  //if(INvhostMate >= 0){
  //  if(INevent.isVhostActive()){
  //    bAllNodes = TRUE;
  //  }
  //  msg.setVhostMate(IN_procdata->vhost[INvhostMate]);
  //  msg.setVhostState(INstandby);
  //}
  //msg.send();

  IN_SLEEPN(0,100000000);
  INkillprocs();
  INswitchVhost();
  INfreeres();
  INsanityPeg = 0xffffffff;
  sync();
  IN_LDCURSTATE = S_INIT;
  if(access(INrebootFile, X_OK) == 0){
    //CR_PRM(POA_INF, "REPT INIT EXECTUING %s", INrebootFile);
    printf("REPT INIT EXECTUING %s\n", INrebootFile);
    if(execl(INrebootFile, "reboot", (char*)0) == -1){
      //CR_PRM(POA_INF, "REPT INIT EXECL OF %s FAILED, errno %d", INrebootFile, errno);
      printf("REPT INIT EXECL OF %s FAILED, errno %d\n", INrebootFile, errno);
    }
    exit(0);
  }
  // do not boot every time only if threshold exceeded
  // or if running with RCC
  if(!bAllNodes && INissimplex){
    exit(0);
  }
  IN_SLEEP(1);
  sync();
  reboot(RB_AUTOBOOT);
  exit(0);
}

/*
** NAME:
**	INcheck_progress()
**
** DESCRIPTION:
**	This routine checks processes to insure initialization progress
**
** INPUTS:
**
** RETURNS:
**
** CALLS:
**	INreqinit()
**
** CALLED BY:
**
** SIDE EFFECTS:
*/

Void
INcheck_progress()
{
  Short 	i;

  if(INworkflg == FALSE) return;

  //INIT_DEBUG((IN_DEBUG | IN_SSEQTR | IN_RSTRTR),(POA_INF,"INcheck_progress(): entered"));
  printf("INcheck_progress(): entered\n");

  for(i = 0; i < IN_SNPRCMX; i++){
    if(IN_INVPROC(i) ||
       (IN_SDPTAB[i].procstep == IN_STEADY) ||
       (IN_SDPTAB[i].procstate == IN_DEAD) || 
       (IN_SDPTAB[i].procstate == IN_HALTED) ||
       (IN_SDPTAB[i].procstep <= IN_ESU) ||
       (IN_LDPTAB[i].proc_category != IN_CP_CRITICAL)){
      continue;
    }
    /* If the proces is not steady or dead than it must be
    ** initializing.  Make sure it is making progress
    */
		
    if(++IN_SDPTAB[i].progress_check >= IN_PROGRESS_CHECK_MAX){
      /* No progress made, initialize this process */
      INreqinit(SN_LV0,i,INPROGRESSFAIL,IN_SOFT,"NO FORWARD PROGRESS");
    }
  }
}

/*
** NAME:
**	INwait_exit()
**
** DESCRIPTION:
**	This routine insures that init does not die to fast.
**	If a process dies too quickly, /etc/init will not respawn it.
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
INwait_exit()
{
  /* Disable signals */
  (Void) signal(SIGCLD,SIG_IGN);
  long time_diff = (time((time_t)0) - INinit_start_time);
  if( time_diff < 20){
    IN_SLEEP(20 - (unsigned int)time_diff);
  }
}

/*
** NAME:
**	INic_crit_up()
**
** DESCRIPTION:
**	This routine checks if all initialization critical processes
**	are up.
**
** INPUTS:
**
** RETURNS:
**	TRUE	- if all IC processes are up
**	FALSE	- otherwise
**
** CALLS:
**
** CALLED BY:
**	INescalate();
*/
Bool
INic_crit_up(Short indx)
{
  //INIT_DEBUG((IN_DEBUG|IN_IREQTR),(POA_INF,"Entering INic_crit_up()"));
  printf("Entering INic_crit_up()\n");

  for(Short i = 0; i < IN_SNPRCMX; i++){
    if(IN_VALIDPROC(i) && i != indx &&
       IN_LDPTAB[i].proc_category == IN_INIT_CRITICAL &&
       IN_SDPTAB[i].procstep != IN_STEADY){
      //INIT_DEBUG((IN_DEBUG|IN_IREQTR),(POA_INF,"Exiting INic_crit_up() w/FALSE, proc %s",IN_LDPTAB[i].proctag));
      printf("Exiting INic_crit_up() w/FALSE, proc %s\n",IN_LDPTAB[i].proctag);
      return(FALSE);
    }
  }

  //INIT_DEBUG((IN_DEBUG|IN_IREQTR),(POA_INF,"Exiting INic_crit_up() w/TRUE"));
  printf("Exiting INic_crit_up() w/TRUE\n");
  return(TRUE);
}

/*
** NAME:
**	INcheck_sanity()
**
** DESCRIPTION:
**	This routine checks processes to insure sanity 
**
** INPUTS:
**
** RETURNS:
**
** CALLS:
**	INreqinit()
**
** CALLED BY:
**
** SIDE EFFECTS:
*/
Void
INcheck_sanity()
{
  register IN_PROCESS_DATA *ldp,*eldp;
  register IN_PTAB *sdp;

  //INIT_DEBUG((IN_DEBUG | IN_TIMER),(POA_INF,"INcheck_sanity(): entered"));
  printf("INcheck_sanity(): entered\n");
  ldp = IN_LDPTAB;
  eldp = &IN_LDPTAB[IN_SNPRCMX];
  sdp = IN_SDPTAB;

  for(; ldp < eldp; ldp++,sdp++){
    /* Only check sanity if process is STEADY or waiting for SU */
    if((sdp->procstep != IN_STEADY && sdp->procstep != IN_BSU)  ||
       ldp->peg_intvl == 0){
      continue;
    }
    if(ldp->last_count != sdp->count){
      /* Sanity has been pegged */
      ldp->last_count = sdp->count;
      ldp->sent_missedsan = FALSE;
      ldp->time_missedsan = 0;
    } else {
      /* Sanity not pegged, increment time_missedsan */
      ldp->time_missedsan += INAUDTMR;
      if(ldp->peg_intvl <= ldp->time_missedsan){
        ldp->time_missedsan = 0;
        if(ldp->sent_missedsan == FALSE){
          /*
          ** The process has missed its sanity
          ** peg for the first time -- change
          ** the sent_missedsan state and attempt
          ** to send the process a message:
          */
          ldp->sent_missedsan = TRUE;
          if (INetype != EHTMRONLY) {
            class INlmissedSan msan;

            GLretVal retval;
            char	name[MHmaxNameLen+1];
            INgetRealName(name, ldp->proctag);
            retval = msan.send(name, INmsgqid, 0);
            /*
             * If the process' queue is
             * full (or the entire system
             * is out of msg queue space)
             * we still leave sent_missedsan TRUE. 
             * If the process is busy handling
             * messages, then it should be pegging
             * its sanity as well. Otherwize the
             * assumption is that the queue is full
             * because the process is insane.
             */
            if (retval == MHagain) {
              //INIT_DEBUG((IN_DEBUG | IN_TIMER),(POA_INF,"INcheck_sanity(): failed to send \"missed sanity\" message to \"%s\", procstate %s\n\t\"MHagain\" returned from MSGH send",ldp->proctag,IN_PROCSTNM(sdp->procstate) ));
              printf("INcheck_sanity(): failed to send \"missed sanity\" message to \"%s\", "
                     "procstate %s\n\t\"MHagain\" returned from MSGH send\n",
                     ldp->proctag,IN_PROCSTNM(sdp->procstate));

              continue;
            }

            //INIT_DEBUG((IN_DEBUG | IN_TIMER | IN_SANITR | IN_RSTRTR),(POA_INF,"INcheck_sanity(): On first SANITY timeout\n\tproc \"%s\", procstate %s\n\t\"INmissedSan\" msg send returned %d",ldp->proctag,IN_PROCSTNM(sdp->procstate), retval));
            printf("INcheck_sanity(): On first SANITY timeout\n\tproc \"%s\", "
                   "procstate %s\n\t\"INmissedSan\" msg send returned %d",
                   ldp->proctag,IN_PROCSTNM(sdp->procstate), retval);
          }
        } else {
          /*
           * Process missed its sanity timer,
           * attempt to cycle the process
           * through a process re-initialization
           */
          U_short indx = (U_short)(ldp - &IN_LDPTAB[0]);
          INreqinit(SN_LV0,indx,IN_SANTMR_EXPIRED,IN_SOFT,"SANITY TIMER EXPIRED");
        }
      }
    }
  }
}

/*
** NAME:
**	INvmem_check()
**
** DESCRIPTION:
**	This routine checks currently available virtual memory
**	and reports any alarm conditions if necessary.
**	It also notifies SYSTAT if any alarm conditions exist.
**	If an alarm condition exists, a cyclic timer is set to call this
**	function periodically (every 30 min for critical alarm and
**	every 60 for all the other alarms) to repeat unalarmed OM
**	indicating current (alarmed) state of virtual memory.
**
** INPUTS:
**	ucl  - TRUE update SYSTAT with current value of alarms and generate OM.
**	       FALSE, only report on changes in condition.
**	periodic - If an abnormal condition exist, print periodic unalarmed OM reporting it
**
** RETURNS:
**
** CALLS:
**
** CALLED BY:
**	INaudit(), INmain(), INtimerprc(), INrcvmsg()
**
** SIDE EFFECTS:
**	IN_LDVMEM - current value of available virtual memory in
**		    INIT shared memory gets updated.
*/

Void
INvmem_check(Bool ucl, Bool periodic){

  CRALARMLVL	clvl;		/* Current value of vmem alarm		*/
  CRALARMLVL	prt_alvl;	/* Print value of vmem alarm		*/
  long	mb_free;		/* Free memory in Mbytes		*/
  const char * alvl_str;		/* Alarm level string			*/
  struct sysinfo si;

  if(INvmem_critical == 0){
    return;
  }

  // Get anoninfo from the kernel
  if(INroot == TRUE){
    if(sysinfo(&si) < 0){
      //CR_X733PRM(POA_TMJ,"SWAP SIZE", processingErrorAlarm, softwareError, NULL, ";218", "REPT INIT ERROR FAILED TO GET SWAP SIZE ERRNO %d", errno);
      printf("REPT INIT ERROR FAILED TO GET SWAP SIZE ERRNO %d\n", errno);
      return;
    }
  } else {
    return;
  }


  int avail_swap = si.freeswap;
  if(avail_swap < 0) {
    avail_swap = 0;
  }
  IN_LDVMEM = (avail_swap / 1024) * si.mem_unit;

  mb_free = IN_LDVMEM / 1024;

  /* Compute current alarm condition */
  if(IN_LDVMEM <= INvmem_critical){
    clvl = POA_CRIT;
    alvl_str = "CRITICAL";
  } else if(IN_LDVMEM <= INvmem_major){
    clvl = POA_MAJ;
    alvl_str = "MAJOR";
  } else if(IN_LDVMEM <= INvmem_minor){
    clvl = POA_MIN;
    alvl_str = "MINOR";
  } else {
    clvl = POA_INF;
    alvl_str = "NORMAL";
  }

  if(ucl == TRUE || IN_LDVMEMALVL != clvl){
    /* Update the INvmem indicator */
    //CRindStatusMsg	statmsg;
		//
    //statmsg.send("INvmem",alvl_str);
		
  }
	
  if(IN_LDVMEMALVL != clvl){
    prt_alvl = clvl;
    /* If alarmed condition exists, set timer for outputting periodic OM */
    if(clvl == POA_CRIT){
      INSETTMR(INvmemtmr,INVMEMINT,INVMEMTAG,TRUE);
    } else if(clvl != POA_INF){
      INSETTMR(INvmemtmr,2*INVMEMINT,INVMEMTAG,TRUE);
    } else {	/* clvl == POA_INF, no alarm clear timer */
      INCLRTMR(INvmemtmr);
    }
  } else {	/* No change in alarms since last check */
    if(periodic == TRUE){	
      /* Print an unalarmed periodic OM	*/
      prt_alvl = POA_INF;
    } else {
      return;
    }
  }

  switch(clvl){
  case POA_CRIT:
  case POA_MAJ:
  case POA_MIN:
    if(prt_alvl != POA_INF){
      //CR_X733PRM(prt_alvl, "VMEM", qualityOfServiceAlarm, resourceAtOrNearingCapacity, 
      //           NULL, ";200", "REPT VMEM %s THRESHOLD EXCEEDED %ld MB FREE",alvl_str,mb_free);
      printf("REPT VMEM %s THRESHOLD EXCEEDED %ld MB FREE\n",
             alvl_str,mb_free);
    } else {
      //CR_PRM(POA_INF,"REPT VMEM %s THRESHOLD EXCEEDED %ld MB FREE",alvl_str,mb_free);
      printf("REPT VMEM %s THRESHOLD EXCEEDED %ld MB FREE\n",
             alvl_str,mb_free);
    }
    break;
  case POA_INF:
    switch(IN_LDVMEMALVL){
    case POA_CRIT:
      //CR_X733PRM(POA_CLEAR, "VMEM", qualityOfServiceAlarm, 
      //           resourceAtOrNearingCapacity, NULL, ";200", 
      //           "REPT VMEM CRITICAL THRESHOLD NO LONGER EXCEEDED %ld MB FREE", mb_free);
      printf("REPT VMEM CRITICAL THRESHOLD NO LONGER EXCEEDED %ld MB FREE\n",
             mb_free);
      break;
    case POA_MAJ:
      //CR_X733PRM(POA_CLEAR, "VMEM", qualityOfServiceAlarm, 
      //           resourceAtOrNearingCapacity, NULL, ";200", 
      //           "REPT VMEM MAJOR THRESHOLD NO LONGER EXCEEDED %ld MB FREE", mb_free);
      printf("REPT VMEM MAJOR THRESHOLD NO LONGER EXCEEDED %ld MB FREE\n",
             mb_free);
      break;
    case POA_MIN:
      //CR_X733PRM(POA_CLEAR, "VMEM", qualityOfServiceAlarm, 
      //           resourceAtOrNearingCapacity, NULL, ";200", 
      //           "REPT VMEM MAJOR THRESHOLD NO LONGER EXCEEDED %ld MB FREE", mb_free);
      printf("REPT VMEM MAJOR THRESHOLD NO LONGER EXCEEDED %ld MB FREE\n",
             mb_free);
      break;
    }
    break;
  }
	
  IN_LDVMEMALVL = clvl;
}

Void
INswitchVhost()
{
  GLretVal	ret;
  if(INvhostMate < 0){
    return;
  }
  if(INevent.isVhostActive()){
    INvhostInitialize vhostInitMsg(IN_procdata->vhost[(INvhostMate + 1)&0x1]);
    INvhostMate = -1;
    ret = vhostInitMsg.send(INvhostMateName, INmsgqid, sizeof(vhostInitMsg), 0, FALSE);
    if(ret != GLsuccess){
      //CR_PRM(POA_INF, "REPT INIT FAILED TO SEND VHOST INIT RET %d", ret);
      printf("REPT INIT FAILED TO SEND VHOST INIT RET %d\n",
             ret);
    }
  }
}
