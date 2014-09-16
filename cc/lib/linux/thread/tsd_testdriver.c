/*
	20070110 IBM murthya : The following tester using 'thr_' based calls is based
	on the version using 'pthread_*' calls , obtained from
	http://publib.boulder.ibm.com/infocenter/iseries/v5r3/index.jsp?topic=/rzahw/rzahwe21rx.htm
*/

// the following is for pthread_get/set specific
#include <pthread.h>
#include <errno.h>

// The following two are for gettid()

#include <sys/types.h> 
#include <linux/unistd.h>
#include <unistd.h>

/* _syscallX macros go away in RH5 */
#ifdef _syscall0
_syscall0(pid_t,gettid)
#else
pid_t gettid(void)
{
        return syscall(__NR_gettid);
}
#endif

#include <stdio.h>
#include <stdlib.h>
 
#include "thread.h"

#define _MULTI_THREADED
#define THR_NULL ((thread_t) 0)

void foo(void);  /* Functions that use the threadSpecific data */
void bar(void);
void dataDestructor(void *data);
 
#define checkResults(string, val) {             \
 if (val) {                                     \
   printf("Failed with %d at %s", val, string); \
   exit(1);                                     \
 }                                              \
}
 
typedef struct {
  int          threadSpecific1;
  int          threadSpecific2;
} threadSpecific_data_t;
 
#define                 NUMTHREADS   2
pthread_key_t           threadSpecificKey;
 
 
void *theThread(void *parm)
{
   int               rc;
   threadSpecific_data_t    *gData;
   printf("Thread %.8x : Entered\n", gettid());
   gData = (threadSpecific_data_t *)parm;
   rc =thr_setspecific(threadSpecificKey, gData);
   checkResults("pthread_setspecific()\n", rc);
   foo();
   return NULL;
}
 
void foo() {
   void * gData   = NULL;
   thr_getspecific(threadSpecificKey,&gData);
   printf("Thread %.8x : foo(), threadSpecific data=%d %d\n",
          gettid(), ((threadSpecific_data_t *)gData)->threadSpecific1, ((threadSpecific_data_t *)gData)->threadSpecific2);
   bar();
}
 
void bar() {
   void *gData = NULL;
   thr_getspecific(threadSpecificKey,&gData);
   printf("Thread %.8x : bar(), threadSpecific data=%d %d\n",
          gettid(), ((threadSpecific_data_t *)gData)->threadSpecific1, ((threadSpecific_data_t *)gData)->threadSpecific2);
   int priority  = 99;
   thread_t curr = thr_self();
   thr_getprio(curr,&priority);
   printf("priority = %d\n",priority);
   return;
}
 
void dataDestructor(void *data) {
   printf("Thread %.8x (i.e)  %.8x : Free data\n", gettid(),thr_self());
   thr_setspecific(threadSpecificKey, NULL);
	sleep(1);
   free(data);
}
 
 
int main(int argc, char **argv)
{
  pthread_t             thread[NUMTHREADS];
  int                   rc=0;
  int                   i;
  threadSpecific_data_t        *gData;
 
  printf("Enter Testcase - %s\n", argv[0]);
  rc = thr_keycreate(&threadSpecificKey, dataDestructor);
  checkResults("thr_keycreate()\n", rc);
 
  printf("Create/start threads\n");
  for (i=0; i <NUMTHREADS; ++i) {
       /* Create per-thread threadSpecific data and pass it to the thread */
     gData = (threadSpecific_data_t *)malloc(sizeof(threadSpecific_data_t));
     gData->threadSpecific1 = i;
     gData->threadSpecific2 = (i+1)*2;
     rc = thr_create(NULL,thr_min_stack() + 32000,theThread,gData,THR_NULL,&thread[i]);

     checkResults("pthread_create()\n", rc);
  }
 
  printf("Wait for the threads to complete, and release their resources\n");
  for (i=0; i <NUMTHREADS; ++i) {

	int departed = 0;

     	// now set the thread priority
     	int priority = (i+1)*2*10;

	// thr_suspend()/thr_continue() supplied by Lucent are deprecated 
	// Thr_suspend needs to be written using mutexes and condition variables

	// Also, for POSIX, the only allowed priority associated with a policy of SCHED_OTHER is 0
	
     	printf("Thread  %.8x's priority being set to %d \n",thread[i],priority);
     	rc = thr_setprio(thread[i],priority);
 
        thr_getprio(thread[i],&priority);
     	printf("Thread  %.8x's priority retrieved as %d \n",thread[i],priority);

	rc = thr_join(thread[i],&departed,(void **)NULL);
  //	rc = pthread_join(thread[i], NULL);
     checkResults("pthread_join()\n", rc);
  }
 
// No equivalent from Sun
  // pthread_key_delete(threadSpecificKey);
  printf("Main completed\n");
  return 0;
}
