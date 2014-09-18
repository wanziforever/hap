/*
**	File ID: 	@(#): <MID8535 () - 08/17/02, 29.1.1.1>
**
**	File:			MID8535
**	Release:		29.1.1.1
**	Date:			08/21/02
**	Time:			19:37:19
**	Newest applied delta:	08/17/02 04:34:58
**
** DESCRIPTION:
**	This file contains the INIT's timer processing routine
**
** FUNCTIONS:
**	INtimerproc()	- Process expired INIT timers
**	INarumsg()	- Send message to ALARM to force pegging of ARU.
**
** NOTES:
*/
#include <sysent.h>	/* exit(), etc. */
#include <signal.h>	/* SIGKILL */

#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "cc/hdr/init/INinit.hh"
#include "cc/hdr/init/INproctab.hh"
#include "cc/init/proc/INmsgs.hh"
#include "cc/hdr/init/INinitialize.hh"
#include "cc/init/proc/INlocal.hh"
#include "cc/hdr/init/INdr.hh"
//#include "cc/hdr/ft/FTreturns.hh"
//#include "cc/hdr/ft/FTdr.hh"
//#include "cc/hdr/ft/FThwm.hh"
//#include "cc/hdr/ft/FTbladeMsg.hh"

int	INnoOamLeadMsg = 0;
#define INmaxNoLeadMsg	2


/*
** NAME:
**	INtimerproc()
**
** DESCRIPTION:
**	This routine handles INIT timers when they expire.
**
** INPUTS:
**	tag	- Tag of expired timer
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
INtimerproc(U_short tag)
{

	//INIT_DEBUG(IN_DEBUG,(POA_INF,"INtimerproc():\n\tentered with tag %x", tag));
  printf("INtimerproc():\n\tentered with tag %x\n", tag);

	switch(tag & INTYPEMASK) {  
	case INGENTAG:
		switch(tag){
		case INARUTAG:

      //INIT_DEBUG((IN_DEBUG | IN_TIMER | IN_AUDTR),(POA_INF,"INtimerproc(): ARU timer expired"));
      printf("INtimerproc(): ARU timer expired\n");
			INarumsg();
			return;

    case INSOFTCHKTAG:
      //INIT_DEBUG((IN_DEBUG | IN_TIMER | IN_AUDTR),(POA_INF,"INtimerproc(): Softchk inhibit timer expired"));
      printf("INtimerproc(): Softchk inhibit timer expired\n");
      //CR_X733PRM(POA_MAJ, "SYSTEM SOFTCHK", qualityOfServiceAlarm, 
      //           unspecifiedReason, NULL, ";202", "REPT INIT SYSTEM SOFTWARE CHECKS INHIBITED");
      printf("REPT INIT SYSTEM SOFTWARE CHECKS INHIBITED\n");
			IN_LDALMSOFTCHK = POA_MAJ;
      return;

		case INCHECKLEADTAG:
			INchecklead();
			return;

		case INSETACTIVEVHOSTTAG:
       {
         if(INevent.getActiveVhost() < 0){
           char mateName[(MHmaxNameLen + 1) * 2];
           sprintf(mateName, "%s:INIT", IN_procdata->vhost[INvhostMate]);
           INsetActiveVhost  setActiveMsg;
           // send unbuffered
           setActiveMsg.send(mateName, INmsgqid, sizeof(setActiveMsg), 0, FALSE);
           INvhostInitialize 	vhostInit(IN_procdata->vhost[INvhostMate]);
           vhostInit.send(INmsgqid, MHnullQ, sizeof(vhostInit), 0);
           INCLRTMR(INsetActiveVhostTmr);
         } else {
           // Failed heartbeat, reset the mate node
           extern int INinitmissed;

           if(INinitmissed < (IN_procdata->vhostfailover_time * 2)){
             //CR_PRM(POA_MAJ, "REPT INIT ACTIVE VHOST %s FAILED HEARTBEAT, RESETTING",IN_procdata->vhost[INvhostMate]);
             printf("REPT INIT ACTIVE VHOST %s FAILED HEARTBEAT, RESETTING\n",
                    IN_procdata->vhost[INvhostMate]);
           } else {
             //CR_PRM(POA_MAJ, "REPT INIT ACTIVE VHOST %s FAILED HEARTBEAT BUT MAIN THREAD HUNG - IGNORING",IN_procdata->vhost[INvhostMate]);
             printf("REPT INIT ACTIVE VHOST %s FAILED HEARTBEAT BUT MAIN THREAD HUNG - IGNORING\n",
                    IN_procdata->vhost[INvhostMate]);
             return;
           }
           //FThwmMachineReset(IN_procdata->vhost[INvhostMate]);
           int i;
           for(i = 0; i < 2; i++){
             IN_SLEEP(1);
             INsanityPeg++;
           }
           INvhostInitialize 	vhostInit(IN_procdata->vhost[INvhostMate]);
           vhostInit.send(INmsgqid, MHnullQ, sizeof(vhostInit), 0);
         }
         return;
       }
		case INVHOSTREADYTAG:
			INescalate(SN_LV5,IN_ACTIVEREADYTIMEOUT,IN_SOFT,INIT_INDEX);
			return;
		case INVMEMTAG:
			//INIT_DEBUG((IN_DEBUG | IN_TIMER),(POA_INF,"INtimerproc(): VMEM timer expired"));
      printf("INtimerproc(): VMEM timer expired\n");
			/* Generate unalarmed message and send an update to SYSTAT */
			INvmem_check(TRUE,TRUE);
			return;
		case INSANITYTAG:
			INsanityPeg++;
			return;
		case INSETLEADTAG:
       {
         INsetLead	setLead;
         setLead.srcQue = INmsgqid;
         char mateName[MHmaxNameLen+1];
         char initName[MHmaxNameLen+1];
         INgetMateCC(mateName);
         sprintf(initName, "%s:INIT", mateName);
         /* send this message unbuffered 	*/
         INevent.send(initName, (char*)&setLead, sizeof(setLead), 0L, 0);

         if(IN_LDCURSTATE == S_ACT){
           // Go Lead
           INrm_check_state(S_LEADACT);
         } 
         return;
       }
		case INOAMLEADTAG:
       {
         INnoOamLeadMsg ++;
         if(INcanBeOamLead && INnoOamLeadMsg > INmaxNoLeadMsg){
           INevent.setOamLead(INmsgqid);
           INoamInitialize  oamInit;
           INevent.broadcast(INmsgqid, (char*)&oamInit, sizeof(oamInit));
           //CR_PRM(POA_INF, "REPT INIT TRANSITIONING OAM LEAD TO %s", INmyPeerHostName);
           printf("REPT INIT TRANSITIONING OAM LEAD TO %s\n",
                  INmyPeerHostName);
         }

         if(INevent.getOAMLead() == INmyPeerHostId){
           int		i;
           char		initName[2*MHmaxNameLen + 1];
           INoamLead	oamLeadMsg;
           INnoOamLeadMsg = 0;
				
           // Send INoamLeadTyp message to everyone in the OA&M cluster
           for(i = 0; i < INmaxResourceGroups; i++){
             if(IN_procdata->oam_lead[i][0] == 0){
               break;
             }
             if(strcmp(IN_procdata->oam_lead[i], INmyPeerHostName) == 0){
               continue;
             }
             sprintf(initName,"%s:INIT", IN_procdata->oam_lead[i]);
             oamLeadMsg.send(initName, INmsgqid, sizeof(INoamLead), 0, FALSE);
           }

           MHmsgh.sendToAllHosts("INIT", (char*)&oamLeadMsg, sizeof(INoamLead), MH_scopeSystemOther);
         }
         return;
       }
		case INOAMREADYTAG:
       {
         INescalate(SN_LV5,IN_OAMREADYTIMEOUT,IN_SOFT,INIT_INDEX);
         break;
       }
		default:
			//INIT_ERROR(("Unknown timer type, tag %d",tag));
      printf("Unknown timer type, tag %d\n",tag);
			return;
		}
	case INAUDTAG:
     {
       //INIT_DEBUG((IN_DEBUG | IN_TIMER | IN_AUDTR),(POA_INF,"INtimerproc(): AUD timer expired"));
       printf("INtimerproc(): AUD timer expired\n");
       (Void) INaudit(FALSE);

       return;
     }

	case INITTAG:
     {
       U_short tagtype = tag & INPTAGMASK;

       switch(tagtype){
       case INSEQTAG:
          {
            //INIT_DEBUG((IN_DEBUG | IN_TIMER),(POA_INF,"INtimerproc(): INIT timer expired"));
            printf("INtimerproc(): INIT timer expired\n");
            switch(IN_LDSTATE.initstate) {
            case IN_NOINIT:
            case INV_INIT:
              //INIT_ERROR(("INIT timer expired for invalid initstate %s",IN_STATENM(IN_LDSTATE.initstate)));
              printf("INIT timer expired for invalid initstate %s\n",
                     IN_STATENM(IN_LDSTATE.initstate));
              INCLRTMR(INinittmr);
              return;

            case IN_CUINTVL:
               {
                 //CR_PRM(POA_INF,("REPT INIT INITIALIZATION INTERVAL COMPLETE"));
                 printf("REPT INIT INITIALIZATION INTERVAL COMPLETE\n");
                 IN_LDSTATE.sn_lvl = SN_NOINIT;
                 IN_LDSTATE.initstate = IN_NOINIT;
                 INCLRTMR(INinittmr);
                 //FTbladeStChgMsg msg(S_LEADACT, IN_NOINIT, IN_LDSTATE.softchk); 
                 //if(INvhostMate >= 0){
                 //  msg.setVhostMate(IN_procdata->vhost[INvhostMate]);
                 //  if(INevent.isVhostActive()){
                 //    msg.setVhostState(INactive);
                 //  } else {
                 //    msg.setVhostState(INstandby);
                 //  }
                 //}
                 //msg.send();

                 /* Clear level 4 count */
                 (void) INlv4_count(TRUE);
                 return;
               }
            case INITING:
              //INIT_ERROR(("INIT timer expired w/initstate INITING systep %s",IN_SQSTEPNM(IN_LDSTATE.systep)));
              printf("INIT timer expired w/initstate INITING systep %s\n",
                     IN_SQSTEPNM(IN_LDSTATE.systep));
              return;
	
            case INITING2:
            case IN_MAXINIT:
            default:
              //INIT_ERROR(("INIT timer expired in unknown INITSTATE %d",IN_LDSTATE.initstate));
              printf("INIT timer expired in unknown INITSTATE %d\n",
                     IN_LDSTATE.initstate);
            }
            return;
          }
       }
     }

	case INPROCTAG:
     {
       U_short indx = tag & INPINDXMASK;
       U_short tagtype = tag & INPTAGMASK;

       switch(tagtype) {
       case INSYNCTAG:
         //INIT_DEBUG((IN_DEBUG | IN_TIMER),(POA_INF,"INtimerproc(): sync timer expired for \"%s\", indx %d", IN_LDPTAB[indx].proctag,indx));
         printf("INtimerproc(): sync timer expired for \"%s\", indx %d\n",
                IN_LDPTAB[indx].proctag,indx);

         if(IN_INVPROC(indx)){
           //INIT_ERROR(("Invalid process entry %d",indx));
           printf("Invalid process entry %d\n",indx);
           INCLRTMR(INproctmr[indx].sync_tmr);
           return;
         }

         switch(IN_SDPTAB[indx].procstate) {
         case IN_INVSTATE:
           //INIT_ERROR(("Sync timer expired for \"%s\" in invalid procstate %s, tag %x",IN_LDPTAB[indx].proctag,IN_PROCSTNM(IN_SDPTAB[indx].procstate), tag));
           printf("Sync timer expired for \"%s\" in invalid procstate %s, tag %x\n",
                  IN_LDPTAB[indx].proctag,IN_PROCSTNM(IN_SDPTAB[indx].procstate), tag);
           INCLRTMR(INproctmr[indx].sync_tmr);
           break;
         case IN_DEAD:
           // Check if process was to be restarted
           if(IN_LDPTAB[indx].next_rstrt == IN_NO_RESTART){
             //INIT_ERROR(("Sync timer expired for \"%s\" next_rstrt = %d, tag %x",IN_LDPTAB[indx].proctag,IN_LDPTAB[indx].next_rstrt, tag));
             printf("Sync timer expired for \"%s\" next_rstrt = %d, tag %x\n",
                    IN_LDPTAB[indx].proctag,IN_LDPTAB[indx].next_rstrt, tag);
             return;
           }

           GLretVal startret;
           INCLRTMR(INproctmr[indx].sync_tmr);
           // If the process was non-critical, start it at
           // level 4 (level 5 if on lead), otherewise do level 1
           // System level is required for non critical processes
           // because their shared memory is released
           if(IN_LDPTAB[indx].proc_category == IN_NON_CRITICAL){
             if(INevent.onLeadCC()){
               startret = INsetrstrt(SN_LV5,indx,IN_SOFT);
             } else {
               startret = INsetrstrt(SN_LV4,indx,IN_SOFT);
             }
           } else {
             startret = INsetrstrt(SN_LV1,indx,IN_SOFT);
           }
           if(startret != GLsuccess){
             INdeadproc(indx,FALSE);
           }
           return;

         case IN_CREATING:
         case IN_HALTED:
         case IN_RUNNING:
         case IN_NOEXIST:
           // Clear the timer
           INCLRTMR(INproctmr[indx].sync_tmr);

           switch(IN_SDPTAB[indx].procstep){
           case IN_BCLEANUP:
           case IN_CLEANUP:
           case IN_ECLEANUP:
             //CR_PRM(POA_INF,"REPT INIT %s TIMED OUT ON CLEANUP",IN_LDPTAB[indx].proctag);
             printf("REPT INIT %s TIMED OUT ON CLEANUP\n",
                    IN_LDPTAB[indx].proctag);
             // kill the process
             if(IN_LDPTAB[indx].pid > 1){
               (Void)INkill(indx,SIGKILL);
               int i;
               for(i = 0; i < 3; i++){
                 IN_SLEEP(2);
                 /* Wait on zombie children */
                 while(INcheck_zombie() > 0);
                 if(INkill(indx,0) != 0){
                   /* Process died */
                   break;
                 }
               }

               if(INkill(indx,0) == 0){
                 CRALARMLVL alarmlvl;
                 if(IN_LDPTAB[indx].proc_category == IN_NON_CRITICAL){
                   alarmlvl = POA_MAJ;
                 } else {
                   alarmlvl = POA_CRIT;
                 }
                 //CR_PRM(alarmlvl,"REPT INIT %s COULD NOT BE KILLED",IN_LDPTAB[indx].proctag);
                 printf("REPT INIT %s COULD NOT BE KILLED\n",
                        IN_LDPTAB[indx].proctag);
                 // Even though a process did not die
                 // we will treat it as dead 
                 // This will cause a new copy to be
                 // created with unknown effect on
                 // the system.  Should we do a 
                 // UNIX boot here?
                 INdeath(indx);
               }
             }
             break;
#ifdef OLD_SU
           case IN_SU:
             /* Timed out either in apply or backout.
             ** If it was apply, backout the SU.
             ** If it was backout, process failed to die,
             ** after we tried to kill it. Just act as if
             ** it died, so we can proceed to start the
             ** old copy.
             */
             if(IN_LDPTAB[indx].updstate == UPD_PRESTART){
               //CR_PRM(POA_INF,"REPT INIT ERROR %s FAILED TO EXIT FOR SOFTWARE UPDATE",IN_LDPTAB[indx].proctag);
               printf("REPT INIT ERROR %s FAILED TO EXIT FOR SOFTWARE UPDATE\n",
                      IN_LDPTAB[indx].proctag);
               INautobkout(FALSE,FALSE);
             } else if(IN_LDPTAB[indx].updstate == UPD_POSTSTART){
               //CR_PRM(POA_INF,"REPT INIT ERROR %s DID NOT DIE DURING SU BACKOUT",IN_LDPTAB[indx].proctag);
               printf("REPT INIT ERROR %s DID NOT DIE DURING SU BACKOUT\n",
                      IN_LDPTAB[indx].proctag);
               INdeath(indx);
             } else {
               //INIT_ERROR(("SU sync timer expired for \"%s\" not in SU step %s, tag %x",IN_LDPTAB[indx].proctag,IN_SQSTEPNM(IN_SDPTAB[indx].procstep), tag));
               printf("SU sync timer expired for \"%s\" not in SU step %s, tag %x\n",
                      IN_LDPTAB[indx].proctag,IN_SQSTEPNM(IN_SDPTAB[indx].procstep), tag);
             }
             break;
           case IN_BSU:
           case IN_ESU:
#endif
           case IN_MAXSTEP:
           case IN_STEADY:
             //INIT_ERROR(("Sync timer expired for \"%s\" in invalid procstep %s, tag %x",IN_LDPTAB[indx].proctag,IN_SQSTEPNM(IN_SDPTAB[indx].procstep), tag));
             printf("Sync timer expired for \"%s\" in invalid procstep %s, tag %x\n",
                    IN_LDPTAB[indx].proctag,IN_SQSTEPNM(IN_SDPTAB[indx].procstep), tag);
             break;
           case IN_BPROCINIT:
           case IN_PROCINIT:
           case IN_EPROCINIT:
             INreqinit(SN_LV0,indx,IN_SYNCTMR_EXPIRED,IN_SOFT,"PROCINIT TIMER EXPIRED");
             break;
           default:
             // We don't care what process step failed
             // Request process initialization
             INreqinit(SN_LV0,indx,IN_SYNCTMR_EXPIRED,IN_SOFT,"SYNC TIMER EXPIRED");
           }

           return;

         case IN_MAXSTATE:
         default:
           //INIT_ERROR(("Sync timer expired for \"%s\" in unknown procstate %d, tag %x",IN_LDPTAB[indx].proctag,IN_SDPTAB[indx].procstate, tag));
           printf("Sync timer expired for \"%s\" in unknown procstate %d, tag %x\n",
                  IN_LDPTAB[indx].proctag,IN_SDPTAB[indx].procstate, tag);
         }
         return;

       case INRSTRTAG:
         //INIT_DEBUG((IN_DEBUG | IN_TIMER | IN_RSTRTR),(POA_INF,"INtimerproc(): restart timer expired for \"%s\", indx %d", IN_LDPTAB[indx].proctag, indx));
         printf("INtimerproc(): restart timer expired for \"%s\", indx %d\n",
                IN_LDPTAB[indx].proctag, indx);
         if(IN_LDPTAB[indx].rstrt_cnt == 0){
           //INIT_ERROR(("Restart timer expired for %s with restart count 0",IN_LDPTAB[indx].proctag));
           printf("Restart timer expired for %s with restart count 0\n",
                  IN_LDPTAB[indx].proctag);
         }
         IN_LDPTAB[indx].rstrt_cnt = 0;
         IN_LDPTAB[indx].next_rstrt = 0;
         IN_LDPTAB[indx].failed_init = FALSE;
         IN_LDPTAB[indx].sn_lvl = SN_NOINIT;
         INCLRTMR(INproctmr[indx].rstrt_tmr);
         return;

       case INGQTAG:
         // Global queue transition timer expired
         // Init the process that failed.
         INCLRTMR(INproctmr[indx].gq_tmr);
         IN_LDPTAB[indx].gqsync = IN_MAXSTEP;
         INreqinit(SN_LV0,indx,IN_GQTIMEOUT,IN_SOFT,"GLOBAL QUEUE TIMEOUT");
         return;
       default:
         //INIT_ERROR(("Unknown proc timer tag %x",tag));
         printf("Unknown proc timer tag %x\n",tag);

       }
       return;
     }

	default:
		//INIT_ERROR(("Unknown timer type, tag %d",tag));
    printf("Unknown timer type, tag %d\n",tag);
	}

	//INIT_DEBUG(IN_TIMER,(POA_INF,"INtimerproc():returning", tag));
  printf("INtimerproc():returning\n", tag);
}

mutex_t	INtmrLock = DEFAULTMUTEX;

/*
** NAME:
**	INsettmr()
**
** DESCRIPTION:
**	This routing handles timer setting
**
** INPUTS:
**	tmr 	- Init timer data
**	duration - timer duration
**	tag	- Tag of expired timer
**	c_flag	- TRUE if circular timer
**	hz_flag - TRUE if timer in HZ
**
** RETURNS:
**
** CALLS:
**	INescalate() - if failed to get a timer
**	INevent.setRtmr - to set a timer
**
** CALLED BY:
**
** SIDE EFFECTS:
*/

Void
INsettmr(INTMR & tmr, Long duration, U_short tag, Bool c_flag, Bool hz_flag)
{
	mutex_lock(&INtmrLock);
	(Void)INevent.clrTmr(tmr.tindx);
	/* make sure that positive duration was requested */
	if(duration <= 0){
		mutex_unlock(&INtmrLock);
		//INIT_ERROR(("Invalid timer duration %ld, tag 0x%lx",duration,tag));
    printf("Invalid timer duration %ld, tag 0x%lx\n",duration,tag);
		return;
	}
	tmr.c_flag = c_flag;
	tmr.ttag = tag;
	if((tmr.tindx = INevent.setRtmr(duration, tag, c_flag, hz_flag)) < 0){  
    INescalate(SN_LV0,tmr.tindx,IN_SOFT,INIT_INDEX);
	}
	mutex_unlock(&INtmrLock);

}

/*
** NAME:
**	INarumsg()
**
** DESCRIPTION:
**	This routine sends a message to ALARM process to 
**	update ARU.
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
INarumsg()
{
	//INIT_DEBUG((IN_DEBUG | IN_TIMER),(POA_INF,"INarumsg(): ARU timer expired, INetype = %d",INetype));
  printf("INarumsg(): ARU timer expired, INetype = %d\n",INetype);
	INsanstrobe();
	/* Send INmissedSan message to the ALARM process */
	if (INetype != EHTMRONLY) {
		class INlmissedSan msan;
		GLretVal retval;
#ifdef __linux
		msan.send("INITMON", INmsgqid, 0);
#endif

		int	alarm_index;
		if((alarm_index = INfindproc("ALARM")) >= IN_SNPRCMX){
			//INIT_DEBUG((IN_DEBUG | IN_TIMER),(POA_INF,"INarumsg(): ALARM process not equipped"));
      printf("INarumsg(): ALARM process not equipped\n");
			return;
		}
		retval = msan.send("ALARM", INmsgqid, 0);
		if(retval < 0 && IN_LDSTATE.initstate != INITING && 
       (IN_SDPTAB[alarm_index].procstep == IN_STEADY ||
        IN_SDPTAB[alarm_index].procstep == IN_BSU)){
			//CRERROR("Failed to send ARU message to ALARM due to %d",retval);
      printf("Failed to send ARU message to ALARM due to %d\n",retval);
		}
	}
}
