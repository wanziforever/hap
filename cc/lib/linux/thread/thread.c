/*
 * File:	thread.c
 *
 * Description:	This file contains the implementation of the Solaris UI
 *		thread implementations and converts them to Linux POSIX
 *		implementation.
 *
 *		This is a light-weight implementation library that chooses
 *		to not implement *all* Solaris UI capabilities.  For a full
 *		Solaris UI implementation (except system-wide synchronization)
 *		see the Solaris-compatible Thread Library (ScTL) project
 *		on sourceforge.net.
 *
 * Contents:	Implementation of the thr_*() Solaris UI Thread Functions:
 *
 * 		thr_create()		- thread creation
 *		thr_join()		- thread join (pickup thread death)
 *		thr_suspend()		- suspend thread
 *		thr_continue()		- resume thread
 *		thr_exit()		- thread exit
 *		thr_self()		- return my thread id
 *		thr_sigsetmask()	- setup thread signal mask
 *		thr_kill()		- send signal to a thread
 *		thr_min_stack()		- get minimum thread stack size
 *		thr_keycreate()		- allocate key for thread-specific data
 *		thr_setspecific()	- set thread-specific data
 *		thr_getspecific()	- get thread-specific data
 *		thr_getprio()		- get thread priority
 *		thr_setprio()		- set thread priority
 *
 * Owner:	Rick Lane
 *
 * Revision History:
 *
 *     Date       Developer                       Comment
 *  ----------	--------------	---------------------------------------------
 *  10/18/2005	Rick Lane	First version created.
 *  01/07/2007	IBM wyoes	Added support for thread_key_t
 *  02/25/2008	Rick Lane	Set detached state code was inadvertently
 *				commented out.
 *  05/11/2009	Rick Lane	Deprecate thr_suspend()/thr_continue() to 
 *				return ENOTSUP.
 */

#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <bits/local_lim.h>	/* PTHREAD_STACK_MIN */
#include <thread.h>
#include <synch.h>

int
thr_create ( void* stack_base, size_t stack_size, void *(*start_func)(void*),
	     void* arg, long flags, thread_t* new_thread_ID )
{
	pthread_attr_t	_attr;
	thread_t	_new_thread;
	int		rc = 0;

	/* map Solaris daemon thread type into a POSIX detached thread */
	if ( flags & THR_DAEMON ) flags |= THR_DETACHED;

	/* initialize and populate thread attributes */
	(void) pthread_attr_init( &_attr );

	/* set thread stack base and/or size */
	if ( stack_base != NULL )
	{
		/* stack_base set must have stack_size set */
		rc = (stack_size < PTHREAD_STACK_MIN ? EINVAL :
		      pthread_attr_setstack( &_attr, stack_base, stack_size ));
	}
	else if ( stack_size != 0 )
	{
		/* stack_base == NULL and  stack_size set */
		rc = (stack_size < PTHREAD_STACK_MIN ? EINVAL :
		      pthread_attr_setstacksize( &_attr, stack_size ));
	}

	/* Linux only supports SYSTEM scope threads */
	if ( rc == 0 && (flags & THR_BOUND) )
	{
		 rc = pthread_attr_setscope( &_attr, PTHREAD_SCOPE_SYSTEM );
	}

	/* create thread as detached if requested */
	if ( rc == 0 && (flags & THR_DETACHED) )
	{
		rc = pthread_attr_setdetachstate( &_attr,
						  PTHREAD_CREATE_DETACHED );
	}

	/* call the POSIX thread creation method */
	if ( rc == 0 )
	{
		rc = pthread_create( &_new_thread, &_attr, start_func, arg );
	}

	/* return new thread ID if user requested it */
	if ( rc == 0 && new_thread_ID != NULL )
	{
		*new_thread_ID = _new_thread;
	}
	(void) pthread_attr_destroy( &_attr );
	return rc;
}


int
thr_join ( thread_t thread, thread_t* departed, void** status )
{
	int	rc;

	/* joining any thread not supported in POSIX implementation */
	if ( thread == (thread_t)0 )
	{
		return EPERM;
	}

	/* call the POSIX thread join method */
	rc = pthread_join( thread, status );

	/* if join was successful, load the thread id of the departed thread */
	if ( rc == 0 && departed != NULL )
	{
		*departed = thread;
	}
	return rc;
}


int
thr_suspend ( thread_t thread )
{
	return ENOTSUP;
}


int
thr_continue ( thread_t thread )
{
	return ENOTSUP;
}


void
thr_exit ( void* status )
{
	return pthread_exit( status );
}


thread_t
thr_self ()
{
	return pthread_self();
}


int
thr_sigsetmask ( int how, const sigset_t* set, sigset_t* oset )
{
	return pthread_sigmask( how, set, oset );
}


int
thr_kill ( thread_t thread, int sig )
{
	return pthread_kill( thread, sig );
}


size_t
thr_min_stack ()
{
	return (size_t)PTHREAD_STACK_MIN;
}


int
thr_keycreate ( thread_key_t *keyp, void(*destructor)(void *) )
{
	return pthread_key_create( keyp, destructor );
}


int
thr_setspecific ( thread_key_t key, void *value )
{
	return pthread_setspecific( key, value );
}


int
thr_getspecific ( thread_key_t key, void **valuep )
{
	if ( (*valuep = pthread_getspecific( key )) == NULL )
		return -1;
	return 0;
}


int
thr_getprio ( thread_t target_thread, int *priority )
{
	int	policy;
	struct sched_param param;

	int rc = pthread_getschedparam( target_thread, &policy, &param );
	*priority = param.sched_priority;
	return rc;
}


int
thr_setprio ( thread_t target_thread, int priority )
{
	int	policy;
	struct sched_param param;

	/*
	 *  If pthread_getschedparam() does not return a 0, it means
	 *  target_thread already terminated.  Set the policy to SCHED_OTHER
	 *  in such a case, even though it is doubtful how much farther the
	 *  process will go after such a codition.  Otherwise, we reuse the
	 *  same value of policy that we get out of this call.
	 */
	if ( pthread_getschedparam( target_thread, &policy, &param ) != 0 )
	{
		policy = SCHED_OTHER;
	}
        param.sched_priority = priority;

	return pthread_setschedparam( target_thread, policy, &param );
}


