// DESCRIPTION:
//     This file contains the timer routine used to set timers, This
//     is the external entry point to be used by timer library clients

#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <hdr/GLtypes.h>

#include "cc/hdr/tim/TMtimers.hh"
#include "cc/hdr/tim/TMreturns.hh"

#include "cc/lib/tim/TMlocal.hh"

const Long TMSECONDSpDAY = 3000 * 24;
const Long TMmaxTCBs = 10000000;
int TMisInBlockingReceive = FALSE;

Long TMtimers::nTCBs = TM_DEF_nTCBs;

mutex_t TMtimers::tmrLock;
/* Initialize timer init flags */
TMtimers::TMtimers() {
	RTinit_flg = FALSE;
	ATinit_flg = FALSE;
	time_frozen = FALSE;
	TMatheap.theap = NULL;
	TMrtheap.theap = NULL;
	TMbtcb = NULL;
	mutex_init(&tmrLock, USYNC_THREAD, NULL);
}

/* Re-initialize timer init flags */
Void
TMtimers::tmrReinit() {
	RTinit_flg = FALSE;
	ATinit_flg = FALSE;
	time_frozen = FALSE;
	if(TMatheap.theap != NULL){
		free(TMatheap.theap);
		TMatheap.theap = NULL;
	}
	if(TMrtheap.theap != NULL){
		free(TMrtheap.theap);
		TMrtheap.theap = NULL;
	}
	if(TMbtcb != NULL){
		free(TMbtcb);
		TMbtcb = NULL;
	}
}

/* tmrInit initializes timer data structures */
GLretVal
TMtimers::tmrInit(Bool atim_flg, Long allocTCBs)
{
	register Long	t;
	register TMTCB	*tcbptr;

	if(allocTCBs > TMmaxTCBs || allocTCBs < 1){
		return(TMTOOMANY);
	}

	/* Check for invalid init state */
	if ((ATinit_flg == TRUE) ||
	    ((RTinit_flg == TRUE) && atim_flg == FALSE)) {
		return(TMINVINIT);
	}

	mypid = getpid();

	nTCBs = allocTCBs;
	if(TMbtcb == NULL && (TMbtcb = (TMTCB*)malloc((unsigned int)(sizeof(TMTCB)*TMTCBNUM))) == NULL) {
		return(TMNOMEM);
	}
 	
	if (RTinit_flg == FALSE) {
		// allocate the space for relative timer heap
		if(TMrtheap.theap == NULL && (TMrtheap.theap = (Long*) malloc((unsigned int)(sizeof(Long)*(TMTCBNUM+1)))) == NULL) {
			return(TMNOMEM);
		}
		/* Now, initialize all TMTCB's to be on the free list. */
		for (t = 0, tcbptr = &TMTCBA(t); t < TMTCBNUM; t++, tcbptr++) {
			tcbptr->tstate = TEMPTY;
			tcbptr->llink = t-1;
			tcbptr->rlink = t+1;
			tcbptr->thi   = TMNULL;
		}
	
		TMfftcb = 0;
		TMlftcb = TMTCBNUM-1;
		TMTCBA(TMfftcb).llink = TMNULL;
		TMTCBA(TMlftcb).rlink = TMNULL;
	
		/* Assign TMidletcb which indicates the number of TCB's on
		 * the TCB free list.  This variable is also assigned
		 * and/or referenced by TMcntcb(), TMfreetcb(), and
		 * TMgettcb().
		 */
		TMidletcb = TMTCBNUM;
	
		/* Initialize the timer request queue, implemented as a heap */
		TMrtheap.fftheap = 1;
		for (t = 0; t <= TMTCBNUM; t++) {
			TMrtheap.theap[t] = TMNULL;
		}
	
		/*
		 * Initialize time:
		 */
		inittime();	
		RTinit_flg = TRUE;
	
	}
	if (atim_flg == TRUE) {
		/* Attach to shared memory to access TOD offset */
/*
 *	9/17/91:
 *	Originally it was thought that a global offset would be used to
 *	compute expiration times for absolute timers.  However, it has
 *	now been decided that the GMT time on the system will NOT jump
 * 	when daylight savings time jumps occur.  Therefore, each instance
 *	of this class will store the current offset between the "times()"
 *	return value and the "time()" return value and the shared memory
 *	offset value is no longer used.
 *
*/
		// allocate the space for relative timer heap
		if(TMatheap.theap == NULL && (TMatheap.theap = (Long*) malloc((unsigned int)(sizeof(Long)*(TMTCBNUM+1)))) == NULL) {
			return(TMNOMEM);
		}
		/* Initialize the timer request queue, implemented as a heap */
		TMatheap.fftheap = 1;
		for (t = 0; t <= TMTCBNUM; t++) {
			TMatheap.theap[t] = TMNULL;
		}
	
		ATinit_flg = TRUE;
	}

	return(GLsuccess);
}


/*
 *	Name:
 *		TMtimers::setlRtmr()
 *
 *	Description:
 *		This routine allows timer library users to set both
 *		cyclic and non-cyclic timers.
 *
 *	Inputs:
 *		time	- Duration (in seconds) from now until timer
 *			  expiration.  For cyclic timers this is also
 *			  the timer's period.
 *
 *		tag	- Tag to be associated with the timer upon
 *			  its expiration.
 *
 *		c_flag	- "cyclic flag":
 *				TRUE	- Set a cyclic timer
 *				FALSE	- Set a one-shot timer
 *
 *		hz_flag	- "hertz flag":
 *				TRUE	- time is in clock ticks
 *				FALSE	- time is in seconds
 *
 *	Returns:
 *		>=0	- Timer index successfully allocated.  This value
 *			  should be used on any subsequent call to
 *			  clrTmr().
 *
 *		< 0	- TMERANGE: Invalid time passed to routine
 *			  TMENOTCB: No timer control blocks available for
 *				    timer allocation.
 *			  TMINTERR: Internal timing library error
 *	Calls:
 *		gettcb()
 *		updtime()
 *		tsched()
 *
 *	Called By:
 *		Timer library users wishing to set a timer.
 *
 *	Side Effects:
 */

Long
TMtimers::setlRtmr(Long time,U_long tag,Bool c_flag, Bool hz_flag)
{
	register Long	t;
	register TMTCB	*tcbptr;
	TMITIME go_off, interval;

	/* Verify that the timer library has been initialized */
	if (!RTinit_flg) {
		/* not initialized, return error */
		return(TMUNINIT);
	}

	/* (argument check) */
	if (time <= 0L) {
		/* The interval was invalid return */
		return(TMERANGE);
	}

	if (hz_flag == TRUE) {
		/*
		 * "time" is in TMHZ, validate range and convert to TMITIME
		 * format. 
		 */
		if ((time < 2) || (time > maxhz)) {
			/* Invalid time value */
			return(TMERANGE);
		}

		/* OK, convert to TMITIME: */
		interval.hi_time = 0;
		interval.lo_time = time;
	}
	else {
		/*
		 * Convert seconds to clock ticks:
		 */
		sectohz(time, &interval);
	}

	mutex_lock(&tmrLock);
	/* Get a TMTCB */
	t = gettcb();
	if (t < 0) {
		/*
		 * Couldn't get a TMTCB,
		 * the overload counter has been bumped
		 */
		if (TMINTERR(t) == TRUE) {
			/*
			 * An internal error was detected, clean up
			 * the timer library before returning:
			 */
			tmrReinit();
			tmrInit(ATinit_flg);
		}
		mutex_unlock(&tmrLock);
		return((GLretVal)t);
	}

	tcbptr = &TMTCBA(t);
	
	/* Set up some fields */
	tcbptr->tag = tag;

	if (c_flag) {
		/*
		 *  We want a cyclic timer here
		 */
		tcbptr->tstate = CRTIMER;
		tcbptr->period = interval;
	}
	else {
		/*
		 * Set up a one-shot timer
		 */
		tcbptr->tstate = ORTIMER;
	}

	/* Compute when it should go off */
	GLretVal ret = updtime(FALSE);
	if (ret != GLsuccess) {
		if (TMINTERR(ret) == TRUE) {
			/*
			 * An internal error was detected, clean up
			 * the timer library before returning:
			 */
			tmrReinit();
			tmrInit(ATinit_flg);
		}

		/* Pass error along... */
		mutex_unlock(&tmrLock);
		return((GLretVal)ret);
	}
	if(!TMIS_ITIME(TMnow)) {
		/*  Invalid current time, return */
		tmrReinit();
		tmrInit(ATinit_flg);
		mutex_unlock(&tmrLock);
		return(TMINTERR0);
	}

	go_off = TMnow;

	TMTIMADD(go_off, interval);

	if (!TMIS_ITIME(go_off)) {
		/*  Invalid expiration time (unexpected!), return */
		tmrReinit();
		tmrInit(ATinit_flg);
		mutex_unlock(&tmrLock);
		return(TMINTERR1);
	}

	tcbptr->go_off = go_off;

	/* Schedule the timer */
	ret = tsched(t, &TMrtheap);
	if (ret != GLsuccess) {
		if (TMINTERR(ret) == TRUE) {
			/*
			 * An internal error was detected, clean up
			 * the timer library before returning:
			 */
			tmrReinit();
			tmrInit(ATinit_flg);
		}
		mutex_unlock(&tmrLock);
		return((GLretVal)t);
	}
	mutex_unlock(&tmrLock);
	/* Since we are setting a timer, if there was a blocking read
	** in another thread, must break out of system call. Send SIGALRM
	** to the process to wake up the sleeping thread and recalculate
	** the timers. To avoid unnecessary signals, do this only if
	** the just set timer is the next to expire.
	*/
	if(TMisInBlockingReceive && (t == TMrtheap.theap[1])){
		kill(mypid, SIGALRM);
	}
	/* Return the systag */
	return(t);
}


/*
 *	Name:
 *		TMtimers::setlAtmr()
 *
 *	Description:
 *		This routine allows timer library users to set both
 *		cyclic and non-cyclic absolute timers.  Cyclic absolute
 *		timers are set to expire every "cycle" days.
 *
 *	Inputs:
 *		time	- Greenwich Mean Time at which the timer is to
 *			  expire.
 *
 *		tag	- Tag to be associated with the timer upon
 *			  its expiration.
 *
 *		cycle	- If set to 0, this is a "one shot" timer.
 *			  Otherwise, this timer will be cyclic with
 *			  a period of "cycle" days.
 *
 *	Returns:
 *		>=0	- Timer index successfully allocated.  This value
 *			  should be used on any subsequent call to
 *			  clrTmr().
 *
 *		< 0	- TMERANGE: Invalid time passed to routine
 *			  TMENOTCB: No timer control blocks available for
 *				    timer allocation.
 *			  TMINTERR: Internal timing library error
 *	Calls:
 *		gettcb()
 *		updtime()
 *		tsched()
 *
 *	Called By:
 *		Timer library users wishing to set a timer.
 *
 *	Side Effects:
 */

Long
TMtimers::setlAtmr(Long time,U_long tag,U_char cycle) {
	register Long	t;
	register TMTCB	*tcbptr;
	TMITIME exp_time, chk_time;

	/* Verify that the timer library has been initialized */
	if (!ATinit_flg) {
		/* not initialized, return error */
		return(TMAUNINIT);
	}

	/* (argument check) */
	if (time <= 0L) {
		/* The interval was invalid - return */
		return(TMERANGE);
	}

	/* Update the current time for validity checks */
	GLretVal ret = updtime(FALSE);
	if (ret != GLsuccess) {
		if (TMINTERR(ret) == TRUE) {
			/*
			 * An internal error was detected, clean up
			 * the timer library before returning:
			 */
			tmrReinit();
			tmrInit(ATinit_flg);
		}

		/* Pass error along... */
		return((GLretVal)ret);
	}

	if(!TMIS_ITIME(TMnow)) {
		/*  Invalid current time, return */
		tmrReinit();
		tmrInit(ATinit_flg);
		return(TMINTERR32);
	}

	/*
	 * The "time" must be greater than the offset plus
	 * the current value of "TMnow" plus the smallest
	 * allowable time increment (1 second):
	 */


	chk_time = TMnow;

	TMTIMADD(chk_time, TMoffset);
	TMTIMINCR(chk_time, TMHZ);

	sectohz(time, &exp_time);

	if (LTIME(exp_time, chk_time)) {
		return(TMERANGE);
	}

	mutex_lock(&tmrLock);
	/* Get a TMTCB */
	t = gettcb();
	if (t < 0) {
		/*
		 * Couldn't get a TMTCB,
		 * the overload counter has been bumped
		 */
		if (TMINTERR(t) == TRUE) {
			/*
			 * An internal error was detected, clean up
			 * the timer library before returning:
			 */
			tmrReinit();
			tmrInit(ATinit_flg);
		}
		mutex_unlock(&tmrLock);
		return((GLretVal)t);
	}

	tcbptr = &TMTCBA(t);
	
	/* Set up some fields */
	tcbptr->tag = tag;

	if (cycle > 0) {
		/*
		 *  We want a cyclic timer here
		 */
		tcbptr->tstate = CATIMER;
		sectohz((cycle * TMSECONDSpDAY), &(tcbptr->period));
	}
	else {
		/*
		 * Set up a one-shot timer
		 */
		tcbptr->tstate = OATIMER;
	}

	tcbptr->go_off = exp_time;

	/* Schedule the timer */
	ret = tsched(t, &TMatheap);
	if (ret != GLsuccess) {
		if (TMINTERR(ret) == TRUE) {
			/*
			 * An internal error was detected, clean up
			 * the timer library before returning:
			 */
			tmrReinit();
			tmrInit(ATinit_flg);
		}
		mutex_unlock(&tmrLock);
		return((GLretVal)t);
	}
	mutex_unlock(&tmrLock);
	/* Return the systag */
	return(t);
}


/*
 *	Name:
 *		TMtimers::clrRtmr()
 *
 *	Description:
 *		This function destroys a timer on the relative
 *		timer heap
 *
 *	Inputs:
 *		timer	- index of timer to be destroyed.  This value should
 *			  have been obtained via the return value from a
 *			  setRtmr() or setAtmr() call.
 *			
 *	Returns:
 *		GLsuccess	- Timer successfully removed from the heap
 *		GLfail		- Invalid timer tag passed to routine
 *
 *	Calls:
 *
 */

GLretVal
TMtimers::clrTmr(Long timer)
{
	register Long	t;

	if (RTinit_flg == FALSE) {
		/* Timer library not initialized... */
		return(TMUNINIT);
	}
	/* Turn the systag into a TMTCBI */
	t = timer;
	if ((t < 0) || (t >= TMTCBNUM)) {
		/* Out of range tag, or state is invalid */
		return(TMERANGE);
	}
	mutex_lock(&tmrLock);
	/* Check to see if the timer is really allocated */
	if (TMTCBA(t).tstate ==	TEMPTY) {
		/* Simply return, indicating that somebody's confused... */
		mutex_unlock(&tmrLock);
		return(TMNOTALOC);
	}

	/* Everything's fine, so we just call TMfreetcb, which takes care of
	 * All necessary unlinking.
	 */
	GLretVal ret = freetcb(t);
	mutex_unlock(&tmrLock);
	return(ret);
}


/*
 * This version of "tmrExp()" is simply here to support the original version
 * of the timer library when timer tags were shorts.
 */
GLretVal
TMtimers::tmrExp(U_short *tag, Long *time) 
{
	U_long ntag;
	GLretVal rval;
	rval = tmrExp(&ntag, time);
	if ((*time !=0) || (rval != GLsuccess)) {
		return(rval);
	}

	/*
	 * There is an expired timer, go ahead and set the tag while
	 * checking to verify that it fits in a "short":
	 */
	*tag = (U_short)ntag;
	if (*tag != ntag) {
		return(TMNOTSHORT);
	}
	return(GLsuccess);
}


/*
** NAME:
**	TMtimers::TMtmrExp()
**
** DESCRIPTION:
**	TMtmrExp() checks to see if any relative or absolute timers have
**	expired.
**
** INPUTS:
**	*tag	- Address into which the expired timer's tag is stored
**		  if a timer has expired.
**	*time	- Address into which the number of clock ticks until
**		  the next timer will expire is stored if no expired timer 
**		  is found.  If no timers are currently set, this value
**		  will be set to zero.
**
** RETURNS:
**	GLsuccess:
**		*exp_flg == TRUE:
**		    A timer has expired and its tag value has been
**		    stored in *tag.  In this event, *time is not set.
**		*exp_flag == FALSE:
**		    No timer has expired.  In this case *tag is not
**		    set, and *time is set to the number of clock ticks
**		    until the next timer will expire.  If no timers are
**		    set, *time is set to zero.
**	TMUNINIT:
**		The timer library has not been initialized
**
**	TMINTERR**:
**		An internal inconsistency has been detected in the timer
**		library.  Any error of this type will cause the timer
**		library to automatically re-initialize itself to a
**		state in which no timers are set.  The calling routine
**		should generate an error and clean up its timer
**		data, resetting any timers which it still wishes to
**		have set.
**	
** SIDE EFFECTS:
**		When one-shot timers expire they are removed from the heap
**		and placed back into the free list of timer control blocks.
**		When cyclic timers expire, there next expiration time is
**		computed and they're placed back in the appropriate
**		heap.
*/
GLretVal
TMtimers::tmrExp(U_long *tag, Long *time)
{
	register Long	t;
	register TMTCB	*tcbptr;
	GLretVal	ret;
	TMITIME		hz_offset;

	if (RTinit_flg == FALSE) {
		/* Timer library not initialized... */
		return(TMUNINIT);
	}

	hz_offset.hi_time = 0;
	hz_offset.lo_time = 0;


	/*
	 *  Set TCB pointer to the top of the heap
	 */
	Long at = TMNULL;

	mutex_lock(&tmrLock);
	/* The earliest timer is always the top of the heap */
	t = TMrtheap.theap[1];
	tcbptr = &(TMTCBA(t));

	/* Is the heap empty? */
	if (t == TMNULL) {
		if (TMrtheap.fftheap != 1) {
			/* Internal error */
			tmrReinit();
			tmrInit(ATinit_flg);
			mutex_unlock(&tmrLock);
			return(TMINTERR21);
		}
		if (ATinit_flg == FALSE) {
			/*
			 * Absolute timers not in use and no relative
			 * timers are currently set
			 */
			*time = -1L;
			mutex_unlock(&tmrLock);
			return(GLsuccess);
		}

		/*
		 * Check to see if an absolute timer is set:
		 */
		t = TMatheap.theap[1];

		if (t == TMNULL) {
			if (TMatheap.fftheap != 1) {
				/* Internal error */
				tmrReinit();
				tmrInit(ATinit_flg);
				mutex_unlock(&tmrLock);
				return(TMINTERR28);
			}

			/*
			 * Neither relative nor absolute timers are set:
			 */
			*time = -1L;
			mutex_unlock(&tmrLock);
			return(GLsuccess);
		}

		tcbptr = &(TMTCBA(t));

		hz_offset = TMoffset;
	}
	else if (ATinit_flg == TRUE) {
		/*
		 * Check to see if an absolute timer is also set:
		 */
		at = TMatheap.theap[1];

		if (at != TMNULL) {
			/*
			 * Check to see if the absolute timer will expire
			 * before the relative timer:
			 */
			TMTCB *atcbptr = &(TMTCBA(at));

			TMITIME exptime;

			
			exptime = TMoffset;
			TMTIMADD(exptime, tcbptr->go_off);

			if (LTIME(atcbptr->go_off, exptime)) {
				/*
				 * Absolute timer will expire next, setup
				 * "t" and "tcbptr" to check for the absolute
				 * timer's expiration. 
				 */
				t = at;
				tcbptr = atcbptr;
				hz_offset = TMoffset;
			}
		}
	}

	/*
	 * At this point, the selected timer should be at the top of
	 * its respective heap:
	 */
	if ((!TMIS_TCBI(t)) || (TMTCBA(t).thi != 1)) {
		tmrReinit();
		tmrInit(ATinit_flg);
		mutex_unlock(&tmrLock);
		return(TMINTERR24);
	}

	/*
	 *  Update the current time and compare with next timer to
	 *  expire - note that is now either set to 0 (for
	 *  relative timers) or to the current offset between the system
	 *  clock and the current GMT (for absolute timers):
	 */
	updtime(FALSE);

	TMTIMADD(hz_offset, TMnow);

	if (LETIME(tcbptr->go_off, hz_offset)) {
		/*
		 *  There is an expired timer, return timer tag
		 *  ...set time until "next" expiration to zero
		 */
		*tag = tcbptr->tag;
		*time = 0L;		

		switch(tcbptr->tstate) {
		case ORTIMER:
			/* One-shot relative timer, free up the TCB */
			ret = tunsched(t, &TMrtheap);
			if (ret != GLsuccess) {
				if (TMINTERR(ret) == TRUE) {
					/*
					 * An internal error was detected,
					 * clean up the timer library
					 * before returning:
					 */
					tmrReinit();
					tmrInit(ATinit_flg);
				}
				mutex_unlock(&tmrLock);
				return(ret);
			}
			freetcb(t);
			mutex_unlock(&tmrLock);
			return(GLsuccess);

		case CRTIMER:
			ret = tunsched(t, &TMrtheap);
			if (ret != GLsuccess) {
				if (TMINTERR(ret) == TRUE) {
					/*
					 * An internal error was detected,
					 * clean up the timer library
					 * before returning:
					 */
					tmrReinit();
					tmrInit(ATinit_flg);
				}
				mutex_unlock(&tmrLock);
				return(ret);
			}

			/* Cyclic interval timers have their go_off
			 * times incremented by their period, and are
			 * re-scheduled.
			 */
			do {
				TMTIMADD(tcbptr->go_off,
						tcbptr->period);
			} while (LTIME(tcbptr->go_off, TMnow));

			if (!(TMIS_ITIME(tcbptr->go_off))) {
				tmrReinit();
				tmrInit(ATinit_flg);
				mutex_unlock(&tmrLock);
				return(TMINTERR25);
			}

			tcbptr->tstate = CRTIMER;
			tsched(t, &TMrtheap);
			mutex_unlock(&tmrLock);
			return(GLsuccess);

		case OATIMER:
			/* One-shot absolute timer, free up the TCB */
			ret = tunsched(t, &TMatheap);
			if (ret != GLsuccess) {
				if (TMINTERR(ret) == TRUE) {
					/*
					 * An internal error was detected,
					 * clean up the timer library
					 * before returning:
					 */
					tmrReinit();
					tmrInit(ATinit_flg);
				}
				mutex_unlock(&tmrLock);
				return(ret);
			}
			freetcb(t);
			mutex_unlock(&tmrLock);
			return(GLsuccess);

		case CATIMER:
			ret = tunsched(t, &TMatheap);
			if (ret != GLsuccess) {
				if (TMINTERR(ret) == TRUE) {
					/*
					 * An internal error was detected,
					 * clean up the timer library
					 * before returning:
					 */
					tmrReinit();
					tmrInit(ATinit_flg);
				}
				mutex_unlock(&tmrLock);
				return(ret);
			}

			/* Cyclic absolute timers have their go_off
			 * times incremented by their period, and are
			 * re-scheduled.
			 */
			do {
				TMTIMADD(tcbptr->go_off,
						tcbptr->period);
			} while (LTIME(tcbptr->go_off, hz_offset));

			if (!(TMIS_ITIME(tcbptr->go_off))) {
				tmrReinit();
				tmrInit(ATinit_flg);
				mutex_unlock(&tmrLock);
				return(TMINTERR33);
			}

			tcbptr->tstate = CATIMER;
			tsched(t, &TMatheap);
			mutex_unlock(&tmrLock);
			return(GLsuccess);

		default:
			/* Undefined timer type... */
			tmrReinit();
			tmrInit(ATinit_flg);

			mutex_unlock(&tmrLock);
			return(TMINTERR26);
			
		}
	}

 	else {
		/*
		 *  Timer has not expired, simply return time until
		 *  the next timer will expire...or a "large" value if
		 *  it is going to expire well into the future:
		 */
		if (tcbptr->go_off.hi_time > (hz_offset.hi_time + 1)) {
			/*
			 *  Set timeout value to an "arbitrarily" large
			 *  number:
			 */
			*time = (Long)maxhz;	/* (2^30)-1 */
		}
		else {
			/*
			 *  Timer is within valid range so use
			 *  TMITIMDIFF macro
			 */
			*time = TMITIMDIFF(hz_offset, tcbptr->go_off);
		}
	}
	mutex_unlock(&tmrLock);
	return(GLsuccess);
}

/*
** NAME:
**	TMtimers::stopTime()
**
** DESCRIPTION:
**	stopTime() "freezes" timers so that, any timers which have not
**	expired when this routine is invoked will not age (or expire)
**	until a call to startTime() is made.
**
** INPUTS:
**
** RETURNS:
**	GLsuccess:
**		Timer aging has been successfully disabled.
**
**	TMUNINIT:
**		The timer library has not been initialized
**
**	TMNOTIME:
**		Time had been "frozen" by a previous call to this
**		routine (without a corresponding call to "startTime").
**
** SIDE EFFECTS
**		Timers will cease to expire after this routine is invoked
**		until a subsequent call to "startTime" is made"
*/

GLretVal
TMtimers::stopTime()
{
	if (RTinit_flg == FALSE) {
		/* Timer library not initialized... */
		return(TMUNINIT);
	}

	if (time_frozen == TRUE) {
		/* Time has already been stopped... */
		return(TMNOTIME);
	}

	/*
	 * Update current time so that we "freeze" time from the
	 * current point in time, NOT from the last time "tmrExp()"
	 * was invoked:
	 */
	GLretVal ret = updtime(FALSE);
	if (ret != GLsuccess) {
		if (TMINTERR(ret) == TRUE) {
			/*
			 * An internal error was detected,
			 * clean up the timer library
			 * before returning:
			 */
			tmrReinit();
			tmrInit(ATinit_flg);
		}
		return((GLretVal)ret);
	}
	
	time_frozen = TRUE;
	return(GLsuccess);
}

/*
** NAME:
**	TMtimers::startTime()
**
** DESCRIPTION:
**	startTime() re-enables timer aging and expiration.  This routine
**	adjusts all of the timers in the relative timer heap to account
**	for the time which has expired since "stopTime()" was invoked.
**	This routine also resets the "time_frozen" member so that
**	time can "march on" from this point.
**
**	It should be noted that time is not quite as "frozen" for
**	absolute timers.  The "current" time does not change
**	between the time "stopTime()" and "startTime()" are invoked
**	and, therefore, unexpired absolute timers will not expire
**	during the interval.  However, as soon as "startTime()" is
**	invoked these timers will "age" by an amount equal to
**	the duration between the "stopTime()" and "startTime()" calls --
**	unlike relative timers.
**	
**
** INPUTS:
**
** RETURNS:
**	GLsuccess:
**		Timer aging has been successfully re-enabled and
**		relative timers have been adjusted so that they
**		have the same time until expiration as they did
**		when "stopTime()" was invoked.
**
**	TMUNINIT:
**		The timer library has not been initialized
**
**	TMTIMEON:
**		Time is not currently "frozen".
**
**
** SIDE EFFECTS
**	Timers will be updated so that relative timers are as close
**	to expiration as they were when "stopTime()" was invoked.
**	Absolute timers will immediately "age" so that they will be
**	as close to expiration as they would have been is the
**	calls to "stopTime()"/"startTime()" had not occurred.
*/

GLretVal
TMtimers::startTime()
{
	U_long	stop_time, timediff;
	Long	tcb_indx;
	U_short	i;

	if (RTinit_flg == FALSE) {
		/* Timer library not initialized... */
		return(TMUNINIT);
	}

	if (time_frozen == FALSE) {
		/* Time isn't currently stopped... */
		return(TMTIMEON);
	}

	/*
	 * Save the time when "time stood still" and update the
	 * time so that it can "march on".  The difference between
	 * these then and now is the increment we should use to
	 * update timers on the relative timer heap:
	 */
	stop_time = TMthen;

	time_frozen = FALSE;
	GLretVal ret = updtime(FALSE);
	if (ret != GLsuccess) {
		if (TMINTERR(ret) == TRUE) {
			/*
			 * An internal error was detected,
			 * clean up the timer library
			 * before returning:
			 */
			tmrReinit();
			tmrInit(ATinit_flg);
		}
		return((GLretVal)ret);
	}

	if(!TMIS_ITIME(TMnow)) {
		/*  Invalid current time, return */
		tmrReinit();
		tmrInit(ATinit_flg);
		return(TMINTERR29);
	}

	if (stop_time > TMthen) {
		/* Rollover case: */
		timediff = stop_time - TMthen;
	}
	else {
		/* Normal case: */
		timediff = TMthen - stop_time;
	}

	/*
	 * Now update all the timers which are currently on the
	 * relative timer heap - note that the zero-th element
	 * of the heap is not used:
	 */
	for (i = 1; i < TMrtheap.fftheap; i++) {
		/*
		 * Get the TCB index of this heap element and
		 * verify that it's valid:
		 */
		tcb_indx = TMrtheap.theap[i];

		if ((!TMIS_TCBI(tcb_indx)) || (TMTCBA(tcb_indx).thi != i)) {
			/* Internal heap error, return */
			return(TMINTERR30);
		}

		TMTIMINCR((TMTCBA(tcb_indx).go_off), timediff);

		if (!TMIS_ITIME(TMTCBA(tcb_indx).go_off)) {
			/*  Invalid expiration time, return */
			tmrReinit();
			tmrInit(ATinit_flg);
			return(TMINTERR31);
		}
	}

	return(GLsuccess);
}

