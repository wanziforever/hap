#ifndef __INSEMAPHORE_H
#define __INSEMAPHORE_H

// DESCRIPTION:
// 	This file includes the definition of the INsemaphore class.
//	This class should be used by processes started by INIT to
//	allocate/deallocate semaphores so that INIT can manage all
//	the semaphores in the system.  This allows INIT to deallocate
//	all the SCN/SCP-specific semaphores in the system during certain
//	higher recovery levels.
//
// FUNCTIONS:
//
// NOTES:
//
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

/*
 *  The INsemaphore class should be used to allocate/deallocate shared
 *  memory.  Note that this class should only be used in conjuction with
 *  the INIT library/INIT interface.
 *
 *  The methods for this class are defined in the INIT library which
 *  must be included by all processes under INIT's control
 */
class INsemaphore {
    public:
	INsemaphore();

	/*
	 * "allocSem" first checks to see if the requested set of semaphores
	 * exists and, if it does, returns the semaphore's ID and a "new_flg"
	 * value of "FALSE".  If the semaphore set associated with "semkey"
	 * did not previously exist, "allocSem" will create it, return
	 * the semaphore ID of the newly created set of semaphores, and
	 * a "new_flg" value of "TRUE":
	 */
	int allocSem(key_t semkey, int nsems, int semflg, Bool &new_flg);
	int deallocSem(int semid);
	Void deallocAllSems();
};

extern INsemaphore	INsem;
#endif
