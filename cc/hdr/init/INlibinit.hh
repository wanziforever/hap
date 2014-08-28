
#ifndef __INLIBINIT_H
#define __INLIBINIT_H

/*
**	File ID: 	@(#): <MID8474 () - 02/19/00, 23.1.1.1>
**
**	File:			MID8474
**	Release:		23.1.1.1
**	Date:			05/13/00
**	Time:			13:44:59
**	Newest applied delta:	02/19/00
**
** DESCRIPTION:
** 	This file contains the data structures and manifest constants
**	needed by the Initialization library routines.
**
** NOTES:
*/
#include <signal.h>

#include "cc/hdr/init/INusrinit.hh"	/* Permanent process INIT info. */
///#include "cc/hdr/cr/CRmsg.hh"
//#include "cc/hdr/cr/CRprmMsg.hh"

/*
 * These macros are accessed within trace statements.  They verify that
 * enumerated typedefs are in range before translating them into
 * text strings for trace output:
 */
#define IN_PROCSTNM(indx) (((unsigned int)indx < (unsigned int)IN_MAXSTATE) ? IN_procstnm[(Short)indx] : "INV INDX")

#define IN_SQSTEPNM(indx) (((unsigned int)indx < (unsigned int)IN_MAXSTEP) ? IN_sqstepnm[(Short)indx] : "INV INDX")

#define IN_SNLVLNM(indx) (((unsigned int)indx < (unsigned int)IN_MAXSNLVL) ? IN_snlvlnm[indx] : "INV INDX")

#define INOUT(type,out_data) 						\
	if (type & IN_procdata->process_flags) {			\
		CR_PRM out_data ;					\
	}

/*
**  This macro was used in another project to update the Display Application
**  Page (DAP) on the 3B20D with each process's init state.  It has been
**  left here and in the INIT library code as a "place holder" if and when
**  the SN needs a similar facility.
*/
#define IN_DAPUPD(type,indx,sync,state) \
		;
/* !@#$%
		if (IN_PROCDATA.in_dapflag == YES) \
			_indapupd(type,indx,sync,state);
*/

extern Void _insync();		/* User Process INIT Synchronization	*/
				/* User Process signal distribution	*/
extern void INmain_init();	/* shared memory initialization		*/

			/* This #define is used for allocating the first
			 * shared memory for an INIT process such that 
			 * sufficient amount of heap memory can be added without
			 * causing run-time memory segment violations.
			 * The default algorithm used by the Kernel process to
			 * attach the shared memory segment is to take the
			 * current end of .data + .text + current heap (found
			 * by executing sbrk(0) ) and * round up to the
			 * next 2MB memory address.  This means
			 * the heap memory addressability is only between 
			 * 0 and 2MB depending on where the sbrk(0) happens to
			 * fall for each process.  Subsequent shmat after the
			 * first default to the address immediately greater than
			 * the last attached shared memory address.  Init solves
			 * this problem for all default shared memory
			 * attachments by making the first attachment at 
			 * 100 million bytes beyond the current sbrk(0) address
			 * -- which allows ample heap space and stack growth
			 * for almost all processes.  Any non-init spawned
			 * processes that expect to use any amount of heap
			 * memory (eg. use of * new and delete operators)
			 * and attach to shared memory segments should attach
			 * their first segment using
			 * 
			 *  addrtoattcach = (sbrk(0) + INSHMATADDRESSOFFSET);
			 *
			 *  shmat(id, addrtoattcach, rd/wrt permissions);
			 *
			 * All subsequent shared memory attachments should use 
			 *
			 *  shmat (id, (Char *)0, rd/wrt permissions);
			 */

#define INSHMATADDRESSOFFSET 100000000   /* 100 million bytes */

		/*
		 * feat 557 requires a more deterministic address
		 * than the above, this is it.  SPA's assume they
		 * have all addresses between 0x20000000 and
		 * 0x2fffffff for their exclusive use.
		 * In SVR4 (R4) the kernel picks a high virtual address 
		 * (0x40000000) to attach a shared memory segment.
		 * Picking a different address result in creation of 
		 * unnecessary memory region.  In the future, if the 
		 * kernel behaviour changes again, this variable can
		 * be modified to leave deterministic space for SPAs
		 * shared memory.
		 */
#define IN_SHMAT_ADDRESS	0x0

#endif  /* INLIBINIT */
