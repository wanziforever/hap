// DESCRIPTION:
//     This file contains the routine invoked by the iNITREQ macro to
//     request an initialization and a function responsible for escalation
//     of recovery based on software errors.

#include <stdlib.h>
#include <errno.h>
#include "cc/hdr/init/INreturns.hh"
#include "INlibinit.hh"
#include "thread.h"


/*
 *	Name:	
 *		_inreq()
 *
 *	Description:
 *		This routine updates the initialization shared memory
 *		area to indicate the level of initialization which was
 *		requested by the calling process.  When invoked by
 *		permanent processes this function will exit after
 *		setting the requested priority in the shared memory
 *		segment.
 *
 *	Inputs:
 *		initlevel - level of initialization
 *
 *		err_code  - error code associated with the init. request
 *
 *		string	  - additional information about the init request
 *		
 *		req_type  - what type of requests is this, i.e. should abort()
 *			    or exit() be used.
 *
 *		Private Memory:
 *			IN_ldata
 *
 *		Shared Memory:
 *			IN_SDPTAB[IN_PINDX]
 *
 *
 *	Returns:
 *			void
 *
 *	Calls:
 *
 *	Called By:
 *			Any user level client process needing an
 *			initialization.
 *
 *	Side Effects:
 *			User level initialization client processes will 
 *			terminate as a side effect of invoking this routine.
 */



extern mutex_t CRlockVarible;

Void
_inreq(SN_LVL initlevel, GLretVal err_code, const char * err_string,IN_REQTYPE req_type)
{
//  CRALARMLVL	alarm_lvl;
//
//	mutex_unlock(&CRlockVarible);
//	INOUT((IN_DEBUG | IN_IREQTR),(POA_INF,"inreq.c: _inreq(): routine entry:\n\tprocess = %s\n",IN_LDPTAB[IN_PINDX].proctag));
//
//	alarm_lvl = POA_MAJ;	// Default all messages to automatic alarm level
//
//	switch(initlevel){
//	case SN_LV0:
//		break;
//	case SN_LV1:
//		if(IN_LDPTAB[IN_PINDX].proc_category == IN_CP_CRITICAL ||
//		   IN_LDPTAB[IN_PINDX].proc_category == IN_PSEUDO_CRITICAL){ 
//			alarm_lvl = POA_CRIT;
//		}
//		break;
//	case SN_LV2:
//	case SN_LV3:
//	case SN_LV4:
//	case SN_LV5:
//	case IN_MAXSNLVL:
//		alarm_lvl = POA_CRIT;
//		break;
//	default:	/* Do not accept invalid initialization level */
//		return;
//	}
//
//	/* If initialization request was intentional, override
//	** the alarm level to POA_INFO and set err_code to GLsuccess
//	*/
//	if(req_type == IN_EXPECTED){
//		alarm_lvl = POA_INF;
//		err_code = IN_INTENTIONAL;
//	} 
//
//	/* Report that an initialization has been requrested */
//	if(alarm_lvl == POA_INF){
//		CR_PRM(alarm_lvl,"REPT INIT %s REQUESTED %s INIT DUE TO %s",
//           IN_LDPTAB[IN_PINDX].proctag,IN_SNLVLNM(initlevel),err_string);
//		IN_SDPTAB[IN_PINDX].alvl = POA_INF;
//	} else {
//    CR_X733PRM(alarm_lvl, IN_LDPTAB[IN_PINDX].proctag, qualityOfServiceAlarm,
//               softwareProgramAbnormallyTerminated, NULL, ";201",
//               "REPT INIT %s REQUESTED %s INIT DUE TO %s",
//               IN_LDPTAB[IN_PINDX].proctag,IN_SNLVLNM(initlevel),err_string);
//		IN_SDPTAB[IN_PINDX].alvl = alarm_lvl;
//	}
//	
//	/* Check if escalation is permitted, ignore softchecks
//	** if exit is intentional.  Intentional exit should be
//	** used sparingly.
//  */
//
//	if(req_type != IN_EXPECTED && 
//     (IN_LDPTAB[IN_PINDX].softchk == IN_INHSOFTCHK || 
//      IN_LDSTATE.softchk == IN_INHSOFTCHK)){
//		return;
//	}
//		
//	/* call secondary debugging script	*/
//	char	cmd[200];
//	sprintf(cmd, "/sn/init/debug %s %d %d", IN_LDPTAB[IN_PINDX].proctag, 
//          IN_LDPTAB[IN_PINDX].pid, err_code);
//	system(cmd);
//
//	/*
//	**  Store requested initialization level in shared memory
//	**  after checking to see if initialization request not already pending.
//	**  INIT process could have asked for initialization so insure that
//	**  initialization is not changed to something lower.
//	*/
//	if(IN_SDPTAB[IN_PINDX].ireq_lvl < initlevel){
//		IN_SDPTAB[IN_PINDX].ireq_lvl = initlevel;
//	}
//
//	IN_SDPTAB[IN_PINDX].ecode = err_code;
//
//	INOUT((INFORM | IN_ALWAYSTR),(POA_INF,"inreq.c: _inreq(): initialization request stored in shared memory:\n\tprocess = %s\n",IN_LDPTAB[IN_PINDX].proctag));
//
//	if(req_type == IN_ABORT){
//		abort();
//	} else {
//		exit((int) 0);
//	}
}

/*
 *	Name:	
 *		_inassert()
 *
 *	Description:
 *		This routine pegs per process and system software errors.
 *		If a process/system error threshold is exceeded, process/system
 *		initialization is requested.
 *
 *	Inputs:
 *		source - source of request (CRERROR or CRASSERT)
 *		Shared Memory:
 *			IN_SDPTAB[IN_PINDX]
 *
 *
 *	Returns:
 *			void
 *
 *	Calls:
 *
 *	Called By:
 *		CRERROR or CRASSERT
 */
Void
_inassert(Short source)
{

//	/* Do not count CRERROR generated asserts if not enabled */
//	if((IN_LDPTAB[IN_PINDX].crerror_inh == TRUE || IN_LDSTATE.crerror_inh == TRUE)
//     && source == CRerrorFlag)  {
//		return;
//	}
//
//	switch(IN_LDPTAB[IN_PINDX].proc_category){
//	case IN_CP_CRITICAL:
//	case IN_PSEUDO_CRITICAL:
//		/* Only count critical process errors toward system escalation */
//		if(++IN_SDERR_COUNT > IN_LDE_THRESHOLD){
//			/* Request system level init	*/
//			INITREQ(SN_LV2,INSYSERR_THRESH,"SYSTEM ERROR THRESHOLD EXCEEDED",IN_EXIT);
//			/* Software check inhibited, subtract system decrement rate from count */
//			IN_SDERR_COUNT -= IN_LDE_DECRATE;
//		}
//	default:
//		/* Check process errors regardless of the process category */
//		if(++(IN_SDPTAB[IN_PINDX].error_count) > 
//       IN_LDPTAB[IN_PINDX].error_threshold){
//			INITREQ(SN_LV0,INPROCERR_THRESH,"PROCESS ERROR THRESHOLD EXCEEDED",IN_EXIT);
//			/* Software check inhibited, subtract process decrement rate from count */
//			IN_SDPTAB[IN_PINDX].error_count -= IN_LDPTAB[IN_PINDX].error_dec_rate;
//		}
//	
//	} 
}

RET_VAL
_in_shutdown()
{
	if(IN_procdata == 0){
		return(IN_SHMFAIL);
	}

	if(kill(IN_procdata->pid, SIGUSR1) == -1){
		switch(errno){
		case ESRCH:
			return(INNOEXIST);
		case EPERM:
			return(INBLOCKED);
		default:
			return(ININVPARM);
		}
	}
	
	return(GLsuccess);
}
