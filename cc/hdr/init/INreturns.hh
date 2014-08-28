#ifndef __INRETURNS_H
#define __INRETURNS_H

/*
**	File ID: 	@(#): <MID8521 () - 08/17/02, 29.1.1.1>
**
**	File:			MID8521
**	Release:		29.1.1.1
**	Date:			08/21/02
**	Time:			19:33:00
**	Newest applied delta:	08/17/02 04:32:45
**
** DESCRIPTION:
** 	This file contains INIT-specific return codes returned in MSGH
**	messages sent from the INIT process.
**
** NOTES:
*/
#include "hdr/GLreturns.h"

/*
 *  Error return values included in "INpCreateFail" messages:
 */

const GLretVal INDUPMSGNM = IN_FAIL;  /* MSGH name in INpCreate msg is a */
				      /* duplicate of an existing process*/
const GLretVal INFORKFAIL = (IN_FAIL-1);  /* Failed to fork() process    */
const GLretVal INMAXPROCS = (IN_FAIL-2);  /* Exceeded INIT's process table */
					  /* size			   */
const GLretVal	ININVPSET = (IN_FAIL-3);  /* Invalid processor set	   */
const GLretVal INNOEXIST  = (IN_FAIL-4);  /* Executable file passed in     */
					  /* "full_path" does not exist    */
const GLretVal INNOTEXECUT= (IN_FAIL-5);  /* Executable file passed in     */
					  /* "full_path" is not executable */
const GLretVal ININVPRIO  = (IN_FAIL-6);  /* Priority level exceeds maximum*/
					  /* allowable UNIX priority       */
const GLretVal ININVUID   = (IN_FAIL-7);  /* "uid" requested was invalid   */
					  /* -- i.e. less than zero	   */
const GLretVal ININVPARM  = (IN_FAIL-8);  /* Invalid parameters 	   */

const GLretVal INNOCORE	  = (IN_FAIL-9);  /* Could not create or access core */
					  /* directory			     */

/*
 *  Error return values included in "INkillProcFail" messages:
 *  (INNOPROC and INDEADPROC are also used in "INsetRstrtFail" and
 *   INNOPROC in "INinitSCN" messages)
 */
const GLretVal INNOPROC   = (IN_FAIL-10); /* Process does not exist */
const GLretVal INNOTTEMP  = (IN_FAIL-11); /* Process was not temporary */
const GLretVal INDEADPROC = (IN_FAIL-12); /* Proc. was already dead and */
					  /* scheduled to be removed from */
					   /* INIT's tables  */

/*
 *  Error return values include in "INsetRunLvlFail" messages:
 *  (Note that INVINITSTATE is also used in "INinitSCNFail" messages)
 */
const GLretVal ININVRUNLVL = (IN_FAIL-15); /* Requested run level is less */
					   /* than the current system run */
					   /* run level			  */
const GLretVal INVINITSTATE = (IN_FAIL-16); /* Requested increase in run lvl*/
					   /* with system init state not */
					   /* equal to "IN_NOINIT"       */
const GLretVal INBADINITLIST = (IN_FAIL-17); /* failure processing initlist */

/*
 *  Error return values included in "INinitSCN" messages:
 */
const GLretVal INVSNLVL     = (IN_FAIL-20); /* Requested recovery level was */
					    /* invalid			   */
const GLretVal INVPROCSTATE = (IN_FAIL-21); /* Single process re-init. was */
					    /* requested for a proc. which */
					    /* was not in the STEADY state */
/* The following error code is returned to CEPs that require UCL option
** in some situations.  If UCL was not supplied, INBLOCKED is returned.
*/
const GLretVal INBLOCKED = (IN_FAIL-30);    /* Action could not be performed */

const GLretVal INFILEOPFAIL = (IN_FAIL-31); /* File operation failed when clearing 	*/
					    /* system softchecks			*/
/*
 *  Error return values from the "allocSeg()" method defined within the
 *  "INsharedMem" class:
 */
const GLretVal INEEXIST   = (IN_FAIL-42); /* Shared memory segment is already*/
					  /* allocated and is not currently  */
					  /* associated with the calling     */
					  /* process.		     	     */

const GLretVal INMAXSEG   = (IN_FAIL-43); /* The maximum number of shared mem*/
					  /* segments have been allocated to */
					  /* the calling process 	     */

/*
 *  Error return values from the "deallocSeg()" method defined within the
 *  "INsharedMem" class:
 */
const GLretVal INNOTALLOC = (IN_FAIL-44); /* The shared memory segment ID */
					  /* argument was not stored in the */
					  /* process's shmid table 	 */
/*
 *  Error return values from the "INsemaphore::allocSem()" method:
 */
const GLretVal INMAXSEM   = (IN_FAIL-46); /* The maximum number of semaphores*/
					  /* have been allocated to the */
					  /* the calling process */


/* Initialization escalation reasons */

const GLretVal INSYSERR_THRESH = (IN_FAIL-50); 	// System error threshold has been
						// exeeded.
const GLretVal INPROCERR_THRESH = (IN_FAIL-51);	// Process error threshold has been
						// exceeded.
const GLretVal INTIMERFAIL = (IN_FAIL-52);	// Could not obtain a timer

const GLretVal INPROC_DEATH = (IN_FAIL-53);	// Unexpected process death

const GLretVal IN_SYNCTMR_EXPIRED = (IN_FAIL-54);//  Synchronization timer expired

const GLretVal IN_SANTMR_EXPIRED = (IN_FAIL-55);// Sanity timer expired

const GLretVal IN_BACKWARD_PROGRESS= (IN_FAIL-56); // Backward progress mark during init

const GLretVal ININVSYSTEP = (IN_FAIL-57);	// Invalid system step

const GLretVal IN_DEATH_THRESH = (IN_FAIL-58);  // Threshold of INIT deaths exceeded

const GLretVal IN_DEATH_INIT  = (IN_FAIL-59);   // INIT died during system initialization

const GLretVal IN_INVSNLVL = (IN_FAIL-60);	// Invalid sn_lvl during initialization

const GLretVal INPROGRESSFAIL = (IN_FAIL-61);	// Failed to make progress during init

const GLretVal IN_CREAT_FAIL = (IN_FAIL-62);	// Failed to create a process

const GLretVal IN_SHMFAIL = (IN_FAIL-63);	// Failed to attach to shared memory

const GLretVal INBADSHMEM = (IN_FAIL-64);	// Corrupted shared memory

const GLretVal IN_LV4_THRESHOLD = (IN_FAIL-65);	// Exceeded threshold of level 4 inits

const GLretVal INKMEM_FAIL = (IN_FAIL-66);	// Failed to access kernel info 

const GLretVal INRM_SS_TIMEOUT = (IN_FAIL-67); // Failed to connect to state server

const GLretVal INRM_SS_MACH_INFO = (IN_FAIL-68);// Failed to obtain machine info

const GLretVal INRM_NO_APP_RES = (IN_FAIL-69);	// Failed to obtain application resource info

const GLretVal INRM_NO_ALT_MACH = (IN_FAIL-70); // Failed to obtain alternative machine info

const GLretVal INRM_NET_FAIL = (IN_FAIL-71);	// Failed to attach to network port

const GLretVal INRM_THREAD_FAIL = (IN_FAIL-72); // Failed to create RM thread

const GLretVal IN_INTENTIONAL = (IN_FAIL-73);   // Intentional initialization

const GLretVal IN_KILLPROC = (IN_FAIL-74);   // Died as a result of killproc msg

const GLretVal IN_GQTIMEOUT = (IN_FAIL-75);   // Timeout when getting global queue

const GLretVal IN_LEADCONFLICT = (IN_FAIL-76);   // Conflict of lead nodes

const GLretVal IN_OAMLEADCONFLICT = (IN_FAIL-77);   // Conflict of oam lead nodes

const GLretVal IN_OAMREADYTIMEOUT = (IN_FAIL-78);   // Timed out on transition to OAM ready

const GLretVal IN_ACTIVEREADYTIMEOUT = (IN_FAIL-79);   // Timed out on transition to vhost ready
const GLretVal IN_ACTIVEVHOSTCONFLICT = (IN_FAIL-80);   // Active vhost conflict
const GLretVal IN_SETUPFAIL = (IN_FAIL-81);   		// InstallPlatform scripts failed
const GLretVal IN_SCRIPTSFAILED = (IN_FAIL-81);   	// Startup scripts failed

/* Switch CC command failure reasons */

const GLretVal INNOTSUPP = (IN_FAIL-150);	// Command not supported
const GLretVal INNOSTDBY = (IN_FAIL-151);	// No standby is present
const GLretVal INWDNORCC = (IN_FAIL-152);	// RCC not running on this node
const GLretVal INWDNOACCESS = (IN_FAIL-153);	// Could not access watchdog port
const GLretVal INWDOTHER = (IN_FAIL-154);	// Other reasons
const GLretVal INWDNOTTTY = (IN_FAIL-155);	// Watchodg device not tty
const GLretVal INWDWRFAILED = (IN_FAIL-156);	// Watchdog write failure
const GLretVal INWDRDFAILED = (IN_FAIL-157);	// Watchdog read failure
const GLretVal INWDRE = (IN_FAIL-158);		// Watchdog sent reset element
const GLretVal INBADSTATE = (IN_FAIL-159);	// Bad node state
const GLretVal INWDTOOLONG = (IN_FAIL-160);	// Message to watchdog is too long
#endif

