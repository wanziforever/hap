
// DESCRIPTION:
//	This file contains the initialization signal handling routine
//	used to multiplex two logical signals -- for process HALT and
//	SYNC actions -- onto the SIGUSR2 signal.
//

#include <signal.h>

#include <sys/time.h>

#include "cc/lib/init/INlibinit.hh"

/*
 *	Name:
 *			_indist()
 *
 *
 *	Description:
 *		This routine processes the SIGUSR2 signal 
 *
 *	Inputs:
 *		SIGUSR2 - signal used for invocation
 *
 *		Shared Memory:
 *			IN_SDPTAB[]
 *			IN_LDPTAB[]
 *
 *		Private Memory:
 *			IN_ldata
 *
 *
 *	Calls:
 *		signal() - reestablish the signal handling disposition
 *		for this routine.  Also, all initialization sequences
 *		result in the SIGALRM, SIGCLD, and SIGUSR1 signals being
 *		disabled.  Permanent processes should not use any other
 *		signals as signal processing routines could interrupt the
 *		initialization sequence!
 *
 *		alarm() - disable asynchronous alarm signals
 *
 *		pause() - suspend execution waiting for a signal
 *
 *		exit() - terminate this process due to fatal errors.
 *
 *	Called By:
 *		This routine is invoked (transparently) on behalf of the
 *		client process whenever the SIGUSR2 synchronization
 *		signal is sent.
 *
 *	Side Effects:
 *		Certain error legs will result in voluntary termination
 *		of this process.
 *
 */



Void
_indist(int)
{
	struct itimerval	itmr_val;

	/*
	**  Disable all signals which may be used by a client process so
	**  that they don't interfere with the initialization sequence.
	**  Clients should not use any of these signals until they've
	**  completed their PROCINIT phase and have entered their
	**  main processing loop.
	**
	**  Also, disable SIGHUP, SIGINT to "immunize" the
	**  process from INIT's death.
	*/
	itmr_val.it_value.tv_sec = 0;
	itmr_val.it_value.tv_usec = 0;
	itmr_val.it_interval.tv_sec = 0;
	itmr_val.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &itmr_val, NULL);
	setitimer(ITIMER_VIRTUAL, &itmr_val, NULL);
	setitimer(ITIMER_PROF, &itmr_val, NULL);

	//(Void) alarm((U_short) 0);

	sigset(SIGINT, INsigintIgnore);
	signal(SIGPROF,SIG_IGN);
	signal(SIGVTALRM,SIG_IGN);
	signal(SIGHUP,SIG_IGN);
	signal(SIGALRM,SIG_IGN);
	sighold(SIGALRM);
	signal(SIGCLD,SIG_IGN);
	signal(SIGUSR1,SIG_IGN);

	/*
	**  Unblock the initialization sync. signal (SIGUSR2).
	*/
	(Void) sigrelse(SIGUSR2);

	_insync();
}

/*
 * The function 'INsigintIgnore' is used as the signal handler
 * for the signal SIGINT. The use of this function is preferable
 * to setting the signal handler to SIG_IGN, as it allows the
 * debugger DBX to halt a process for examination.
 */
void INsigintIgnore( int )
{
	;
}
