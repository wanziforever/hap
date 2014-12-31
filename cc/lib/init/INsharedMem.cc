
// DESCRIPTION:
// 	This file contains the methods of the class "INsharedMem" declared
//	in "cc/hdr/init/INsharedMem.H".  This class provides a standard
//	interface with INIT for processes which allocate and use
//	shared memory segments.  INIT will guarantee that shared memory
//	segments allocated to temporary processes will be deallocated
//	when their associated temporary processes are removed from
//	the system.
//
// FUNCTIONS:
//	INsharedMem()	- Class constructor
//	allocSeg()	- Allocate a shared memory segment
//	getSeg()	- Return the next segment allocated to this process
//	deallocAllSegs() - Deallocate all the shared memory segments allocated
//			   to this process
//	
//
// NOTES:
//

#include <sysent.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "thread.h"
#include "cc/hdr/init/INreturns.hh"
#include "cc/hdr/init/INinit.hh"
#include "cc/lib/init/INlibinit.hh"
#include "cc/hdr/init/INsharedMem.hh"
#include "cc/hdr/cr/CRalarmLevel.hh"

U_short INsharedMem::next_seg = 0;


/*
** NAME:
**	allocSeg()
**
** DESCRIPTION:
**	This method will attempt to allocate a shared memory segment and,
**	if successful, store the shared memory segment's ID in INIT's
**	process table associated with the calling process.  For permanent
**	processes this method first checks to see if the shared memory
**	segment already exists and, if so, simply returns its ID.
**
**	Temporary processes, however, will receive an error return if
**	they call this method with a key associated with a shared memory
**	segment which already exists.  This is because temporary processes
**	can only store segments which they themselves allocate since INIT
**	AUTOMATICALLY deallocates these segments when the process is
**	removed from the system.  When temporary processes are re-started
**	or re-initialized, they should retrieve shared memory IDs which
**	they may have previously allocated via the "getSeg()" method.
**
** INPUTS:
**	memkey	- Key of the shared memory segment to be allocated/found
**	size	- Size of the shared memory segment
**	perm_flg - Permissions to be associated with the shared memory
**		   segment created via "shmget()".
**	new_flg - "allocSeg()" sets this flag to "TRUE" is the shared
**		  memory segment is created.  Otherwise, if the
**		  shared memory segment already existed, then this
**		  value is returned as "FALSE".
**	rel_flg - if true, the shared memory will be released when the process
**		  owning it undergoes an init requireing reinit of configuration
**		  (SN_LV1, SN_LV3 and SN_LV4)
**	private_key - a private key that can be used to identify shared memory
**		  segement for a process
**
** RETURNS:
**	ret >= 0  - ID of shared memory segment associated with "memkey"
**
**	GLfail    - "shmget()" call used to allocate a new shared memory
**		    segment failed, "errno" indicates the reason for
**		    the failure to allocate shared memory.
**
**	INEEXIST  - The calling process was a temporary process and the shared
**		    memory segment associated with "memkey" already existed
**		    but was not associated with the calling process.
**	INMAXSEG  - There are no more free entries in the calling process's
**		    shared memory ID table (temporary procs. only)
**
** CALLS:
**	shmget()  - To get the ID of existing shared memory segments and
**		    to allocate new shared memory segments.
**	INmain_init() - to attach to INIT shared memory segments
**
** CALLED BY:
**	Temporary and permanent processes under the control of INIT which
**	wish to use shared memory segments.
**
** SIDE EFFECTS:
*/

int
INsharedMem::allocSeg(key_t memkey, unsigned long size, int perm_flg, Bool &new_flg, 
Bool rel_flg, IN_SHM_KEY private_key)
{
  printf("INsharedMem::allocSeg() enter\n");
	int	ret_id;		/* Shared memory ID returned from "shmget()" */
	U_short	nindx;

	/* allocSeg can be called by code running in global constructor.
	** Make sure INIT shared memory is properly attached before continuing.
	** INmain_init() may not return if shared memory could not be correctly
	** set up.
	*/

	if(IN_sdata == 0){
		INmain_init();
	}

	// We do not support shared memory allocation in lab mode
	if(IN_IS_LAB()){
    printf("lab mode return fail\n");
		return(GLfail);
	}

	new_flg = FALSE;
	Bool temp_flg = FALSE;

	if(private_key >= IN_MAX_KEY){
		return(GLfail);
	}

	if (IN_LDPTAB[IN_PINDX].permstate == IN_TEMPPROC) {
		temp_flg = TRUE;
	}

	mutex_lock(&IN_SDSHMLOCK);
	IN_SDSHMLOCKCNT++;
	for (nindx = 0; nindx < INmaxSegs; nindx++) {
		if (IN_SDSHMDATA[nindx].m_pIndex < 0) {
			break;
		}
	}

  printf("INsharedMem::allocSeg() going to check shared memory\n");
	if ((memkey != IPC_PRIVATE) &&
	    ((ret_id = shmget(memkey, size, 0)) >= 0))  {
		/*
		 * Shared memory already exists, now see if the
		 * identifier is already associated with this
		 * process in INIT's process tables:
		 */
		U_short indx;
		for (indx = 0; indx < INmaxSegs; indx++ ) {
			if (IN_SDSHMDATA[indx].m_pIndex == IN_PINDX && IN_SDSHMDATA[indx].m_shmid == ret_id) {
				break;
			}
		}

		if (indx < INmaxSegs) {
			/*
			 * This process had already registered this
			 * shared memory identifier so it's the
			 * "rightful" owner, OK to return:
			 *  Also, update release flag  and private key.
			 */
			IN_SDSHMDATA[indx].m_rel = rel_flg;
			IN_SDSHMDATA[indx].m_pkey = private_key;
			mutex_unlock(&IN_SDSHMLOCK);
			return(ret_id);
		}

		if (temp_flg == TRUE) {
			/*
			 * Temporary processes are not allowed
			 * to "allocate" shared memory segments
			 * which already exist and which are not
			 * already in their shared memory ID list
			 */
			mutex_unlock(&IN_SDSHMLOCK);
			return(INEEXIST);
		}

		/* At this point a process is trying to allocate a
		** public shared memory segment that it did not
		** allocate. With an exception of process death while
		** allocating memory segment, this could only happen 	
		** if multiple processes try to allocate the same
		** public shared memory. This will result in that memory
		** being deleted if that process dies.  Only one process
		** should create shared memory segments. Currently,
		** just print an error.  In the future, force all offenders
		** to change their code and possibly do not record this memory.
		*/

		//CR_PRM(POA_INF,"INIT: allocSeg called by %s when shared memory key %0lx already existed", IN_LDPTAB[IN_PINDX].proctag,memkey);
    printf("INIT: allocSeg called by %s when shared memory key %0lx already existed",
           IN_LDPTAB[IN_PINDX].proctag,memkey);
		
		if (nindx < INmaxSegs) {
			IN_SDSHMDATA[nindx].m_shmid = ret_id;
			IN_SDSHMDATA[nindx].m_rel = rel_flg;
			IN_SDSHMDATA[nindx].m_pkey = private_key;
			IN_SDSHMDATA[nindx].m_pIndex = IN_PINDX;
		}
		mutex_unlock(&IN_SDSHMLOCK);
		return(ret_id);
	}

	if (nindx >= INmaxSegs){
		mutex_unlock(&IN_SDSHMLOCK);
		return(INMAXSEG);
	}

	IN_SDSHMDATA[nindx].m_rel = rel_flg;
	IN_SDSHMDATA[nindx].m_pkey = private_key;
  printf("INsharedMem::allocSeg() going to shmget memory \n");
	if((IN_SDSHMDATA[nindx].m_shmid = shmget(memkey, size,((0x1ff & perm_flg) | IPC_CREAT))) >=0){
		new_flg = TRUE;
		IN_SDSHMDATA[nindx].m_pIndex = IN_PINDX;
	}

	ret_id = IN_SDSHMDATA[nindx].m_shmid;
	mutex_unlock(&IN_SDSHMLOCK);
	return(ret_id);
}

/*
** NAME:
**	getSeg()
**
** DESCRIPTION:
**	This function returns the shared memory segment IDs associated
**	with the calling process which are stored in INIT's process tables
**	to be returned to the caller.  Only the shared memory segments that
**	match the specified privte key are returned. 
**	A flag is provided to allow for
**	the shared memory index to be "rewound" to the beginning of
**	the caller's shared memory array.  If this flag is FALSE, the
**	next ID beyond the last one returned from the process's shared
**	memory table will be returned.
**
** INPUTS:
**	first_flg  -  TRUE: Indicates that the first shared memory ID
**			    stored in the process's shmid table is to be
**			    returned.
**		   - FALSE: Indicates that the next shared memory ID
**			    beyond the one returned from the previous
**			    call to this method is to be returned.
**	private_key - key which should be used to retrieve the shared
**			memory segments owned by the process.
**
** RETURNS:
**	>= 0:	Shared memory ID "owned" by the calling process
**	GLfail:	No more shared memory IDs are associated with this
**		process that match the specified private key 
**		beyond the last valid ID returned by a previous
**		call to this method.
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
*/
int
INsharedMem::getSeg(Bool first_flg,IN_SHM_KEY private_key)
{
	int	ret_id;
	U_short	nindx;

	/* getSeg can be called by code running in global constructor.
	** Make sure INIT shared memory is properly attached before continuing.
	** INmain_init() may not return if shared memory could not be correctly
	** set up.
	*/

	if(IN_sdata == 0){
		INmain_init();
	}

	if(IN_IS_LAB()){
		return(GLfail);
	}

	if(private_key >= IN_MAX_KEY){
		return(GLfail);
	}

	if (first_flg == TRUE) {
		next_seg = 0;
	}

	if (next_seg >= INmaxSegs) {
		return(GLfail);
	}

	mutex_lock(&IN_SDSHMLOCK);
	IN_SDSHMLOCKCNT++;

	for (nindx = next_seg; nindx < INmaxSegs; nindx++) {
		if ((IN_SDSHMDATA[nindx].m_pIndex == IN_PINDX) && 
		    ((ret_id = IN_SDSHMDATA[nindx].m_shmid) >= 0) && 
		    (IN_SDSHMDATA[nindx].m_pkey == private_key)){
			next_seg = nindx + 1;
			mutex_unlock(&IN_SDSHMLOCK);
			return(ret_id);
		}
	}

	mutex_unlock(&IN_SDSHMLOCK);
	return(GLfail);
}

/*
** NAME:
**	deallocSeg()
**
** DESCRIPTION:
**	This method deallocates a shared memory segment and removes its
**	segment ID from INIT's tables.
**
** INPUTS:
**	seg_id	- The shared memory segment ID associated with the segment
**		  which is to be deallocated.
**
** RETURNS:
**	GLsuccess - The shared memory segment was successfully deallocated
**	GLfail	  - The "shmctl()" system call failed, "errno" is set
**	INNOTALLOC - The shared memory segment ID was not found in the
**		     the table associated with the calling process (temporary
**		     procs. only)
**
** CALLS:
**	shmctl()
**
** CALLED BY:
**
** SIDE EFFECTS:
*/
int
INsharedMem::deallocSeg(int seg_id)
{
	struct shmid_ds	membuf;
	U_short	nindx;

	Bool temp_flg = FALSE;

	if (IN_LDPTAB[IN_PINDX].permstate == IN_TEMPPROC) {
		temp_flg = TRUE;
	}

	mutex_lock(&IN_SDSHMLOCK);
	IN_SDSHMLOCKCNT++;

	for (nindx = 0; nindx < INmaxSegs; nindx++) {
		if (IN_SDSHMDATA[nindx].m_pIndex == IN_PINDX && 
			IN_SDSHMDATA[nindx].m_shmid == seg_id) {
			memmove(&IN_SDSHMDATA[nindx], &IN_SDSHMDATA[nindx+1], sizeof(INshmemInfo) * (INmaxSegs - nindx - 1));
			IN_SDSHMDATA[INmaxSegs - 1].m_pIndex = -1;
			break;
		}
	}

	if ((nindx >= INmaxSegs) &&
	    (temp_flg == TRUE)) {
		mutex_unlock(&IN_SDSHMLOCK);
		return(INNOTALLOC);
	}

	mutex_unlock(&IN_SDSHMLOCK);
	return (shmctl(seg_id, IPC_RMID, &membuf));
}
/*
** NAME:
**	deallocAllSegs()
**
** DESCRIPTION:
**	This method deallocates all the shared memory segment IDs
**	associated with the calling process in INIT's process tables.
**
** INPUTS:
**
** RETURNS:
**
** CALLS:
**	shmctl()
**
** CALLED BY:
**
** SIDE EFFECTS:
*/

Void
INsharedMem::deallocAllSegs()
{
	struct shmid_ds	membuf;
	int	seg_id;
	int	nindx;

	mutex_lock(&IN_SDSHMLOCK);
	IN_SDSHMLOCKCNT++;
	for (nindx = 0; nindx < INmaxSegs; nindx++) {
		if ((IN_SDSHMDATA[nindx].m_pIndex == IN_PINDX) && 
			((seg_id = IN_SDSHMDATA[nindx].m_shmid) >= 0)) {
			(Void)shmctl(seg_id, IPC_RMID, &membuf);
			memmove(&IN_SDSHMDATA[nindx], &IN_SDSHMDATA[nindx+1], sizeof(INshmemInfo) * (INmaxSegs - nindx - 1));
			// Adjust nindx because of the memmove above
			nindx --;
			IN_SDSHMDATA[INmaxSegs - 1].m_pIndex = -1;
		}
	}
	mutex_unlock(&IN_SDSHMLOCK);
	return;
}

INsharedMem	INshmem;
