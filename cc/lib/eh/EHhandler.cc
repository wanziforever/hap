
// DESCRIPTION:
// 	This file contains member functions for the "EHevent" class.
//
// FUNCTIONS:
//
// NOTES:
//

#include <sysent.h>
#include <signal.h>
#include <sys/time.h>

#include "hdr/GLtypes.h"
#include "cc/hdr/tim/TMreturns.hh"
#include "cc/hdr/tim/TMtmrExp.hh"
#include "cc/hdr/tim/TMmtype.hh"
#include "cc/hdr/eh/EHhandler.hh"
#include "cc/hdr/eh/EHreturns.hh"

#define EHSETTIMER(time)  caught_alarm = FALSE;     \
  signal(SIGALRM, EHcatcher);                       \
  timeVal.it_value.tv_sec = time / 1000;            \
  timeVal.it_value.tv_usec = (time % 1000) * 1000;  \
  setitimer(ITIMER_REAL, &timeVal, NULL)

#define EHRMTIMER()        setitimer(ITIMER_REAL, &reset, NULL)

static Bool caught_alarm;
static struct itimerval reset = {0, 0, 0, 0};   // used to turn off the timer
static struct itimerval timeVal = {0, 0, 0, 0}; // actual time interval

extern "C" void EHcatcher(int);

void
EHcatcher(int){
	caught_alarm = TRUE;
}

GLretVal EHhandler::getEvent(MHqid msgqid, Char *msg_p, Short &msgsz,
                             Long ptype, Bool block_flg,
                             EHEVTYPE etype, Bool msg_first) {
	Long new_msgsz;
	GLretVal ret;
	new_msgsz = (Long)msgsz;
	ret = getEvent(msgqid, msg_p, new_msgsz, ptype, block_flg,
                 etype, msg_first);
	msgsz = (Short)new_msgsz;
	return(ret);
}

GLretVal EHhandler::getEvent(MHqid msgqid, Char *msg_p, Long &msgsz,
                             Long ptype, Bool block_flg,
                             EHEVTYPE etype, Bool msg_first) {
	GLretVal tmret, ret;
	U_long	ttag;
	TMtmrExp *tmrmsg_p;
	Long save_msgsz;

	Bool rcv_flg = FALSE;	/* indicates whether we've tried to */
  /* receive a message or not	    */
	Long exptim = 0;

	/*
	 * Make a copy of the original value of "msgsz" as
	 * it may be zeroed out my "receive()" and we might
	 * want it later:
	 */
	save_msgsz = msgsz;

	/*
	 * Now check to see if we should start out with a non-blocking
	 * read from our message queue:
	 */
	if ((msg_first == TRUE) && (etype != EHTMRONLY)) {

		rcv_flg = TRUE;
		/*
		 * Try to read a message right away:
		 */
		ret = receive(msgqid, msg_p, msgsz, ptype, 0);
		if (ret != MHnoMsg) {
			/*
			 * Either we've received a message or received
			 * an error (other than "no message") from the
			 * receive() routine:
			 */
			return(ret);
		} else {
			/*
			 * Restore the original value of "msgsz"
			 */
			msgsz = save_msgsz;
		}
	}

	/*
	 * Check for an expired timer:
	 */
	if (etype == EHMSGONLY) {
		/*
		 * We're not to check the timer library, if blocking
		 * is allowed then set "exptim" for a blocking read
		 * from the MSGH queue:
		 */
		if (block_flg == TRUE) {
			exptim = -1;
		}
	} else {
		/*
		 * Verify that the message size is large enough to hold
		 * a "timer message"
		 */
		if (sizeof(class TMtmrExp) > msgsz) {
			return(EHTOOSMALL);
		}

		if ((tmret = tmrExp(&ttag, &exptim)) != GLsuccess) {
			/*
			 * Error return from timer library, pass error
			 * code along:
			 */
			return(tmret);
		}

		if (exptim == 0) {
			/*
			 * We've found an expired timer,
			 * format the "expired timer msg":
			 */
			msgsz = sizeof(class TMtmrExp);
			tmrmsg_p = (class TMtmrExp *)msg_p;
			tmrmsg_p->priType = 0;
			tmrmsg_p->msgType = TMtmrExpTyp;
			tmrmsg_p->tmrTag = ttag;
			return(GLsuccess);
		}

		if (time_frozen == TRUE) {
			/*
			 * If time has been "stopped" (via "stopTime()")
			 * then, effectively, there are no timers set
			 * since they won't expire until "startTime()"
			 * is invoked.  Therefore, force the "no
			 * timers are set" condition:
			 */
			exptim = -1;
		}

		if (exptim < 0) {
			/*
			 * No timers were set, or else time is "frozen"
			 * and the timers are not currently aging:
			 */
			if (etype == EHTMRONLY)  {
				/*
				 * We won't be trying to receive a
				 * message at this point so return:
				 */
				return(EHNOEVENT);
			}

			if (block_flg != TRUE) {
				/*
				 * Blocking is not allowed --
				 * set "exptim" to zero so that
				 * we don't block below:
				 */
				exptim = 0;
			}
			/*
			 * At this point, if blocking is allowed
			 * then "exptim" is set to -1 which will
			 * cause a block msg queue recieve below
			 */
		}
		else {
			/*
			 * "exptim" > 0 case, no timers have expired but
			 * at least one is currently set to expire in
			 * exptim "ticks"
			 */
			if (block_flg == TRUE) {
				/*
				 * A timer is set (exptim is > 0) and we're
				 * allowed to block
				 */
				if (etype == EHTMRONLY) {
					/*
					 * Not allowed to check the message
					 * queue but we're
					 * not prohibited from blocking --
					 * simply "pause()" until the timer
					 * expires...
					 */
					exptim = ((exptim * 1000)/TMHZ) + 1; /* Convert from "ticks" */
					if(exptim > 0){
						EHSETTIMER(exptim);
						pause();
						EHRMTIMER();
						if(caught_alarm == FALSE){
							/*
						 	 * We were interrupted, by
							 * signal other then SIGALRM,
						 	 * return immediately:
               */
							return(MHintr);
						}
					}

					/*
					 * The timer should have expired by
					 * this time so jump to "TMREXP"
					 * processing:
					 */
					goto TMREXP;
				}
				/*
				 * At this point "exptim" is greater than
				 * zero and we're allowed to fall through
				 * to the "receive()" method below for
				 * a timed attempt to get a message
				 */
			}
			else {
				/*
				 * A timer is set (exptim > 0) but we're
				 * not permitted to block.
				 */
				if ((rcv_flg == TRUE) || (etype == EHTMRONLY)) {
					/*
					 * Either we've already tried to
					 * receive a message from our MSGH
					 * queue or we're not permitted to
					 * receive messages, either way
					 * we must return at this point:
					 */
					return(EHNOEVENTNB);
				}

				/*
				 * Clear the "exptim" value so that the
				 * we don't attempt a blocking queue
				 * receive below:
				 */
				exptim = 0;
			}
		}
	}

	if (rcv_flg != TRUE) {
		/*
		 * We haven't attempted a non-blocking message receive
		 * yet.  For efficiency we should always try this so that
		 * we don't have to set up an itimer everytime the
		 * "msg_first" flag is FALSE.
		 *
		 * Also, note that we don't get here if "etype" is EHTMRONLY
		 * so there's no need to check it again
		 */

		ret = receive(msgqid, msg_p, msgsz, ptype, 0);
		if (ret != MHnoMsg)  {
			/*
			 * Either we've received a message, gotten an
			 * error in the "receive()" method, or failed to
			 * receive a message when we're not supposed to
			 * set up a blocking msg receive, either way
			 * pass the return code on:
			 */
			return(ret);
		}
		else {
			/*
			 * Restore the original value of "msgsz"
			 */
			msgsz = save_msgsz;
		}
	}

	if (exptim == 0) {
		/*
		 * We've already tried a non-blocking MSGH queue receive
		 * and failed and we're not to try a blocking receive
		 * -- simply return indicating that no events are
		 * currently pending:
		 */
		return(EHNOEVENTNB);
	}

	/*
	 * Now exptim should be set to the time to wait
	 * until a timer expires, or -1 (blocking, no
	 * timers set).  For "exptim" > 0, convert the "ticks" value
	 * returned from "timExp()" to the milliseconds value
	 * "receive()" expects:
	 */
	if (exptim > 0) {
		exptim = ((exptim * 1000)/TMHZ) + 1; /* Convert from "ticks" */
    /*	to ms (+1 for 0 case)*/
	}		
	
	/*
	 * Set up the (potentially) blocking message receive:
	 */
	ret = receive(msgqid, msg_p, msgsz, ptype, exptim);
	
	if (ret != MHtimeOut) {
		/*
		 * We received a message or an error occurred, return either
		 * way:
		 */
		return(ret);
	}

	/*
	 * Restore the original value of "msgsz"
	 */
	msgsz = save_msgsz;

	/*
	 * See if a timer has expired:
	 */
	TMREXP:		/* Place to which our "goto" goes... */

	/*
	 * Verify that the message size is large enough to hold
	 * a "timer message"
	 */
	if (sizeof(class TMtmrExp) > msgsz) {
		return(EHTOOSMALL);
	}

	tmret = tmrExp(&ttag, &exptim);
	
	if (tmret != GLsuccess) {
		/*
		 * Return the error returned by "tmrExp":
		 */
		return(tmret);
	}

	if (exptim == 0) {
		/*
		 * We've found an expired timer,
		 * format the "expired timer msg":
		 */
		msgsz = sizeof(class TMtmrExp);
		tmrmsg_p = (class TMtmrExp *)msg_p;
		tmrmsg_p->priType = 0;
		tmrmsg_p->msgType = TMtmrExpTyp;
		tmrmsg_p->tmrTag = ttag;
		return(GLsuccess);
	}
	else {
		/*
		 * Still don't have any expired timers...
		 */
		return(MHintr);
	}
}


/*
 * The "cleanup()" function removes the process's MSGH queue name and
 * re-initializes its timer library
 */
Void
EHhandler::cleanup(MHqid mhqid, const Char *name)
{
	(Void)rmName(mhqid, name, FALSE);
	tmrReinit();
}
