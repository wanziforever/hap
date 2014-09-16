// DESCRIPTION:
//	This file contains a routine used by permanent
//	processes to peg their sanity control structures
//	that are monitored by the INIT subsystem both during initialization
//	and normal operation.
//
// FUNCTIONS:
//	_in_step()
//	_in_progress()
//
// NOTES:
//
#include "cc/hdr/init/INreturns.hh"
#include "cc/lib/init/INlibinit.hh"
#include "cc/hdr/cr/CRalarmLevel.hh"
#include <string.h>

/*
 *	Name:	
 *		_in_step()
 *
 *	Description:
 *		This routine is called by client processes periodically
 *		during initializations to indicate that they are "sane".  
 *		The INIT verifies that the progress_mark is periodically
 *		updated and that it is advancing.  Backward change in 
 *		progress_mark will cause initialization.
 *
 *	Inputs:
 *		progress_mark - current place in process initialization sequence
 *		step_desc - string describing the step
 *
 *	Returns:
 *		none
 *
 *	Calls:
 *		none
 *
 *	Called By:
 *		All SN client processes during initialization.
 *		This is	done via the IN_STEP macro.
 *
 */
Void
_in_step(U_long progress_mark, const char *)
{
	// Ignore if process not initializing
	if(IN_SDPTAB[IN_PINDX].procstep == IN_STEADY){
		return;
	}

	// Do not allow progress marks to go backwards
	if(IN_SDPTAB[IN_PINDX].progress_mark >= progress_mark){
		INITREQ(SN_LV0,IN_BACKWARD_PROGRESS,"BACKWARD INIT PROGRESS",IN_EXIT);
	}
	
	IN_SDPTAB[IN_PINDX].progress_check = 0;
	IN_SDPTAB[IN_PINDX].progress_mark = progress_mark;
}

extern const char* INcompleteStr;

/*
 *	Name:	
 *		_in_progress()
 *
 *	Description:
 *		This routine is called by client processes periodically
 *		during initializations to output progress indication to the craft.
 *
 *	Inputs:
 *		step_desc - string describing the step
 *
 *	Returns:
 *		none
 *
 *	Calls:
 *		none
 *
 *	Called By:
 *		All SN client processes during initialization.
 *		This is	done via the IN_PROGRESS macro.
 *
 */

Void
_in_progress(const char *step_desc)
{
	if(strcmp(step_desc, INcompleteStr) == 0){
		if(IN_SDPTAB[IN_PINDX].alvl == POA_INF){
      //CR_PRM(POA_INF,"REPT INIT %s PROGRESS %ld %s",
      //       IN_LDPTAB[IN_PINDX].proctag,IN_SDPTAB[IN_PINDX].progress_mark,step_desc);
      printf("REPT INIT %s PROGRESS %d %s\n",
             IN_LDPTAB[IN_PINDX].proctag,IN_SDPTAB[IN_PINDX].progress_mark,step_desc);
		} else {
      //CR_X733PRM(POA_CLEAR, IN_LDPTAB[IN_PINDX].proctag, qualityOfServiceAlarm,
      //           softwareProgramAbnormallyTerminated, NULL, ";201",
      //           "REPT INIT %s PROGRESS %ld %s",
      //           IN_LDPTAB[IN_PINDX].proctag,IN_SDPTAB[IN_PINDX].progress_mark,step_desc);
      printf("REPT INIT %s PROGRESS %d %s\n",
             IN_LDPTAB[IN_PINDX].proctag,IN_SDPTAB[IN_PINDX].progress_mark,step_desc);
			IN_SDPTAB[IN_PINDX].alvl == POA_INF;
		}
	}  else {
    //CR_PRM(POA_INF,"REPT INIT %s PROGRESS %ld %s",
    //       IN_LDPTAB[IN_PINDX].proctag,IN_SDPTAB[IN_PINDX].progress_mark,step_desc);
    printf("REPT INIT %s PROGRESS %d %s\n",
           IN_LDPTAB[IN_PINDX].proctag,IN_SDPTAB[IN_PINDX].progress_mark,step_desc);
	}
}
