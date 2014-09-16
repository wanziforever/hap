
// DESCRIPTION:
// 	This file contains the methods of the class "INsemaphore" declared
//	in "cc/hdr/init/INsemaphore.H".  This class provides a standard
//	interface with INIT for processes which allocate and use
//	semaphores.  INIT will guarantee that semaphores allocated to
//	temporary processes will be deallocated when their associated
//	temporary processes are removed from the system.  Also, in certain
//	higher recovery levels INIT will remove all the semaphores created
//	via this class in an attempt to restart the system with a "clean
//	slate".
//
// FUNCTIONS:
//	INsemaphore()	- Class constructor
//	allocSem()	- Allocate a semaphore set
//	deallocSem()	- Deallocate a semaphore set
//	deallocAllSems() - Deallocate all the semaphores allocated
//			   to this process
//	
//
// NOTES:
//

#include <sysent.h>

#include "hdr/GLreturns.h"
#include "cc/hdr/init/INinit.h"
#include "cc/hdr/init/INusrinit.H"
#include "cc/hdr/init/INsemaphore.H"
#include "cc/hdr/init/INreturns.H"

INsemaphore::INsemaphore() {}	/* null constructor */


/*
** NAME:
**	allocSem()
**
** DESCRIPTION:
**	This method will attempt to allocate a semaphore and, if successful,
**	store its ID in INIT's process table associated with the calling
**	process.  This method first checks to see if the semaphore set already
**	exists and, if so, simply returns its ID.  If the semaphore set does
**	not exist then it is created and the new semaphore ID is returned.
**	If a new semaphore is created then this method returns a "new_flg"
**	value of "TRUE".  Otherwise, "new_flg" is returned with a value
**	of "FALSE".
**
** INPUTS:
**	semkey	- Key of the semaphore to be allocated/found
**	nsems	- The number of semaphores to be associated with
**		  the semaphore ID.
**	sem_perm - Permissions to be associated with the semaphore
**		   created via "semget()".
**
** RETURNS:
**	ret >= 0  - ID of semaphore associated with "semkey"
**
**	GLfail    - "semget()" call used to allocate a new semaphore failed,
**		    "errno" indicates the reason for the failure to allocate
**		     a set of semaphores.
**	INEEXIST  - The semaphore set associated with "memkey" already existed
**		    but was not associated with the calling process.
**	INMAXSEM  - There are no more free entries in the calling process's
**		    semaphore ID table
**
** RETURN PARAMETERS:
**	new_flg - "allocSem()" sets this flag to "TRUE" if a new semaphore
**		  set is created.  Otherwise, if the semaphore set already
**		  existed, then this value is returned as "FALSE".
**
** CALLS:
**	semget()  - To get the ID of existing semaphores and
**		    to allocate new semaphores.
**
** CALLED BY:
**	Temporary and permanent processes under the control of INIT which
**	wish to use semaphores.
**
** SIDE EFFECTS:
*/

int
INsemaphore::allocSem(key_t semkey, int nsems, int perm_flg, Bool &new_flg)
{
	int	ret_id;		/* Semaphore ID returned from "semget()" */
	U_short	nindx;
	int	*semids;

	new_flg = FALSE;

	semids = IN_SDPTAB[IN_PINDX].semids;

	for (nindx = 0; nindx < IN_NUMSEMIDS; nindx++) {
		if (semids[nindx] < 0) {
			break;
		}
	}

	if (nindx >= IN_NUMSEMIDS) {
		return(INMAXSEM);
	}

	if ((semkey != IPC_PRIVATE) &&
	    ((ret_id = semget(semkey, nsems, 0)) >= 0))  {
		/*
		 * Shared memory already exists, now see if the
		 * identifier is already associated with this
		 * process in INIT's process tables:
		 */
		U_short indx;
		for (indx = 0; indx < IN_NUMSEMIDS; indx++ ) {
			if (semids[indx] == ret_id) {
				break;
			}
		}

		if (indx < IN_NUMSEMIDS) {
			/*
			 * This process had already registered this
			 * semaphore identifier so it's the
			 * "rightful" owner, OK to return:
			 */
			return(ret_id);
		}

		/*
		 * At this point something may be wrong...only the "owner"
		 * of a semaphore should invoke "allocSem()" however this
		 * owner is asking for a semaphore which already exists in
		 * the system but which is not associated with this calling
		 * process?!?  For now, attempt to register this semaphore
		 * as being associated with this process:
		 */
		if (nindx < IN_NUMSEMIDS) {
			semids[nindx] = ret_id;
			return(ret_id);
		}
		return(INMAXSEM);
	}

	if (nindx >= IN_NUMSEMIDS) {
		/*
		 * This process has hit its limit for the number of semaphores
		 * it can create:
		 */
		return(INMAXSEM);
	}

	/*
	 * Attempt to create a new semaphore.  Note that we
	 * won't prevent permanent processes from creating more than
	 * "IN_NUMSEMIDS" segments since, presumably, permanent processes
	 * are responsible for guaranteeing that they don't lose
	 * semaphores -- unlike temporary processes which may be
	 * automatically removed from the system.
	 */
	ret_id = semget(semkey, nsems,((0x1ff & perm_flg) | IPC_CREAT));
	if (ret_id >= 0) {
		new_flg = TRUE;

		semids[nindx] = ret_id;
	}
	return(ret_id);
}

/*
** NAME:
**	deallocSem()
**
** DESCRIPTION:
**	This method deallocates a semaphore and removes its
**	segment ID from INIT's tables.  If the semaphore set is
**	not associated with the calling process and the calling
**	process is temporary, then it is not removed from the system.
**
** INPUTS:
**	sem_id	- The semaphore ID associated with the segment
**		  which is to be deallocated.
**
** RETURNS:
**	GLsuccess - The semaphore was successfully deallocated
**	GLfail	  - The "semctl()" system call failed, "errno" is set
**	INNOTALLOC - The semaphore ID was not found in the
**		     the table associated with the calling process (temporary
**		     procs. only)
**
** CALLS:
**	semctl()
**
** CALLED BY:
**
** SIDE EFFECTS:
*/
int
INsemaphore::deallocSem(int sem_id)
{
	int	*semids;
	U_short	nindx;

	Bool temp_flg = FALSE;

	semids = IN_SDPTAB[IN_PINDX].semids;
	if (IN_LDPTAB[IN_PINDX].permstate == IN_TEMPPROC) {
		temp_flg = TRUE;
	}

	for (nindx = 0; nindx < IN_NUMSEMIDS; nindx++) {
		if (semids[nindx] == sem_id) {
			semids[nindx] = -1;
			break;
		}
	}

	if ((nindx >= IN_NUMSEMIDS) &&
	    (temp_flg == TRUE)) {
		return(INNOTALLOC);
	}

	return (semctl(sem_id, 0, IPC_RMID, 0));
}
/*
** NAME:
**	deallocAllSems()
**
** DESCRIPTION:
**	This method deallocates all the semaphore IDs
**	associated with the calling process in INIT's process tables.
**
** INPUTS:
**
** RETURNS:
**
** CALLS:
**	semctl()
**
** CALLED BY:
**
** SIDE EFFECTS:
*/

Void
INsemaphore::deallocAllSems()
{
	int	sem_id;
	int	*semids;
	int	nindx;

	semids = IN_SDPTAB[IN_PINDX].semids;

	for (nindx = 0; nindx < IN_NUMSEMIDS; nindx++) {
		if ((sem_id = semids[nindx]) >= 0) {
			(Void)(semctl(sem_id, 0, IPC_RMID, 0));
			semids[nindx] = -1;
		}
	}
	return;
}

INsemaphore	INsem;
