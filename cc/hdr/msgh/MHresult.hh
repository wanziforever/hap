#ifndef __MHRESULT_H
#define __MHRESULT_H

/*
**	File ID:	@(#): <MID8698 () - 08/17/02, 29.1.1.1>
**
**	File:					MID8698
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:33:37
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**	Definition of MSGH funtion return constants
**
** OWNER:
**	Shi-Chuan Tu
**
** NOTES:
*/

#include "hdr/GLreturns.h"

enum MHresult {
	MHnoEnt = MSGH_FAIL,    // not exist ( = -12000)
	MHintr = MSGH_FAIL-1,   // interrupted system call
	MHnoName = MSGH_FAIL-2,  // no more name registration
	MHexist = MSGH_FAIL-3,  // already exist
	MHnoMem = MSGH_FAIL-4,  // no enough data space
	MHagain = MSGH_FAIL-5,  // try again
	MHnoSpc = MSGH_FAIL-6,  // no more system resource
	MHnoCom = MSGH_FAIL-7,  // lose communication to MSGH process
	MHnoAth = MSGH_FAIL-8,  // not attach to MSGH subsystem yet
	MHnoMsg = MSGH_FAIL-9,  // no message
	MHidRm = MSGH_FAIL-10,  // queue was received from the system
	MH2Big = MSGH_FAIL-11,  // msgBody[] is greater than msgsz
	MHfault = MSGH_FAIL-12,  // point to an illegal address
	MHnoMat = MSGH_FAIL-13,  // no match
	MHbadQid = MSGH_FAIL-14,  // illegal mhqid
	MHbadName = MSGH_FAIL-15,  // non-registered name
	MHtimeOut = MSGH_FAIL-16,  // Timer expiration
	MHbadType = MSGH_FAIL-17,  // bad priority type
	MHbadSz = MSGH_FAIL-18,  // bad message size
	MHnoShm = MSGH_FAIL-19,  // can't access MSGH's shared memory
	MHnoQue = MSGH_FAIL-20,  // can't access MSGH's queue
	MHprocDead = MSGH_FAIL-21, // process is dead
	MHshmGetFail = MSGH_FAIL -22, // Unable to allocate large msg shm blk
	MHshmAttachFail = MSGH_FAIL -23, // Unable to attach to large msg shm blk
	MHlrgMsgNotReg = MSGH_FAIL -24, // Attempt to send large msg with MSGH without registering for large messages first
	MHlrgMsgBlkNotAvlbl = MSGH_FAIL -25,  // Could not find a shmem block with which the large msg could be sent
	MHshmDetachFail = MSGH_FAIL -26,  // Could not detach from the shm
	MHlrgMsgBufNotAlloc = MSGH_FAIL -27,  // Rcv'd a large Msg Sig. but the Large Msg Buf was free
	MHlrgMsgProcMismatch = MSGH_FAIL -29,  // Rcv'd a large Msg Sig. but the Large Msg Buf does not belong to this processs


	MHother = MSGH_FAIL-30,  // other reasons
	MHnoHost = MSGH_FAIL-31, // far host not accessible
	MHinvHostId = MSGH_FAIL-32, // Invalid host id
	MHnoSendBuf = MSGH_FAIL-33, // no send buffers in global buffer pool
	MHnoHostSendBuf = MSGH_FAIL-34, // no send buffers for a specific host
	MHnoLead = MSGH_FAIL-35,	// No lead CC
	MHwrongNode = MSGH_FAIL-36,	// Invalid operation for this node type
	MHnoQAssigned = MSGH_FAIL-37,	// No queue assigned to this global Q
	MHnotGlobalQ = MSGH_FAIL-38,	// global queue operation requested not on global queue
	MHglobalQ = MSGH_FAIL-39,	// non global queue operation for global queue
	MHgdNotReady = MSGH_FAIL-40,	// Global data object not ready
	MHgdFailedAudit = MSGH_FAIL-41, // Failed GDO audit
	MHnotDistQ = MSGH_FAIL-42, 	// dist queue operation requested not on dist queue
	MHbadVersion = MSGH_FAIL-43,    // library version does no match shared memory
	MHbadEnv = MSGH_FAIL-44,    	// Invalid cluster environment type
	MHenvMismatch = MSGH_FAIL-45,	// Requested prefered environment is not allowed

	MHresult_BOGUS_SUNPRO
};

#endif
