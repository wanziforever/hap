#include	<stdlib.h>
#include	<sys/types.h>
#include	<time.h>
#include	<sys/times.h>

#include	"hdr/GLtypes.h"
#include	"cc/hdr/tim/TMtimers.hh"
#include	"cc/hdr/tim/TMreturns.hh"
#include	"cc/lib/tim/TMlocal.hh"

U_long*		TMtimes = NULL;


//  Initialize current time variables:
Void
TMtimers::inittime()
{
	struct tms	dummy;

	if(TMtimes != NULL){
		TMthen = *TMtimes;
	} else {
		TMthen = (U_long)times(&dummy);
	}
	TMnow.hi_time = (U_short)TMthen>>30;
	TMnow.lo_time = TMthen & 07777777777;

	/*
	 * convert time() to TMITIME and then get diff between time() and times().
	 * times() return is stored in TMnow
	 */
	sectohz(time(0), &TMoffset);
	TMTIMSUB(TMoffset,TMnow);

	/*
	 * Want to check offset every hour or so, don't have to
	 * update TMoffset for another hour.
	 */
	TMlastupd = TMnow;
}

/* Update the current time
*/
GLretVal
TMtimers::updtime(Bool offsetFlg)
{
	U_long	newtime, timediff;
	struct tms dummy;

	/*
	 * If time is "frozen" then simply return:
	 */
	if (time_frozen == TRUE) {
		return(GLsuccess);
	}

	if(TMtimes != NULL){
		newtime = *TMtimes;
	} else {
		newtime = (U_long)times(&dummy);
	}

	/* Update the current TMITIME */
	if (!TMIS_ITIME(TMnow)) {
		// Internal error
		return(TMINTERR19);
	}
	if (TMthen > newtime) {

		// Rollover case:
		timediff = TMthen - newtime;
	}
	else {
		// Normal case:
		timediff = newtime - TMthen;
	}

	TMTIMINCR(TMnow, timediff);
	if (TMnow.hi_time < 0) {
		// Internal error
		return(TMINTERR20);
	}
	TMthen = newtime;

	/*
	 * Check to see if more than an hour has passed since TMoffset was
	 * last updated.  If TMnow is more than an hour (3600 * TMHZ) bigger
	 * than TMlastupd, call updoffset()
	 * Call TMITIMDIFF with smaller number first 
	 */
	if ((TMITIMDIFF(TMlastupd, TMnow) > (3600 * TMHZ)) || (offsetFlg == TRUE))
	{
		sectohz(time(0), &TMoffset);
		TMTIMSUB(TMoffset, TMnow);

		/*
		 * Want to check offset every hour or so, don't have to
		 * update TMoffset for another hour.
		 */
		TMlastupd = TMnow;
	}
	
	return(GLsuccess);
}


Void
TMtimers::updoffset()
{
	updtime(TRUE);
}
