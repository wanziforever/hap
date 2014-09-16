#ifndef __PTHREAD_UTILS_H
#define __PTHREAD_UTILS_H

#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __STDC__

extern  int pthread_cond_reltimedwait_np(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *reltime);

#else
extern  int pthread_cond_reltimedwait_np();
#endif /* __STDC__ */

#ifdef  __cplusplus 
}
#endif


#endif
