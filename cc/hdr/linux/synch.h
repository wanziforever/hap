/*
 * File:	synch.h
 *
 * Description:	Implementation of Solaris UI synchronization primitives using
 *		standard POSIX pthread library primitives.  Also incorporates
 *		the futex-2.2 package to implement full inter-process 
 *		synchronization methods using the futex Linux kernel trap.
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
 *  06/06/2006  Shmuel Kallner  Updated struct mutex_t for 32/64 bit issues
 */
#ifndef _SYNCH_H
#define _SYNCH_H

#ifdef __linux
#define FUTEX_2_2		/* futex-2.2 support */
#endif

#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#ifdef FUTEX_2_2
#include "futex/usersem.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
 
/*
 *  Define Solaris UI synch definitions for various functions
 */
#define COND_MAGIC		0x4356          /* "CV" */
#define MUTEX_MAGIC		0x4d58          /* "MX" */

/* definitions of synchronization types */
#define USYNC_THREAD		0x00	/* private to a process */
#define USYNC_PROCESS		0x01	/* shared by processes */
#define LOCK_NORMAL		0x00	/* same as USYNC_THREAD */
#define LOCK_ERRORCHECK		0x02	/* error check lock */
#define LOCK_RECURSIVE		0x04	/* recursive lock */
#define USYNC_PROCESS_ROBUST    0x08	/* shared by processes robustly */ 

#ifdef FUTEX_2_2
#define FUTEX_INITIALIZER	{ 1 }
#define FCOND_INITIALIZER	{ 0 }
#endif

/* definitions for mutex and condition variable initializers */
#if __GNUC__ < 4
#define DEFAULTMUTEX                                            \
	{ USYNC_THREAD, MUTEX_MAGIC, 0,                               \
       (pthread_mutex_t)PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP }
#define RECURSIVEMUTEX                                            \
	{ USYNC_THREAD, MUTEX_MAGIC, 0,                                 \
       (pthread_mutex_t)PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP }
#define ERRORCHECKMUTEX                                           \
	{ USYNC_THREAD, MUTEX_MAGIC, 0,                                 \
       (pthread_mutex_t)PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP }
#define DEFAULTCV                                                       \
	{ USYNC_THREAD, COND_MAGIC, 0, (pthread_cond_t)PTHREAD_COND_INITIALIZER }
#else
#define DEFAULTMUTEX                            \
	{ USYNC_THREAD, MUTEX_MAGIC, 0,               \
       PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP }
#define RECURSIVEMUTEX                          \
	{ USYNC_THREAD, MUTEX_MAGIC, 0,               \
       PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP }
#define ERRORCHECKMUTEX                           \
	{ USYNC_THREAD, MUTEX_MAGIC, 0,                 \
       PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP }
#define DEFAULTCV                                           \
	{ USYNC_THREAD, COND_MAGIC, 0, PTHREAD_COND_INITIALIZER }
#endif

#ifdef FUTEX_2_2
#define SHAREDMUTEX                                             \
	{ USYNC_PROCESS, MUTEX_MAGIC, 0, (futex_t)FUTEX_INITIALIZER }
#define SHAREDCV                                                \
	{ USYNC_PROCESS, COND_MAGIC, 0, (fcond_t)FCOND_INITIALIZER }
#endif

#ifdef FUTEX_2_2
/*
 *  Define futex-2.2 interface type definitions for external access
 */
  struct _fcond
  {
    int	counter;
  };
  typedef struct _fcond		fcond_t;
  typedef struct futex		futex_t;
#endif


/*
 *  Define UI synch type definitions as they map into POSIX definitions
 */
  typedef struct mutex_t
  {
    uint16_t	__type;
    uint16_t	__magic;
    uint32_t        __padding;

    union
    {
      pthread_mutex_t	__std_mutex;	/* standard mutex	*/
      futex_t		__shm_mutex;	/* shared mutex		*/

      /* 
       * The following padding was added here to deal with
       * issues related 32/64 bit compilation using this
       * include. On Linux in 64 bit mode pthread_cond_t is
       * forty bytes long, while it is only twenty-four bytes
       * long in 32 bit mode.         (MAS2BCT Kallner)
       */
#ifdef __linux

#define PADSIZE  40
      char padding[ PADSIZE ];

      /*
       * The following will fail to compile if the sizeof
       * pthread_mutex_t is ever greater than 40.
       */
      char padding_check[ PADSIZE - sizeof(pthread_mutex_t) ];
#endif
    };
  }  mutex_t __attribute__ ((__aligned__(8)));

  typedef struct
  {
    uint16_t	__type;
    uint16_t	__magic;
    uint32_t        __padding;

    union
    {
      pthread_cond_t	__std_cond;	/* standard condition	*/
      fcond_t		__shm_cond;	/* shared condition	*/
    };
  }  cond_t  __attribute__ ((__aligned__(8)));

/*
 *  Define POSIX implementations for the various common Solaris UI synch
 *  implementations.
 */
#ifdef __STDC__

  extern int	mutex_init( mutex_t* mp, int type, void* arg );
  extern int	mutex_lock( mutex_t* mp );
  extern int	mutex_trylock( mutex_t* mp );
  extern int	mutex_unlock( mutex_t* mp );
  extern int	mutex_destroy( mutex_t* mp );

  extern int	cond_init( cond_t* cvp, int type, void* arg );
  extern int	cond_wait( cond_t* cvp, mutex_t* mp );
  extern int	cond_timedwait( cond_t* cvp, mutex_t* mp,
                              struct timespec* abstime );
  extern int	cond_reltimedwait( cond_t* cvp, mutex_t* mp,
                                 struct timespec* reltime );
  extern int	cond_signal( cond_t* cvp );
  extern int	cond_broadcast( cond_t* cvp );
  extern int	cond_destroy( cond_t* cvp );

#ifdef FUTEX_2_2
  extern int	fcond_init( fcond_t* cvp );
  extern int	fcond_wait( fcond_t* cvp, futex_t* mp );
  extern int	fcond_timedwait( fcond_t* cvp, futex_t* mp,
                               const struct timespec* abstime );
  extern int	fcond_reltimedwait( fcond_t* cvp, futex_t* mp,
                                  const struct timespec* reltime );
  extern int	fcond_signal( fcond_t* cvp );
  extern int	fcond_broadcast( fcond_t* cvp );
  extern int	fcond_destroy( fcond_t* cvp );
#endif

#else /* __STDC__ */

  extern int	mutex_init();
  extern int	mutex_lock();
  extern int	mutex_trylock();
  extern int	mutex_unlock();
  extern int	mutex_destroy();

  extern int	cond_init();
  extern int	cond_wait();
  extern int	cond_timedwait();
  extern int	cond_reltimedwait();
  extern int	cond_signal();
  extern int	cond_broadcast();
  extern int	cond_destroy();

#ifdef FUTEX_2_2
  extern int	fcond_init();
  extern int	fcond_wait();
  extern int	fcond_timedwait();
  extern int	fcond_reltimedwait();
  extern int	fcond_signal();
  extern int	fcond_broadcast();
  extern int	fcond_destroy();
#endif

#endif /* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif /* _SYNCH_H */
