/*
** File ID:	@(#): <MID32112 () - 06/24/03, 23.1.1.5>
**
** File:		MID32112
** Release:		23.1.1.5
** Date:		06/26/03
** Time:		14:22:09
** Newest applied delta:06/24/03
*/

/*
** DESCRIPTION:
**	This file implements functionality that is only used on 
**	PENTIUM ACTIVE/STANDBY configuration. None of the functions
**	in this file are called in TANDEM or simplex environment.
**
** FUNCTIONS:
**	INrm_main() - 	Main function for resource monitor thread 
**	INrm_check_state() - Periodic check of next synchronization state
**	INrm_sync()	- Resource monitor synchronization function
**	INgetaltstate()	- Get the state of the other machine
**	INshutOracle() - Shutdown Oracle prior to manual switchover to limit 
**			 possible data corruption
**	
*/

/* standard unix header files */
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <sys/times.h>
#include <limits.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include "INlocal.hh"
#include "INmsgs.hh"
#include "cc/hdr/init/INinitialize.hh"
//#include "cc/hdr/ft/FTmsgs.hh"
//#include "cc/hdr/ft/FTnodeStChgMsg.hh"
#include "cc/hdr/msgh/MHgq.hh"


#define INmaxCCs	2

extern MHenvType	INenv;

Void
INchecklead()
{
	//INIT_DEBUG((IN_DEBUG | IN_TIMER),(POA_INF,"INchecklead(): Check lead timer expired"));
  printf("INchecklead(): Check lead timer expired\n");
	GLretVal	retval;

	// Check if we have access to MSGH yet, if not return;
	if(INetype != EHBOTH){
		return;
	}

	// This will go to many machines, including TSs, but 
	// if the message does not apply, it will be tossed

	INcheckLead	msg;
	msg.srcQue = INmsgqid;

	if((retval = INevent.sendToAllHosts("INIT", (char*)&msg, sizeof(msg), 0L)) <= 0){
    //INIT_DEBUG(IN_BSEQTR, (POA_INF, "INchecklead(), send() failed, retval=%d",retval));
    printf("INchecklead(), send() failed, retval=%d\n",retval);
	}
}


char 
INgetaltstate()
{
	char 		name[MHmaxNameLen+1];
	GLretVal 	retval;
	Short		hostid;

	INgetMateCC(name);
	if((retval = INevent.name2HostId(hostid, name)) != GLsuccess){
		//INIT_ERROR(("Failed to get hostid name %20.20s, retval = %d", name, retval));
    printf("Failed to get hostid name %20.20s, retval = %d\n", name, retval);
		return(S_UNAV);
	}

	return(IN_NODE_STATE[hostid]);
}

Void
INrm_check_state(char newstate)
{

	struct stat 	stbuf;
	char		prevState;

	if(newstate != S_OFFLINE){
		if(stat(IN_OFFLINE_FILE, &stbuf) == 0 && unlink(IN_OFFLINE_FILE) == -1){
			//CR_PRM(POA_INF, "REPT INIT ERROR FAILED TO DELETE OFFLINE FILE ERRNO %d", errno);
      printf("REPT INIT ERROR FAILED TO DELETE OFFLINE FILE ERRNO %d\n",
             errno);
		}
	}

	if(newstate == IN_LDCURSTATE && newstate != S_INIT){
		//INIT_DEBUG((IN_DEBUG | IN_SSEQTR), (POA_INF, "newstate %c == curstate %c", newstate, IN_LDCURSTATE));
    printf("newstate %c == curstate %c\n", newstate, IN_LDCURSTATE);
		return;
	}

	switch(newstate){
	case S_OFFLINE:
		// Write the offline file
     {
       int fd;

       if((fd = open(IN_OFFLINE_FILE, O_RDWR | O_CREAT, 0644)) < 0){
         //CR_PRM(POA_INF, "REPT INIT ERROR FAILED TO CREATE OFFLINE FILE ERRNO %d", errno);
         printf("REPT INIT ERROR FAILED TO CREATE OFFLINE FILE ERRNO %d\n",
                errno);
       }
       close(fd);
       INsanset(0L);
       //CR_PRM(POA_INF, "REPT INIT TRANSITIONING TO OFFLINE");
       printf("REPT INIT TRANSITIONING TO OFFLINE\n");
     }
	case S_UNAV:
		if(newstate != S_OFFLINE){
			//CR_PRM(POA_INF, "REPT INIT TRANSITIONING TO UNAVAILABLE");
      printf("REPT INIT TRANSITIONING TO UNAVAILABLE\n");
		}
	case S_INIT:
		if(newstate != S_OFFLINE && newstate != S_UNAV){
			//CR_PRM(POA_INF, "REPT INIT TRANSITIONING TO INIT");
      printf("REPT INIT TRANSITIONING TO INIT\n");
		}
		IN_LDCURSTATE = newstate;
    INkillprocs();
		INswitchVhost();
    INfreeres(FALSE);
		INinit();
		/* Act like INIT intended to die		*/
		IN_LDSTATE.sn_lvl = SN_LV4;
		IN_LDSTATE.initstate = INITING2;
		exit(0);
	case S_STBY:
		//CR_PRM(POA_INF, "REPT INIT TRANSITIONING TO STANDBY");
    printf("REPT INIT TRANSITIONING TO STANDBY\n");
		if(IN_LDCURSTATE != S_INIT && IN_LDCURSTATE != S_UNAV && IN_LDCURSTATE != S_OFFLINE){
			//INIT_ERROR(("INrm_check_state(): Invalid state transition, from %c to %c",IN_LDCURSTATE,newstate));
      printf("INrm_check_state(): Invalid state transition, from %c to %c\n",
             IN_LDCURSTATE,newstate);
		}
		IN_LDCURSTATE = S_INIT;
		IN_LDCURSTATE = newstate;
		IN_LDSTATE.isactive = FALSE;
		break;
	case S_ACT:
		//CR_PRM(POA_INF, "REPT INIT TRANSITIONING TO ACTIVE");
    printf("REPT INIT TRANSITIONING TO ACTIVE\n");
		if(IN_LDCURSTATE != S_STBY && IN_LDCURSTATE != S_INIT){
			//INIT_ERROR(("INrm_check_state(): Invalid state transition, from %c to %c",IN_LDCURSTATE,newstate));
      printf("INrm_check_state(): Invalid state transition, from %c to %c\n",
             IN_LDCURSTATE,newstate);
		}
		IN_LDCURSTATE = newstate;
		/* Kill anything that may be running */
    INkillprocs(FALSE);
		/* Release all shared memory except INIT	*/
    INfreeres(FALSE);
		/* Completely reinitialize INIT's shared memory	*/
		IN_LDSTATE.sn_lvl = SN_LV4;
		INinit();
		/* Act like INIT intended to die		*/
		IN_LDSTATE.initstate = INITING2;
		exit(0);
	case S_LEADACT:
		//CR_PRM(POA_INF, "REPT INIT TRANSITIONING TO LEAD");
    printf("REPT INIT TRANSITIONING TO LEAD\n");
		if(IN_LDCURSTATE != S_ACT && IN_LDCURSTATE != S_INIT){
			//INIT_ERROR(("INrm_check_state(): Invalid state transition, from %c to %c",IN_LDCURSTATE,newstate));
      printf("INrm_check_state(): Invalid state transition, from %c to %c\n",
             IN_LDCURSTATE,newstate);
		}

		prevState = IN_LDCURSTATE;
		IN_LDCURSTATE = newstate;

		/* If system is steady, then do level 3 initialization
		** otherwise shut everything down and do a level 5
		*/
		if(IN_LDSTATE.sn_lvl != SN_NOINIT && IN_LDSTATE.initstate != IN_CUINTVL){
			if(INevent.getLeadCC() > 0){
				INinitialize	initmsg;
				initmsg.sn_lvl = SN_LV5;
				INevent.sendToAllHosts("INIT", (char*)&initmsg, sizeof(initmsg), 0L, FALSE);
				IN_SLEEP(1);
			}
			/* Kill anything that may be running */
      INkillprocs(FALSE);
			/* Release all shared memory except INIT	*/
      INfreeres(FALSE);
			/* Completely reinitialize INIT's shared memory	*/
			IN_LDSTATE.sn_lvl = SN_LV5;
			INinit();
		} else {
			IN_LDSTATE.sn_lvl = SN_LV3;
		}
		/* Act like INIT intended to die		*/
		IN_LDSTATE.initstate = INITING2;
		exit(0);
	default:
		//INIT_ERROR(("INrm_check_state(): Invalid state change %c",newstate));
    printf("INrm_check_state(): Invalid state change %c\n",
           newstate);
		break;
	}
}


CRALARMLVL
INadjustAlarm(CRALARMLVL alvl)
{
	if(IN_procdata == (IN_PROCDATA *) -1){
		return(alvl);
	}

	if(!IN_ISACTIVE(IN_LDCURSTATE) && IN_LDCURSTATE  != S_STBY &&
     alvl != POA_MAN){
		return(POA_INF);
	}

	return(alvl);
}

