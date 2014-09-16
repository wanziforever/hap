/*
 * File:	cond.c
 *
 * Description:	This file contains the implementation of the Solaris UI
 *		condition variable synchronization implementations and
 *		converts them to Linux POSIX implementation.
 *
 *		This is a light-weight implementation library that chooses
 *		to not implement *all* Solaris UI capabilities.  For a full
 *		Solaris UI implementation (except system-wide synchronization)
 *		see the Solaris-compatible Thread Library (ScTL) project
 *		on sourceforge.net.
 *
 *		NOTE:	These Solaris UI synchronization methods can now
 *			return EWOULDBLOCK if there is a condition where
 *			the conditition variable cannot guarantee that the
 *			condition was signalled without being missed.
 *
 * Contents:	Implementation of the cond_*() Synchronization Functions:
 *
 *		cond_init()		- condition variable initialization
 *		cond_wait()		- wait on condition variable
 *		cond_timedwait()	- wait w/timeout on condition variable
 *		cond_reltimedwait()	- condition variable relative timedwait
 *		cond_signal()		- signal one (any) waiting thread
 *		cond_broadcast()	- signal all waiting threads
 *		cond_destroy()		- destroy condition variable
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
#include <sys/time.h>
#include <synch.h>


/*
 * Function:	cond_init()
 *
 * Description:	Initialize Solaris UI condition variable.
 *
 * Arguments:	cvp		- condition variable pointer
 *		type		- condition variable type
 *		arg		- argument (not used)
 *
 * Returns:	0 		= successful
 *		EFAULT		= illegal address or translation
 *		EINVAL		= invalid argument
 *		ENOMEM		= insufficient resources
 *		EBUSY		= attempt to re-initialize condition
 *		EPERM		= not a supported feature
 */

int
cond_init ( cond_t* cvp, int type, void* arg )
{
	int	rc;

	if ( cvp == NULL ) return EFAULT;

	switch( (cvp->__type = type) ) {

	case USYNC_PROCESS:
#ifdef FUTEX_2_2
		/* use futex-based condition variable */
		rc = fcond_init( &cvp->__shm_cond );
#else
		return EPERM;
#endif
		break;

	case USYNC_THREAD:
		/* use standard POSIX condition variable */
		rc = pthread_cond_init( &cvp->__std_cond, NULL );
		break;

	default:
		return EINVAL;
	}

	if ( rc == 0 ) cvp->__magic = COND_MAGIC;
	return rc;
}


/*
 * Function:	cond_wait()
 *
 * Description:	Solaris UI condition variable blocking wait.
 *
 * Arguments:	cvp		- condition variable pointer
 *		mp		- mutex protecting condition variable
 *
 * Returns:	0 		= signalled by fcond_signal or fcond_broadcast
 *		EWOULDBLOCK	= futex not equal to expected value
 *		EINTR		= interrupted by a singal or other spurious
 *				  interrupt
 *		EFAULT		= illegal address or translation
 *		EINVAL		= invalid operation
 *		EPERM		= not a supported feature
 */

int
cond_wait ( cond_t* cvp, mutex_t* mp )
{
	int	rc;

	if ( cvp == NULL || mp == NULL ) return EFAULT;

	switch ( cvp->__type ) {

	case USYNC_PROCESS:
#ifdef FUTEX_2_2
		/* use futex-based condition variable */
		rc = fcond_wait( &cvp->__shm_cond, &mp->__shm_mutex );
#else
		return EPERM;
#endif
		break;

	case USYNC_THREAD:
		/* use standard POSIX condition variable */
		rc = pthread_cond_wait( &cvp->__std_cond, &mp->__std_mutex );
		break;

	default:
		return EFAULT;
	}
	return rc;
}


/*
 * Function:	cond_timedwait()
 *
 * Description:	Solaris UI condition variable wait, blocking until at most a
 *		given absolute time in the future.
 *
 * Arguments:	cvp		- condition variable pointer
 *		mp		- mutex protecting condition variable
 *		abstime		- absolute future time to unblock
 *
 * Returns:	0 		= signalled by fcond_signal or fcond_broadcast
 *		ETIMEDOUT	= timed out (time reached abstime)
 *		EWOULDBLOCK	= futex not equal to expected value
 *		EINTR		= interrupted by a singal or other spurious
 *				  interrupt
 *		EFAULT		= illegal address or translation
 *		EINVAL		= invalid operation
 *		EPERM		= not a supported feature
 */

int
cond_timedwait ( cond_t* cvp, mutex_t* mp, timestruc_t* abstime )
{
	struct timespec	_abstime;
	int		rc;

	if ( cvp == NULL || mp == NULL || abstime == NULL )
		return EFAULT;

	if ( abstime->tv_nsec >= 1000000000 )
		return EINVAL;

	_abstime.tv_sec  = abstime->tv_sec;
	_abstime.tv_nsec = abstime->tv_nsec;

	switch ( cvp->__type ) {

	case USYNC_PROCESS:
#ifdef FUTEX_2_2
		/* use futex-based condition variable */
		rc = fcond_timedwait( &cvp->__shm_cond, &mp->__shm_mutex,
				      &_abstime );
#else
		return EPERM;
#endif
		break;

	case USYNC_THREAD:
		/* use standard POSIX condition variable */
		rc = pthread_cond_timedwait( &cvp->__std_cond, &mp->__std_mutex,
				  	     &_abstime );
		break;

	default:
		return EFAULT;
	}

	/* convert Linux/Solaris error codes */
	if ( rc == ETIMEDOUT ) rc = ETIME;
	return rc;
}


/*
 * Function:	cond_reltimedwait()
 *
 * Description:	Solaris UI condition variable wait, blocking until at most a
 *		given relative amount of time from current time.
 *
 * Arguments:	cvp		- condition variable pointer
 *		mp		- mutex protecting condition variable
 *		reltime		- relative time to block (NULL to block forever)
 *
 * Returns:	0 		= signalled by fcond_signal or fcond_broadcast
 *		ETIMEDOUT	= timed out (relative time passed)
 *		EWOULDBLOCK	= futex not equal to expected value
 *		EINTR		= interrupted by a singal or other spurious
 *				  interrupt
 *		EFAULT		= illegal address or translation
 *		EINVAL		= invalid operation
 *		EPERM		= not a supported feature
 */

int
cond_reltimedwait ( cond_t* cvp, mutex_t* mp, timestruc_t* reltime )
{
	int	rc;

	if ( cvp == NULL || mp == NULL || reltime == NULL )
		return EFAULT;

	if ( reltime->tv_nsec >= 1000000000 )
		return EINVAL;

	switch ( cvp->__type ) {

	case USYNC_PROCESS:
	{
#ifdef FUTEX_2_2
		/* use futex-based condition variable */
		struct timespec	_reltime;

		_reltime.tv_sec  = reltime->tv_sec;
		_reltime.tv_nsec = reltime->tv_nsec;

		rc = fcond_reltimedwait( &cvp->__shm_cond, &mp->__shm_mutex,
					 &_reltime );
#else
		return EPERM;
#endif
		break;
	}

	case USYNC_THREAD:
	{
		/* use standard POSIX condition variable */
		struct timeval	now;
		struct timespec _abstime;

		/* POSIX does not have a *cond_reltimedwait() version */ 
		gettimeofday( &now, NULL );
		_abstime.tv_sec = now.tv_sec + reltime->tv_sec;
		_abstime.tv_nsec = (now.tv_usec * 1000) + reltime->tv_nsec;
		if ( _abstime.tv_nsec > 1000000000 )
		{
			_abstime.tv_sec++;
			_abstime.tv_nsec -= 1000000000;
		}

		rc = pthread_cond_timedwait( &cvp->__std_cond, &mp->__std_mutex,
					     &_abstime );
		break;
	}

	default:
		return EFAULT;
	}

	/* convert Linux/Solaris error codes */
	if ( rc == ETIMEDOUT ) rc = ETIME;
	return rc;
}


/*
 * Function:	cond_signal()
 *
 * Description:	Signal one Solaris UI condition variable.
 *
 * Arguments:	cvp		- condition variable pointer
 *
 * Returns:	0 		= successful
 *		EFAULT		= illegal address or translation
 *		EINVAL		= invalid operation
 *		EPERM		= not a supported feature
 */

int
cond_signal ( cond_t* cvp )
{
	int	rc;

	if ( cvp == NULL ) return EFAULT;

	switch ( cvp->__type ) {

	case USYNC_PROCESS:
#ifdef FUTEX_2_2
		/* use futex-based condition variable */
		rc = fcond_signal( &cvp->__shm_cond );
#else
		return EPERM;
#endif
		break;

	case USYNC_THREAD:
		/* use standard POSIX condition variable */
		rc = pthread_cond_signal( &cvp->__std_cond );
		break;

	default:
		return EFAULT;
	}
	return rc;
}


/*
 * Function:	cond_broadcast()
 *
 * Description:	Broadcast signal all Solaris UI condition variables.
 *
 * Arguments:	cvp		- condition variable pointer
 *
 * Returns:	0 		= successful
 *		EFAULT		= illegal address or translation
 *		EINVAL		= invalid operation
 *		EPERM		= not a supported feature
 */

int
cond_broadcast ( cond_t* cvp )
{
	int	rc;

	if ( cvp == NULL ) return EFAULT;

	switch ( cvp->__type ) {

	case USYNC_PROCESS:
#ifdef FUTEX_2_2
		/* use futex-based condition variable */
		rc = fcond_broadcast( &cvp->__shm_cond );
#else
		rc = EPERM;
#endif
		break;

	case USYNC_THREAD:
		/* use standard POSIX condition variable */
		rc = pthread_cond_broadcast( &cvp->__std_cond );
		break;

	default:
		return EFAULT;
	}
	return rc;
}



/*
 * Function:	cond_destroy()
 *
 * Description:	Destroy Solaris UI condition variable.
 *
 * Arguments:	cvp		- condition variable pointer
 *
 * Returns:	0 		= successful
 *		EFAULT		= illegal address or translation
 *		EINVAL		= invalid argument
 *		EPERM		= not a supported feature
 *		EBUSY		= process is currently blocked on condition
 */

int
cond_destroy ( cond_t* cvp )
{
	int	rc;

	if ( cvp == NULL ) return EFAULT;

	switch ( cvp->__type ) {

	case USYNC_PROCESS:
#ifdef FUTEX_2_2
		/* use futex-based condition variable */
		rc = fcond_destroy( &cvp->__shm_cond );
#else
		rc = EPERM;
#endif
		break;

	case USYNC_THREAD:
		/* use standard POSIX condition variable */
		rc = pthread_cond_destroy( &cvp->__std_cond );
		break;

	default:
		return EFAULT;
	}

	/* invalidate condition variable data */
	cvp->__magic = 0;
	cvp->__type = 0xff;
	return rc;
}
