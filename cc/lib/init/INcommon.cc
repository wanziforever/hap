
// DESCRIPTION:
// 	This file contains interfaces that can be used by any
//	process in the system, not just permanent processes.
//
// FUNCTIONS:
//	INattach() - attach to INIT shared memory
//	INdetach() - detach from INIT shared memory
//	INvmemsize() - available virtual memory size in K
//

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include "hdr/GLmsgs.h"
#include "cc/hdr/init/INproctab.hh"
#include "cc/hdr/init/INreturns.hh"
#include "cc/hdr/init/INlib.hh"
IN_PROCDATA 	*IN_procdata;

/*
 *      Name:
 *              INattach()
 *
 *      Description:
 *              Attach to INIT shared memory.
 *
 *      Inputs:
 *
 *      Returns:
 *             GLsuccess if attached,
 *		IN_SHMFAIL - if attach failed
 *		INBADSHEM - if shared memory version is not valid  
 *
 *      Calls:
 *		shmget(), shmat()
 *
 *      Called By:
 */

GLretVal
INattach()
{
	if(IN_procdata == (IN_PROCDATA *) 0){
		int shmid;
		if ( (shmid = shmget((key_t)INPDATA, sizeof(IN_PROCDATA),0)) < 0) {
       			 return(IN_SHMFAIL);
		}
		if ((IN_procdata = (IN_PROCDATA *) shmat(shmid, (Char *)0, SHM_RDONLY)) == ( IN_PROCDATA *)-1) {
			return(IN_SHMFAIL);
		}
	}

	if(IN_LDSHM_VER != IN_SHM_VER){
		INdetach();
		return(INBADSHMEM);
	} else {
		return(GLsuccess);
	}
}

/*
 *      Name:
 *              INdetach()
 *
 *      Description:
 *              Detach from INIT shared memory.
 *
 *      Inputs:
 *
 *      Returns:
 *		None.
 *
 *      Calls:
 *		shmdt()
 *
 *      Called By:
 */

void
INdetach()
{
	shmdt((char *)IN_procdata);
	IN_procdata = (IN_PROCDATA *)0;
}

/*
 *      Name:
 *              INvmemsize()
 *
 *      Description:
 *              Retrun current available virtual memory space.
 *		This value is updated every 15 seconds.
 *
 *      Inputs:
 *
 *      Returns:
 *		Size of available virtual memory in K bytes.
 *		Negative value if INIT shared memory cannot be accessed.
 *		A negative return of this function is an indication
 *		that PSP software is not running and the caller should
 *		most likely cleanup and exit.
 *
 *      Calls:
 *		INattach()
 *
 *      Called By:
 */
Long
INvmemsize()
{
	if(INattach() == GLsuccess){
		return(IN_LDVMEM);
	} else {
		return(-1L);
	}
}


/* Mapping of AINET priorities to UNIX priorities */
/**************************************************************
** IMPORTANT: if this table is changed, corresponding table in
** cc/init/proc/INdata.C must also be changed to match.
***************************************************************/
short INprio_map[IN_MAXPRIO] = {
29,	/* 0 = init */
26,
26,
26,
26,	/* 4 = first of call processing priority */
26,
23,
23,
23,
20,
20,
17,
17,	/* 12 = last of call processing priority */
16,	/* 13 = first of high OAM processes */
13,
13,
10,
10,
7,
7,	/* 19 = last of high OAM processes */
0,	/* 20 = regular OAM and default */
-3,
-3,
-6,
-6,	/* below this priority, the process may not escalate above call processing */
-9,
-9,
-12,
-12,
-15,
-15,
-18,
-18,
-21,
-21,
-24,	
-24,	/* 36 = filler class */
-27,	/* 37 = filler class */
-27,	/* 38 = filler class */
-27	/* 39 = filler class */
};

/*
 *      Name:
 *              INprio_change()
 *
 *      Description:
 *              Change the priority of the caller.
 *
 *      Inputs:
 *		priority - requested priority level
 *      Returns:
 *		GLfail - if priocnt() system calls failed
 *		GLsuccess - otherwise
 *
 *      Calls:
 *		priocntl()
 *
 */

GLretVal
INprio_change(int priority)
{
#ifdef __sun
	if(priority < 0 || priority >= IN_MAXPRIO){
		/* Reset the priority to insure array index in bounds when
		** performing priority translation. This will result in lowest
		** priority being set.
		*/
		priority = IN_MAXPRIO - 1;
	}
	pcparms_t	pcparms;
	static pcinfo_t	pcinfo;
	static short	sys_maxprio = 0;
	static float	p_ratio = 1.0;
	static short uprilim = 0;

	if(sys_maxprio == 0 || uprilim == 0){
		/* Get information on maximum priority */
		strcpy(pcinfo.pc_clname,"TS");
		if(priocntl((idtype_t)0,(id_t)0,PC_GETCID, (caddr_t)&pcinfo) == -1){
			return(GLfail);
		}

		sys_maxprio = ((tsinfo_t *)pcinfo.pc_clinfo)->ts_maxupri;
		if(sys_maxprio < INTS_MAXUPRI){
			p_ratio = (float)sys_maxprio/INTS_MAXUPRI;
		}

		/* Get priority limits for this user */
		pcparms.pc_cid = pcinfo.pc_cid;
		
	 	if(priocntl(P_PID,P_MYID,PC_GETPARMS,(caddr_t)&pcparms) < 0){
			return(GLfail);
		}
		
		uprilim = ((tsparms_t *)pcparms.pc_clparms)->ts_uprilim;
	}

	pcparms.pc_cid = pcinfo.pc_cid;
	((tsparms_t *)pcparms.pc_clparms)->ts_uprilim = uprilim;
	((tsparms_t *)pcparms.pc_clparms)->ts_upri = (short)(p_ratio * INMAP_PRIO(priority));
	/* Adjust priority to maximum allowable for this process */
	if(((tsparms_t *)pcparms.pc_clparms)->ts_upri > uprilim){
		((tsparms_t *)pcparms.pc_clparms)->ts_upri = uprilim;
	}

	/* Change the priority */
	if(priocntl(P_PID,P_MYID,PC_SETPARMS,(caddr_t)&pcparms) < 0){
		return(GLfail);
	}
#endif	
	return(GLsuccess);
}
