/*
 * File:	thread.h
 *
 * Description:	Implementation of Solaris UI thread administration primitives
 *		using standard POSIX pthread library primitives.  
 *
 *		Copyright (c) 2005 Lucent Technologies
 *
 * Owner:	Rick Lane
 *
 * Revision History:
 *
 *     Date       Developer                       Comment
 *  ----------	--------------	---------------------------------------------
 *  10/18/2005	Rick Lane	First version created.
 *  01/07/2007	IBM wyoes	Added support for thread_key_t
 */
#ifndef _THREAD_H
#define _THREAD_H

#include <signal.h>
#include <pthread.h>
#include "synch.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Define Solaris UI thread definitions for various functions
 */
#define THR_MIN_STACK   thr_min_stack()

/* define UI thread creation flags that map into POSIX options */
#define THR_BOUND		0x00000001  /* => PTHREAD_SCOPE_SYSTEM	    */
#define THR_NEW_LWP		0x00000002  /* Solaris obsolete 	    */
#define THR_DETACHED		0x00000040  /* => PTHREAD_CREATE_DETACHED   */
#define THR_SUSPENDED		0x00000080  /* not supported in POSIX	    */
#define THR_DAEMON		0x00000100  /* map: PTHREAD_CREATE_DETACHED */

/*
 *  Define UI thread type definitions as they map into POSIX definitions
 */
typedef pthread_t	thread_t;
typedef pthread_key_t	thread_key_t;

/*
 *  Define POSIX implementations for the various common Solaris UI thread
 *  implementations.
 */
#ifdef __STDC__

extern int	thr_create( void* stack_base, size_t stack_size,
			    void *(*start_func)(void *), void* arg, long flags,
                       	    thread_t* new_thread_ID );
extern int	thr_join( thread_t thread, thread_t* departed, void** status );
extern int	thr_suspend( thread_t thread );
extern int	thr_continue( thread_t thread );
extern void	thr_exit( void* status );
extern thread_t	thr_self( void );
extern int	thr_sigsetmask( int how, const sigset_t* set, sigset_t* oset );
extern int	thr_kill( thread_t thread, int sig );
extern size_t	thr_min_stack( void );
extern int	thr_keycreate( thread_key_t *keyp, void(*destructor)(void *) );
extern int	thr_setspecific( thread_key_t key, void *value );
extern int	thr_getspecific( thread_key_t key, void **valuep );
extern int	thr_getprio( thread_t target_thread, int *priority );
extern int	thr_setprio( thread_t target_thread, int priority );


#else /* __STDC__ */

extern int	thr_create();
extern int	thr_join();
extern int	thr_suspend();
extern int	thr_continue();
extern void	thr_exit();
extern thread_t	thr_self();
extern int	thr_sigsetmask();
extern int	thr_kill();
extern size_t	thr_min_stack();
extern int	thr_keycreate();
extern int	thr_setspecific();
extern int	thr_getspecific();
extern int	thr_getprio();
extern int	thr_setprio();

#endif /* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif /* _THREAD_H */
