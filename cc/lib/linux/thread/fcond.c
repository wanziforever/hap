/*
 * File:	fcond.c
 *
 * Description:	This file contains the implementation of the futex-2.2
 *		non-pthread condition variable synchronization implementations.
 *		This is a front-end implementation which calls the futex-2.2
 *		package non-pthread functions.
 *
 * Contents:	Implementation of the nonpthread*() Synchronization Functions:
 *
 *		fcond_init()		- condition variable initialization
 *		fcond_wait()		- wait on condition variable
 *		fcond_timedwait()	- wait w/timeout on condition variable
 *		fcond_reltimedwait()	- condition variable relative timedwait
 *		fcond_signal()		- signal one (any) waiting thread
 *		fcond_broadcast()	- signal all waiting threads
 *		fcond_destroy()		- destroy condition variable
 *
 * Owner:	Rick Lane
 *
 * Revision History:
 *
 *     Date       Developer                       Comment
 *  ----------	--------------	---------------------------------------------
 *  10/18/2005	Rick Lane	First version created.
 */
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <limits.h>
#include <sys/time.h>
#include <synch.h>

#ifdef FUTEX_2_2

/*
 * Function:	fcond_init()
 *
 * Description:	Futex condition variable initialization.
 *
 * Arguments:	cvp		- condition variable pointer
 *
 * Returns:	0 		= successful
 *		EFAULT		= illegal address or translation
 */

int
fcond_init ( fcond_t* cvp )
{
	if ( cvp == NULL ) return EFAULT;

	cvp->counter = 0;
	return 0;
}


/*
 * Function:	fcond_wait()
 *
 * Description:	Futex condition variable blocking wait.
 *
 * Arguments:	cvp		- condition variable pointer
 *		mp		- futex protecting condition variable
 *
 * Returns:	0 		= signalled by fcond_signal or fcond_broadcast
 *		EWOULDBLOCK	= futex not equal to expected value
 *		EINTR		= interrupted by a singal or other spurious
 *				  interrupt
 *		EFAULT		= illegal address or translation
 *		EINVAL		= invalid operation
 */

int
fcond_wait ( fcond_t* cvp, futex_t* mp )
{
	if ( cvp == NULL || mp == NULL ) return EFAULT;

	/* get condition variable and release lock */
	int value = cvp->counter;
	futex_up( mp );

	errno = 0;
	int ret = sys_futex( &cvp->counter, FUTEX_WAIT, value, NULL );
	if ( ret < 0 && errno != 0 )
		ret = errno;
	
	/* re-aquire lock before exiting */
	while ( futex_down( mp ) < 0 && errno == EINTR ) ;
	return ret;
}


/*
 * Function:	fcond_timedwait()
 *
 * Description:	Futex condition variable wait, blocking until at most a
 *		given absolute time in the future.
 *
 * Arguments:	cvp		- condition variable pointer
 *		mp		- futex protecting condition variable
 *		abstime		- absolute future time to unblock
 *
 * Returns:	0 		= signalled by fcond_signal or fcond_broadcast
 *		ETIMEDOUT	= timed out (time reached abstime)
 *		EWOULDBLOCK	= futex not equal to expected value
 *		EINTR		= interrupted by a singal or other spurious
 *				  interrupt
 *		EFAULT		= illegal address or translation
 *		EINVAL		= invalid operation
 */

int
fcond_timedwait ( fcond_t* cvp, futex_t* mp, const struct timespec* abstime )
{
	struct timeval	_now;
	struct timespec	now, rel;
	int		ret;

	if ( cvp == NULL || mp == NULL || abstime == NULL )
		return EFAULT;

	if ( abstime->tv_nsec >= 1000000000 )
		return EINVAL;

	/* get condition variable and release lock */
	int value = cvp->counter;
	futex_up( mp );

	/* get current time and check for premature timeout */
	gettimeofday( &_now, NULL );
	now.tv_sec  = _now.tv_sec;
	now.tv_nsec = _now.tv_usec * 1000;

	if ( now.tv_sec > abstime->tv_sec ||
	     (now.tv_sec == abstime->tv_sec && now.tv_nsec > abstime->tv_nsec) )
	{
		/* premature timeout (abstime in the past) */
		errno = ETIMEDOUT;
		ret = -1;
	}
	else
	{
		/* convert absolute time to relative for futex kernel call */
		rel.tv_sec  = abstime->tv_sec - now.tv_sec;
		rel.tv_nsec = abstime->tv_nsec - now.tv_nsec;
		if ( rel.tv_nsec < 0 ) {
			rel.tv_sec--;
			rel.tv_nsec += 1000000000;
		}

		errno = 0;
		ret = sys_futex( &cvp->counter, FUTEX_WAIT, value, &rel );
	}
	if ( ret < 0 && errno != 0 )
		ret = errno;

	/* re-aquire lock before exiting */
	while ( futex_down( mp ) < 0 && errno == EINTR ) ;
	return ret;
}


/*
 * Function:	fcond_reltimedwait()
 *
 * Description:	Futex condition variable wait, blocking until at most a
 *		given relative amount of time from current time.
 *
 * Arguments:	cvp		- condition variable pointer
 *		mp		- futex protecting condition variable
 *		reltime		- relative time to block (NULL to block forever)
 *
 * Returns:	0 		= signalled by fcond_signal or fcond_broadcast
 *		ETIMEDOUT	= timed out (relative time passed)
 *		EWOULDBLOCK	= futex not equal to expected value
 *		EINTR		= interrupted by a singal or other spurious
 *				  interrupt
 *		EFAULT		= illegal address or translation
 *		EINVAL		= invalid operation
 */

int
fcond_reltimedwait ( fcond_t* cvp, futex_t* mp, const struct timespec* reltime )
{
	if ( cvp == NULL || mp == NULL || reltime == NULL )
		return EFAULT;

	if ( reltime->tv_nsec >= 1000000000 )
		return EINVAL;

	/* get condition variable and release lock */
	int value = cvp->counter;
	futex_up( mp );

	errno = 0;
	int ret = sys_futex( &cvp->counter, FUTEX_WAIT, value,
			     (struct timespec *) reltime );
	if ( ret < 0 && errno != 0 )
		ret = errno;
	
	/* re-aquire lock before exiting */
	while ( futex_down( mp ) < 0 && errno == EINTR ) ;
	return ret;
}


/*
 * Function:	fcond_signal()
 *
 * Description:	Signal one futex condition variable.
 *
 * Arguments:	cvp		- condition variable pointer
 *
 * Returns:	0 		= successful
 *		EFAULT		= illegal address or translation
 *		EINVAL		= invalid operation
 */

int
fcond_signal ( fcond_t* cvp )
{
	if ( cvp == NULL ) return EFAULT;

	/* Unreliable if they don't hold mutex, but that's as per spec */
	cvp->counter++;

	errno = 0;
	int ret = sys_futex( &cvp->counter, FUTEX_WAKE, 1, NULL );

	/* FUTEX_WAKE returns the number of processes woken up */
	if ( ret > 0 )
		ret = 0;

	/* convert error number failure return to error return */
	if ( ret < 0 && errno != 0 )
		ret = errno;

	return ret;
}


/*
 * Function:	fcond_broadcast()
 *
 * Description:	Broadcast signal all futex condition variables.
 *
 * Arguments:	cvp		- condition variable pointer
 *
 * Returns:	0 		= successful
 *		EFAULT		= illegal address or translation
 *		EINVAL		= invalid operation
 */

int
fcond_broadcast ( fcond_t* cvp )
{
	if ( cvp == NULL ) return EFAULT;

	/* Unreliable if they don't hold mutex, but that's as per spec */
	cvp->counter++;

	errno = 0;
	int ret = sys_futex( &cvp->counter, FUTEX_WAKE, INT_MAX, NULL );

	/* FUTEX_WAKE returns the number of processes woken up */
	if ( ret > 0 )
		ret = 0;

	/* convert error number failure return to error return */
	if ( ret < 0 && errno != 0 )
		ret = errno;

	return ret;
}


/*
 * Function:	fcond_destroy()
 *
 * Description:	Destroy futex condition variables.
 *
 * Arguments:	cvp		- condition variable pointer
 *
 * Returns:	0 		= successful
 *		EFAULT		= illegal address or translation
 */

int
fcond_destroy ( fcond_t* cvp )
{
	if ( cvp == NULL ) return EFAULT;
	return 0;
}

#endif /*FUTEX_2_2*/
