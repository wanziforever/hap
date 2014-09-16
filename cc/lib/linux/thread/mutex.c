// Description: This file contains the implementation of the
//     UI mutual exclusion lock synchronization implementations
//     and converts them to linux POSIX implementations.
//
//     This is a light-weight implementation library that choose
//     to not implement *all UI capbilities.
// 
// Contents: Implemention of the mutex_*() Synchronization Functions:
//
//     mutex_init()  - mutex initialization
//     mutex_lock()  - mutex lock
//     mutex_trylock() - lock mutex if not currently held
//     mutex_unlock()  - mutex unlock
//     mutex_destroy()  - destroy mutex

#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include "synch.h"

// Function: mutex_init()
// Description: Initialize UI mutext.
int mutex_init(mutex_t *mp, int type, void* arg) {
  pthread_mutexattr_t _attr;
  int _kind;
  int  rc;
  if (mp == NULL) return EFAULT;
  // POSIX doesn't support recursive error check locks
  if ((type & (LOCK_ERRORCHECK | LOCK_RECURSIVE)) ==  \
      (LOCK_ERRORCHECK | LOCK_RECURSIVE)) {
    return EPERM;
  }
  if ( type & (USYNC_PROCESS | USYNC_PROCESS_ROBUST) ) {
		return EPERM;
	} else {
		/* use standard POXIX mutex */
		mp->__type = USYNC_THREAD;

		/* set the mutex attribute */
		(void) pthread_mutexattr_init( &_attr );
		_kind = PTHREAD_MUTEX_ADAPTIVE_NP;
		if ( type & LOCK_RECURSIVE )
			_kind = PTHREAD_MUTEX_RECURSIVE_NP;
		if ( type & LOCK_ERRORCHECK )
			_kind = PTHREAD_MUTEX_ERRORCHECK_NP;
		rc = pthread_mutexattr_settype( &_attr, _kind );
		if ( rc == 0 )
			rc = pthread_mutex_init( &mp->__std_mutex, &_attr );
		(void) pthread_mutexattr_destroy( &_attr );
	}

	if ( rc == 0 ) mp->__magic = MUTEX_MAGIC;
	return rc;
}

// Description: Attempt to aquire (lock) a UI mutex
int mutex_lock(mutex_t *mp) {
  int rc;
  if ( mp == NULL ) return EFAULT;
  switch (mp->__type)  {
  case USYNC_PROCESS:
    return EPERM;
  case USYNC_THREAD:
    // use standard POSIX mutex
    rc = pthread_mutex_lock(&mp->__std_mutex);
    break;
  default:
    return EFAULT;
  }
  return rc;
}

// Description: Check if UI mutex lock is held and
// lock only if not currently held.
int mutex_trylock(mutex_t* mp) {
  int rc;
  if (mp == NULL) return EFAULT;
  switch (mp->__type) {
  case USYNC_PROCESS:
    return EPERM;
  case USYNC_THREAD:
    rc = pthread_mutex_trylock(&mp->__std_mutex);
    break;
  default:
    return EFAULT;
  }
  return rc;
}

// Description: Unlock UI mutex
int mutex_unlock(mutex_t *mp) {
  int rc;
  if (mp == NULL) return EFAULT;
  switch (mp->__type) {
  case USYNC_PROCESS:
    return EPERM;
  case USYNC_THREAD:
    rc = pthread_mutex_unlock(&mp->__std_mutex);
    break;
  default:
    return EFAULT;
  }
  return rc;
}
  
// Description: Destroy UI mutex
int mutex_destroy(mutex_t *mp) {
  int rc;
	if ( mp == NULL ) return EFAULT;

	switch( mp->__type ) {

	case USYNC_PROCESS:
		return EPERM;
	case USYNC_THREAD:
		/* use standard POSIX mutex */
		rc = pthread_mutex_destroy( &mp->__std_mutex );
		break;

	default:
		return EFAULT;
	}

	/* destory mutex data */
	mp->__type = 0xff;
	mp->__magic = 0;
	return rc;
}
