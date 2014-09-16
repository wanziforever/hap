
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <values.h>
#include "cc/hdr/init/INusrinit.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/msgh/MHlrgMsgExt.hh"
#include "cc/hdr/msgh/MHgd.hh"
#include "cc/hdr/msgh/MHregGd.hh"
#include "cc/hdr/msgh/MHgdUpd.hh"
#include "cc/hdr/msgh/MHrt.hh"
#include "cc/proc/msgh/MHgdAud.hh"

#ifdef PROC_UNDER_INIT
#define MHgdRegTimeOut		160000L	// Time out for GDO registration in ms. (160 sec)
#else
#define MHgdRegTimeOut		60000L	// Time out for GDO registration in ms. (60 sec)
#endif
#define MHbufFullTimeOut	60000000L  // Buffer full timeout in usec. (60 msec)
#define MHbufFullCnt		15

static struct timespec tsleep;

#define MHmaxDump       8120
 
void
MHgdDump(MHgdShm* m_p)
{
  char    buffer[MHmaxDump];
  memset(buffer, 0x0, sizeof(buffer));

  sprintf(buffer, "m_lockcnt %d, m_lastLockcnt %d, m_LastSync %lld, "
          "m_SyncAddress %lld\n", m_p->m_lockcnt, m_p->m_lastlockcnt,
          m_p->m_LastSync, m_p->m_SyncAddress);
  sprintf(&buffer[strlen(buffer)], "m_nLastSent %d, m_nNextOp %d, "
          "m_curBuf %d, m_headBuf %d, sync %d, srcQue %s\n",
          m_p->m_nLastSent, m_p->m_nNextOp, m_p->m_curBuf, m_p->m_headBuf,
          m_p->m_doSync, m_p->m_srcQue.display());
  sprintf(&buffer[strlen(buffer)], "nInv %ld, %ld %ld, nMv %ld, %ld %ld "
          "WaitOp %ld, WaitBuf %d, SFail %ld\nWaitMsgOp %ld, WaitMsgBuf %ld, "
          "NextMsg %ld, MsgSent %ld, StartToUpdate %ld\n",
          m_p->m_nInvalidates, m_p->m_InvalidateGig, m_p->m_InvalidateBytes,
          m_p->m_nMoves, m_p->m_MoveGig, m_p->m_MoveBytes,
          m_p->m_nWaitOp, m_p->m_nWaitBuf, m_p->m_SendFailed,
          m_p->m_nMsgWaitOp, m_p->m_nWaitMsgBuf, m_p->m_NextMsg,
          m_p->m_NextSent, m_p->m_StartToUpdate);

  int     i;
  for(i = 0; i < MHmaxHostReg; i++){
    if(!m_p->m_Hosts[i].m_bIsDistributed){
      continue;
    }
    sprintf(&buffer[strlen(buffer)],"hostid %d, m_lastAcked %d, m_nextMsg %d, "
            "m_noAcks %d, nOutOfSeq %d\n", i, m_p->m_Hosts[i].m_lastAcked,
            m_p->m_Hosts[i].m_nextMsg, m_p->m_Hosts[i].m_noAcks,
            m_p->m_Hosts[i].m_nOutOfSeq);
  }

  // Dump the contents of the send buffers
  int curBuf = m_p->m_curBuf;
  int  headBuf = m_p->m_headBuf;
  sprintf(&buffer[strlen(buffer)], "Num addr, type-gdid-size-seq-ackSeq\n");
  MHgdUpd*        pMsg;
  while(headBuf != curBuf){
    if(m_p->m_buf[headBuf] != MHempty){
      pMsg = (MHgdUpd*)&m_p->m_Data[m_p->m_buf[headBuf]];

      sprintf(&buffer[strlen(buffer)],"%d %d, %d-%d-%d-%d-%d\n", headBuf,
              m_p->m_buf[headBuf], pMsg->msgType, pMsg->m_GdId,
              pMsg->m_Size, pMsg->m_seq, pMsg->m_ackSeq);
    } else {
      sprintf(&buffer[strlen(buffer)], "buffer %d is empty\n", headBuf);
    }
    headBuf++;
    if(headBuf >= MHgdMaxBuf){
      headBuf = 0;
    }
  }

  //CRDEBUG_PRINT(0x1,("%s", buffer));
  printf("%s", buffer);
}

MHgd::MHgd()
{
	tsleep.tv_sec = 0;
	tsleep.tv_nsec = MHbufFullTimeOut;
	m_p = NULL;
	m_prt = NULL;
	m_shmid = -1;
	m_audthread = MAXINT;
}

GLretVal MHgd::attach(const char* name, Bool bCreate, Bool& isNew,
                      int permissions, LongLong size, char* &pAttached,
                      Long bufferSz, void* atAddress, const MHgdDist dist,
                      Long maxUpdSz, Long msgBufSz, Bool doDelaySend) {
	GLretVal	retval;
	MHqid		myqid;
	
	if(m_p != NULL){
		if(m_p->m_bufferSz != bufferSz || m_p->m_Size != size || m_p->m_msgBufSz != msgBufSz){
			return(MHnoMat);
		}
		pAttached = m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz;
		return(MHexist);
	}

	if(size < 0 || size > MHgdMaxSz){
		return(MHbadSz);
	}
	
	if(maxUpdSz >= MHlrgMsgBlkSize - MHgdUpdHead){
		return(MHbadSz);
	}

	if(bufferSz < 3 * maxUpdSz){
		bufferSz = 3 * maxUpdSz;
	}

	if(msgBufSz != 0 && msgBufSz < 2 * maxUpdSz){
		msgBufSz = 2 * maxUpdSz;
	}
	
	if((m_prt = MHmsgh.rt) == NULL){
		return(MHnoAth);
	}

	// If trying to attach on a wrong node, fail
	// For CSN if no CCs are configured act like we are on a CC
  if((dist == MHGD_CCONLY) &&
     (!m_prt->isCC(m_prt->hostlist[m_prt->LocalHostIndex].hostname.display()))){
		return(MHwrongNode);
	}

	// Get the qid of the lead MSGH process
	Short	leadcc = MHmsgh.getLeadCC();
	if(leadcc == MHempty){
		//No lead present, fail attach
		return(MHnoLead);
	};

	// Get a temporary qid to receive reply from MHGDPROC to
	// allow the attach from non-main thread
	// Because regName also can call attach, check for queue
	// existence to avoid infinite recursion
	// Only remove the name if this call created it
	char	tmpQName[MHmaxNameLen + 1];
	Bool	gotQueue = FALSE;

	sprintf(tmpQName, "GDO%d", MHmsgh.pid);
	if((retval = MHmsgh.getMhqid(tmpQName, myqid)) != GLsuccess){
		if((retval = MHmsgh.regName(tmpQName, myqid, TRUE, FALSE,
                                FALSE, MH_LOCAL)) != GLsuccess){
			return(retval);
		} 
		gotQueue = TRUE;
	}

	// Send the message to the lead MSGH and wait for response
	// When the response is received, attach to GDO and initialize it if
	// the create flag was true.
	// If GDO is created on the fly (outside of initialization), a blank
	// copy will be created on all nodes and it should be initialized 
	// while the main owner is initializing it.
	
	MHregGd	msg;
	msg.m_Name = name;
	msg.m_Size = size;
	msg.m_Permissions = permissions;
	msg.m_Uid = getuid();
	msg.m_Dist = dist;
	msg.m_bReplicate = TRUE;
	msg.m_bufferSz = bufferSz;
	msg.m_msgBufSz = msgBufSz;
	msg.m_doDelaySend = doDelaySend;
	msg.m_bCreate = bCreate;
	msg.m_maxUpdSz = maxUpdSz;
	msg.m_shmkey = MHempty;
	
	// Send the registration message
	if((retval = msg.send(MHMAKEMHQID(leadcc,MHmsghQ), myqid,
                        sizeof(MHregGd), 0L)) != GLsuccess){
		if(gotQueue){
			MHmsgh.rmName(myqid, tmpQName, TRUE);
		}
		return(retval);
	}
	
	union {
		LongLong	align;
		char 		msgbfr[MHmsgSz];
	};
	Long	msgSz = MHmsgSz;
	int	count = 0;

  MHattachAgain:
	// Receive only MHregPtyp to insure that only MSGH messages are retrieved
	if((retval = MHmsgh.receive(myqid, msgbfr, msgSz,
                              MHregPtyp, MHgdRegTimeOut  )) != GLsuccess){
		count ++;
		if(retval == MHintr && count < 10){
			goto MHattachAgain;
		}
		if(gotQueue){
			MHmsgh.rmName(myqid, tmpQName, TRUE);
		}
		return(retval);
	}

	if(gotQueue){
		MHmsgh.rmName(myqid, tmpQName, TRUE);
	}

	MHregGdAck*	ack;
	ack = (MHregGdAck *)msgbfr;
	if(ack->msgType != MHregGdAckTyp || sizeof(MHregGdAck) != msgSz){
		return(MHnoCom);
	}
	
	if(ack->m_RetVal != GLsuccess){
		return(ack->m_RetVal);
	}
	
	// Attach GDO shared memory. The data address will not be exactly
	// the requested address because of GDO data space, however the only
	// reason to specify address is to insure that the address is the same
	// on all machines, the precise address is not significant.  In
	// any case this should be strongly discouraged.

	if((m_p = (MHgdShm*)shmat(ack->m_Shmid, atAddress,
                            permissions)) == (MHgdShm*)-1){
		return(MHshmAttachFail);
	}

	isNew = ack->m_bIsNew;
	pAttached = m_p->m_Data + bufferSz + msgBufSz;

	return(GLsuccess);
}

void
MHgd::invalidate(const void* pStartAddress, Long length)
{
	invalidateCmpInt(pStartAddress, length, 0, MHnullQ);
}

void
MHgd::invalidateCmpInt(const void* pStartAddress, Long length,
                       int cmpOffset, MHqid notifyQid) {
	if(m_p == NULL){
#ifdef PROC_UNDER_INIT
		INITREQ(SN_LV0, MHnoEnt, "Uninitialized GDO", IN_ABORT);
#else
		//CRERROR("Uninitialized GDO");
    printf("Uninitialized GDO");
#endif
		return;
	}
	if(m_p->m_Dist == MHGD_NONE){
		return;
	}
	if(!m_p->m_doSync){
		return;
	}

	// Even if length exceeds a size of a single message, MSGH already needs
	// to support large segmented messages, this code relies on messaging
	// code to handle the segmentation, there will be no explicit
	// segmentation here
	if(length <= 0 || length > m_p->m_maxUpdSz - MHgdUpdHead){
		//CRERROR("%s GDO wrong length %d", m_p->m_Name, length);
    printf("%s GDO wrong length %d", m_p->m_Name, length);
#ifdef PROC_UNDER_INIT
		INITREQ(SN_LV0, MHbadSz, "Bad Invalidate Size", IN_ABORT);
#else
		abort();
#endif
		return;
	}
	// do the address sanity check
	if(((char*)pStartAddress < m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz) ||
	   ((char*)pStartAddress + length > m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz + m_p->m_Size) ||
	   ((char*)pStartAddress + cmpOffset > m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz + m_p->m_Size)){
		//CRERROR("%s bad invalidate address 0x%x, length %ld, cmpOffset %d, attached at 0x%x",
    //        m_p->m_Name, pStartAddress, length, cmpOffset, m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz);
    printf("%s bad invalidate address 0x%x, length %ld, cmpOffset %d, attached at 0x%x",
           m_p->m_Name, pStartAddress, length, cmpOffset, m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz);
#ifdef PROC_UNDER_INIT
		INITREQ(SN_LV0, MHbadSz, "Bad Invalidate Address", IN_ABORT);
#else
		abort();
#endif
	}

	// May add code here later to send data out immediately if more
	// than 2K of updates are queued, right now will just rely on
	// polling code
	int count = 0;
	int	nextOp;

	mutex_lock(&m_p->m_lock);
	m_p->m_lockcnt++;
	while(count < MHbufFullCnt){
		if (m_p->m_nNextOp >= m_p->m_nLastSent) {
			if (m_p->m_nNextOp - m_p->m_nLastSent < MHgdMaxOps - 2) {
				nextOp = m_p->m_nNextOp;
				m_p->m_nNextOp ++; 
				if (m_p->m_nNextOp >= MHgdMaxOps) {
					m_p->m_nNextOp = 0;
				}
				break;
			}
		} else if(m_p->m_nLastSent - m_p->m_nNextOp > 2){
			nextOp = m_p->m_nNextOp;
			m_p->m_nNextOp ++; 
			break;
		}

		m_p->m_nWaitOp++;
		mutex_unlock(&m_p->m_lock);
		count++;
		// Wait for a while and see if buffers will free up
		nanosleep(&tsleep, NULL);
		mutex_lock(&m_p->m_lock);
		m_p->m_lockcnt++;
	}


	if(count == MHbufFullCnt) {
		mutex_unlock(&m_p->m_lock);
		//CRERROR("invalidate(%s): Operation buffers full", m_p->m_Name);
    printf("invalidate(%s): Operation buffers full", m_p->m_Name);
		MHgdDump(m_p);
		return;	
	}

	if(notifyQid == MHnullQ){
		m_p->m_Ops[nextOp].m_type = MHGD_INVALIDATE;
		m_p->m_Ops[nextOp].m_invalidate.m_start = (char*)pStartAddress - m_p->m_Data;
		m_p->m_Ops[nextOp].m_invalidate.m_length = length;
	} else {
		m_p->m_Ops[nextOp].m_type = MHGD_INVCMP;
		m_p->m_Ops[nextOp].m_invcmp.m_start = (char*)pStartAddress - m_p->m_Data;
		m_p->m_Ops[nextOp].m_invcmp.m_length = length;
		m_p->m_Ops[nextOp].m_invcmp.m_offset = cmpOffset;
		m_p->m_Ops[nextOp].m_invcmp.m_qid = notifyQid<<0;
	}
	mutex_unlock(&m_p->m_lock);
}

// This function expects that the m_lock mutex is locked
void
MHgd::getUpdBuf(MHgdUpd*& msgp, Bool bWait)
{
	// Save last address used
	Long 	last_address;
	last_address = m_p->m_buf[m_p->m_curBuf] + msgp->m_Size;
	last_address = (last_address + sizeof(LongLong)) & MHlongAlign;
	if(last_address + m_p->m_maxUpdSz >= m_p->m_bufferSz){
		last_address = 0;
	}

  if((bWait == FALSE) && (m_p->m_headBuf != m_p->m_curBuf) &&
     (last_address <= m_p->m_buf[m_p->m_headBuf]) &&
     ((last_address + m_p->m_maxUpdSz) >= m_p->m_buf[m_p->m_headBuf])) {
    msgp = NULL;
    return;
  }


	int count = 0;
	int curBuf = m_p->m_curBuf;
	while(bWait && (m_p->m_headBuf != m_p->m_curBuf) &&
        (last_address <= m_p->m_buf[m_p->m_headBuf]) &&
        ((last_address + m_p->m_maxUpdSz) >= m_p->m_buf[m_p->m_headBuf]) &&
        (count < MHbufFullCnt)) {
		count++;
		m_p->m_nWaitBuf++;
		mutex_unlock(&m_p->m_lock);
		nanosleep(&tsleep, NULL);
		mutex_lock(&m_p->m_lock);
		m_p->m_lockcnt++;
		// Because mutex was released while waiting, must
		// recalculate last_address, and check if buffer already allocated
		if(curBuf != m_p->m_curBuf){
			msgp = (MHgdUpd*)(&m_p->m_Data[m_p->m_buf[m_p->m_curBuf]]);
			if(msgp->m_Size == sizeof(MHgdUpd)){
				return;
			}
		}
		last_address = m_p->m_buf[m_p->m_curBuf] + msgp->m_Size;
		last_address = (last_address + sizeof(LongLong)) & MHlongAlign;
		if(last_address + m_p->m_maxUpdSz >= m_p->m_bufferSz){
			last_address = 0;
		}
	}
	if(count == MHbufFullCnt){
		//CRERROR("%s out of buffer space", m_p->m_Name);
    printf("%s out of buffer space", m_p->m_Name);
		msgp = NULL;
		return;
	}
	// Get another buffer
	m_p->m_curBuf++;
	if(m_p->m_curBuf >= MHgdMaxBuf){
		m_p->m_curBuf = 0;
	}
	if(m_p->m_buf[m_p->m_curBuf] != -1){
		// No more free buffers, what to do here??
		//CRERROR("%s Out of GDO buffers", m_p->m_Name);
    printf("%s Out of GDO buffers", m_p->m_Name);
		msgp = NULL;
		return;
	}
	m_p->m_buf[m_p->m_curBuf] = last_address;
	msgp = (MHgdUpd*)(&m_p->m_Data[m_p->m_buf[m_p->m_curBuf]]);
	msgp->priType = MHintPtyp;
	msgp->msgType = MHgdUpdTyp;
	msgp->m_GdId = m_p->m_GdId;
	msgp->m_seq = m_p->m_curBuf;
	msgp->m_Size = sizeof(MHgdUpd);
	return;
}

void
MHgd::invalidateSeqInt(const void* pStartAddress, Long length, Bool bLocked)
{
	invalidateSeqCmpInt(pStartAddress, length, 0, -1, bLocked);
}

void
MHgd::invalidateSeqCmpInt(const void* pStartAddress, Long length, int cmpOffset,
                          int notifyQid, Bool bLocked) {
	if(m_p == NULL){
#ifdef PROC_UNDER_INIT
		INITREQ(SN_LV0, MHnoEnt, "Uninitialized GDO", IN_ABORT);
#else
		//CRERROR("Uninitialized GDO");
    printf("Uninitialized GDO");
#endif
		return;
	}

	if(m_p->m_Dist == MHGD_NONE){
		return;
	}

	if(!m_p->m_doSync){
		return;
	}
	if((length < 0) || length > m_p->m_maxUpdSz - MHgdUpdHead){
		//CRERROR("%s GDO wrong length %d", m_p->m_Name, length);
    printf("%s GDO wrong length %d", m_p->m_Name, length);
#ifdef PROC_UNDER_INIT
		INITREQ(SN_LV0, MHbadSz, "Bad InvalidateSeq Size", IN_ABORT);
#endif
		return;
	}
	// do the address sanity check
	if(((char*)pStartAddress < m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz) ||
	   ((char*)pStartAddress + length > m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz + m_p->m_Size) ||
	   ((char*)pStartAddress + cmpOffset > m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz + m_p->m_Size)){
		//CRERROR("%s bad invalidate address 0x%x, length %ld, offset %d, attached at 0x%x",
    //        m_p->m_Name, pStartAddress, length, cmpOffset, m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz);
    printf("%s bad invalidate address 0x%x, length %ld, offset %d, attached at 0x%x",
            m_p->m_Name, pStartAddress, length, cmpOffset, m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz);
#ifdef PROC_UNDER_INIT
		INITREQ(SN_LV0, MHbadSz, "Bad Invalidate Address", IN_ABORT);
#else
		abort();
#endif
	}

	// Get the pointer to current message, if message full
	// allocate another message. Never allocate message buffer
	// < maxUpdSz
	if(!bLocked){
		mutex_lock(&m_p->m_lock);
		m_p->m_lockcnt++;
	}
	MHgdUpd* 	msgp;
	msgp = (MHgdUpd*)(&m_p->m_Data[m_p->m_buf[m_p->m_curBuf]]);
	if(msgp->m_Size + sizeof(Long) + sizeof(MHgdOp) + length >= m_p->m_maxUpdSz){
		getUpdBuf(msgp);
		if(msgp == NULL){
			if(!bLocked){
				mutex_unlock(&m_p->m_lock);
			}
			return;
		}
	} 
	// Align the current address and copy the data
	if((msgp->m_Size & MHlongAlign) != 0){
		msgp->m_Size = (msgp->m_Size + sizeof(LongLong)) & MHlongAlign;
	}
	MHgdOp* opp;
	opp = (MHgdOp*)((char*)msgp + msgp->m_Size);
	if(notifyQid == -1){
		opp->m_type = MHGD_INVALIDATE;
		opp->m_invalidate.m_start = (char*)pStartAddress - m_p->m_Data;
		opp->m_invalidate.m_length = length;
	} else {
		opp->m_type = MHGD_INVCMP;
		opp->m_invcmp.m_start = (char*)pStartAddress - m_p->m_Data;
		opp->m_invcmp.m_length = length;
		opp->m_invcmp.m_qid = notifyQid;
		opp->m_invcmp.m_offset = cmpOffset;
	}
	msgp->m_Size += sizeof(MHgdOp);
	memcpy((char*)msgp + msgp->m_Size, (char*)pStartAddress, length);
	msgp->m_Size += length;
	m_p->m_nInvalidates++;
	m_p->m_InvalidateBytes += sizeof(MHgdOp) + length;
	if(m_p->m_InvalidateBytes & 0xc0000000){
		m_p->m_InvalidateGig += (m_p->m_InvalidateBytes >> 30);
		m_p->m_InvalidateBytes &= 0x3fffffff;
	}
	if(!bLocked){
		mutex_unlock(&m_p->m_lock);
	}
}

#ifdef NOTIMP
void
MHgd::atomicUpdate(MHgdOp* ops)
{
	if(m_p == NULL){
#ifdef PROC_UNDER_INIT
		INITREQ(SN_LV0, MHnoEnt, "Uninitialized GDO", IN_ABORT);
#else
		//CRERROR("Uninitialized GDO");
    printf("Uninitialized GDO");
#endif
		return;
	}

	if(m_p->m_Dist == MHGD_NONE){
		return;
	}
	if(!m_p->m_doSync){
		return;
	}
	MHgdUpd* 	msgp;
	mutex_lock(&m_p->m_lock);
	m_p->m_lockcnt++;
	msgp = (MHgdUpd*)(&m_p->m_Data[m_p->m_buf[m_p->m_curBuf]]);
	int	i = 0;
	Long	tot_size = 0;
	while(ops[i].m_type != MHGD_EMPTY){
		if((tot_size & MHlongAlign) != 0){
			tot_size = (tot_size + sizeof(LongLong)) & MHlongAlign;
		}
		switch(ops[i].m_type){
		case MHGD_INVALIDATE:
			tot_size += ops[i].m_invalidateExt.m_length + sizeof(MHgdOp);
			break;
		case MHGD_MEMMOVE:
			tot_size += sizeof(MHgdOp);
			break;
		default:
			mutex_unlock(&m_p->m_lock);
			//CRERROR("Invalid op type %d", ops[i].m_type);
      printf("Invalid op type %d", ops[i].m_type);
			return;
		}
		i++;
	}
	if(tot_size > m_p->m_maxUpdSz - MHgdUpdHead){
		mutex_unlock(&m_p->m_lock);
		//CRERROR("total update size too large %d", tot_size);
    printf("total update size too large %d", tot_size);
		return;
	}
	if(tot_size + msgp->m_Size + sizeof(Long) >= m_p->m_maxUpdSz){
		getUpdBuf(msgp);
		if(msgp == NULL){
			mutex_unlock(&m_p->m_lock);
			return;
		}
	}
	
	i = 0;
	while(ops[i].m_type != MHGD_EMPTY){
		switch(ops[i].m_type){
		case MHGD_INVALIDATE:
			invalidateSeqInt(ops[i].m_invalidateExt.m_start, ops[i].m_invalidateExt.m_length, TRUE);
			break;
		case MHGD_MEMMOVE:
			memMoveSeqInt(ops[i].m_memmoveExt.m_to, ops[i].m_memmoveExt.m_from,
                    ops[i].m_memmoveExt.m_length, TRUE);
			break;
		default:
			//CRERROR("Invalid op type %d", ops[i].m_type);
      printf("Invalid op type %d", ops[i].m_type);
			break;
		}
		i++;
	}
	mutex_unlock(&m_p->m_lock);
}
#endif

void
MHgd::memMoveSeqInt(const void* to, const void* from, Long length, Bool bLocked)
{
	if(m_p == NULL){
#ifdef PROC_UNDER_INIT
		INITREQ(SN_LV0, MHnoEnt, "Uninitialized GDO", IN_ABORT);
#else
		//CRERROR("Uninitialized GDO");
    printf("Uninitialized GDO");
#endif
		return;
	}
	if(m_p->m_Dist == MHGD_NONE){
		return;
	}

	if(!m_p->m_doSync){
		return;
	}
	// do the address sanity check
	if(((char*)to < m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz) ||
	   ((char*)to + length > m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz + m_p->m_Size) || 
	   ((char*)from < m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz) ||
	   ((char*)from + length > m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz + m_p->m_Size)){
		//CRERROR("%s bad memmove from 0x%x, to 0x%x, length %ld, attached at 0x%x",
    //        m_p->m_Name, from, to, length, m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz);
    printf("%s bad memmove from 0x%x, to 0x%x, length %ld, attached at 0x%x",
            m_p->m_Name, from, to, length, m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz);
#ifdef PROC_UNDER_INIT
		INITREQ(SN_LV0, MHbadSz, "Bad Invalidate Address", IN_ABORT);
#else
		abort();
#endif
	}


	// Get the pointer to current message, if message full
	// allocate another message. Never allocate message buffer
	// < m_maxUpdSz
	if(!bLocked){
		mutex_lock(&m_p->m_lock);
		m_p->m_lockcnt++;
	}
	MHgdUpd* 	msgp;
	msgp = (MHgdUpd*)(&m_p->m_Data[m_p->m_buf[m_p->m_curBuf]]);
	if(msgp->m_Size + sizeof(Long) + sizeof(MHgdOp) >= m_p->m_maxUpdSz){
		getUpdBuf(msgp);
		if(msgp == NULL){
			if(!bLocked){
				mutex_unlock(&m_p->m_lock);
			}
			return;
		}
	} 
	// Align the current address 
	if((msgp->m_Size & MHlongAlign) != 0){
		msgp->m_Size = (msgp->m_Size + sizeof(LongLong)) & MHlongAlign;
	}
	MHgdOp* opp;
	opp = (MHgdOp*)((char*)msgp + msgp->m_Size);
	opp->m_type = MHGD_MEMMOVE;
	opp->m_memmove.m_to = (char*)to - m_p->m_Data;
	opp->m_memmove.m_from = (char*)from - m_p->m_Data;
	opp->m_memmove.m_length = length;
	msgp->m_Size += sizeof(MHgdOp);
	m_p->m_nMoves++;
	m_p->m_MoveBytes += length;
	if(m_p->m_MoveBytes & 0xc0000000){
		m_p->m_MoveGig += (m_p->m_MoveBytes >> 30);
		m_p->m_MoveBytes &= 0x3fffffff;
	}
	if(!bLocked){
		mutex_unlock(&m_p->m_lock);
	}
}

void
MHgd::memMove(const void* to, const void* from, Long length)
{
	if(m_p == NULL){
#ifdef PROC_UNDER_INIT
		INITREQ(SN_LV0, MHnoEnt, "Uninitialized GDO", IN_ABORT);
#else
		//CRERROR("Uninitialized GDO");
    printf("Uninitialized GDO");
#endif
		return;
	}
	if(m_p->m_Dist == MHGD_NONE){
		return;
	}

	if(!m_p->m_doSync){
		return;
	}

	// do the address sanity check
	if(((char*)to < m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz) ||
	   ((char*)to + length > m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz + m_p->m_Size) || 
	   ((char*)from < m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz) ||
	   ((char*)from + length > m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz + m_p->m_Size)){
		//CRERROR("%s bad memmove from 0x%x, to 0x%x, length %ld, attached at 0x%x",
    //        m_p->m_Name, from, to, length, m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz);
    printf("%s bad memmove from 0x%x, to 0x%x, length %ld, attached at 0x%x",
            m_p->m_Name, from, to, length, m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz);
#ifdef PROC_UNDER_INIT
		INITREQ(SN_LV0, MHbadSz, "Bad Invalidate Address", IN_ABORT);
#else
		abort();
#endif
	}

	int 	count = 0;
	int	nextOp;

	mutex_lock(&m_p->m_lock);
	m_p->m_lockcnt++;
	while(count < MHbufFullCnt){
		if(m_p->m_nNextOp >= m_p->m_nLastSent){
			if(m_p->m_nNextOp - m_p->m_nLastSent < MHgdMaxOps - 2){
				nextOp = m_p->m_nNextOp;
				m_p->m_nNextOp ++; 
				if(m_p->m_nNextOp >= MHgdMaxOps){
					m_p->m_nNextOp = 0;
				}
				break;
			}
		} else if(m_p->m_nLastSent - m_p->m_nNextOp > 2){
			nextOp = m_p->m_nNextOp;
			m_p->m_nNextOp ++; 
			break;
		}

		m_p->m_nWaitOp++;
		mutex_unlock(&m_p->m_lock);
		count++;
		// Wait for a while and see if buffers will free up
		nanosleep(&tsleep, NULL);
		mutex_lock(&m_p->m_lock);
		m_p->m_lockcnt++;
	}

	if(count == MHbufFullCnt){
		mutex_unlock(&m_p->m_lock);
		//CRERROR("invalidate(%s): Operation buffers full", m_p->m_Name);
    printf("invalidate(%s): Operation buffers full", m_p->m_Name);
		MHgdDump(m_p);
		return;	
	}

	m_p->m_Ops[nextOp].m_type = MHGD_MEMMOVE;
	m_p->m_Ops[nextOp].m_memmove.m_to = (char*)to - (char*)&m_p->m_Data;
	m_p->m_Ops[nextOp].m_memmove.m_from = (char*)from - m_p->m_Data;
	m_p->m_Ops[nextOp].m_memmove.m_length = length;
	mutex_unlock(&m_p->m_lock);
}

void
MHgd::send(MHqid qid, char* msgp, Long size)
{
	GLretVal	retval;

	if(m_p == NULL){
#ifdef PROC_UNDER_INIT
		INITREQ(SN_LV0, MHnoEnt, "Uninitialized GDO", IN_ABORT);
#else
		//CRERROR("Uninitialized GDO");
    printf("Uninitialized GDO");
#endif
		return;
	}

	if(size > m_p->m_maxUpdSz || m_p->m_msgBufSz == 0){
		abort();
	}

	if(!m_p->m_doSync || !m_p->m_doDelaySend){
		// Send the message right away
		if((retval = MHmsgh.send(qid, msgp, size, 0L)) != GLsuccess){
			//CRDEBUG(CRmsgh+4,("Failed to send message to qid %s, retval = %d",
      //                  qid.display(), retval));
      printf("Failed to send message to qid %s, retval = %d",
                        qid.display(), retval);
		}
		return;
	}

	int 	count = 0;
	int	nextMsg;
	int	mindex = -1;

	mutex_lock(&m_p->m_lock);
	m_p->m_lockcnt++;
  // If there are no pending unsynched invalidates, just send the message
  // right away.
  if(m_p->m_nLastSent == m_p->m_nNextOp){
    if(m_p->m_headBuf == m_p->m_curBuf){
      mutex_unlock(&m_p->m_lock);
      // Send the message
      if((retval = MHmsgh.send(qid, msgp, size, 0L)) != GLsuccess){
        //CRDEBUG(CRmsgh+4,("Failed to send message to qid %s, retval = %d",
        //                  qid.display(), retval));
        printf("Failed to send message to qid %s, retval = %d",
                          qid.display(), retval);
      }
      return;
    }
    // Calculate index right away
    mindex = m_p->m_curBuf;
  }

	while(count < MHbufFullCnt){
		if(m_p->m_NextMsg >= m_p->m_NextSent){
			if(m_p->m_NextMsg - m_p->m_NextSent < MHgdMaxSend - 2){
				nextMsg = m_p->m_NextMsg;
				m_p->m_NextMsg ++; 
				if(m_p->m_NextMsg >= MHgdMaxSend){
					m_p->m_NextMsg = 0;
				}
				break;
			}
		} else if(m_p->m_NextSent - m_p->m_NextMsg > 2){
			nextMsg = m_p->m_NextMsg;
			m_p->m_NextMsg ++; 
			break;
		}

		m_p->m_nMsgWaitOp++;
		mutex_unlock(&m_p->m_lock);
		count++;
		// Wait for a while and see if buffers will free up
		nanosleep(&tsleep, NULL);
		mutex_lock(&m_p->m_lock);
		m_p->m_lockcnt++;
	}


	if(count == MHbufFullCnt){
		mutex_unlock(&m_p->m_lock);
		//CRERROR("send: Message buffers full");
    printf("send: Message buffers full");
		return;	
	}

	Long 	last_address;
	int	lastMsg = nextMsg - 1;

	if(lastMsg < 0){
		lastMsg = MHgdMaxSend - 1;
	}

	last_address = m_p->m_delSend[lastMsg].m_msgp + 
     m_p->m_delSend[lastMsg].m_size;
	last_address = (last_address + sizeof(LongLong)) & MHlongAlign;
	if(last_address + size >= m_p->m_msgBufSz){
		last_address = 0;
	}

	count = 0;
	while((m_p->m_NextMsg != m_p->m_NextSent) &&
        (last_address <= m_p->m_delSend[m_p->m_NextSent].m_msgp) &&
        ((last_address + size) >= m_p->m_delSend[m_p->m_NextSent].m_msgp) &&
        (count < MHbufFullCnt)){
		count++;
		m_p->m_nWaitMsgBuf++;
		mutex_unlock(&m_p->m_lock);
		nanosleep(&tsleep, NULL);
		mutex_lock(&m_p->m_lock);
		m_p->m_lockcnt++;
	}
	if(count == MHbufFullCnt){
		//CRERROR("%s out of message buffer space", m_p->m_Name);
    printf("%s out of message buffer space", m_p->m_Name);
		mutex_unlock(&m_p->m_lock);
		return;
	}

  // Put the same index in all the other outstanding messages
  while(mindex != -1 && m_p->m_StartToUpdate != m_p->m_NextMsg){
    m_p->m_delSend[m_p->m_StartToUpdate].m_msgindx = mindex;
    m_p->m_StartToUpdate++;
    if(m_p->m_StartToUpdate >= MHgdMaxSend){
      m_p->m_StartToUpdate = 0;
    }
  }

	m_p->m_delSend[nextMsg].m_msgp = last_address;
	m_p->m_delSend[nextMsg].m_size = size;
	m_p->m_delSend[nextMsg].m_qid = qid;
	m_p->m_delSend[nextMsg].m_msgindx = mindex;
	m_p->m_delSend[nextMsg].m_opNum = m_p->m_nNextOp - 1;
	if(m_p->m_delSend[nextMsg].m_opNum < 0){
		m_p->m_delSend[nextMsg].m_opNum = 0;
	}
	memcpy(&m_p->m_Data[m_p->m_bufferSz + last_address], msgp, size);
	mutex_unlock(&m_p->m_lock);

}

GLretVal
MHgd::detach()
{
	if(m_audthread != MAXINT){
		// kill audit thread
		thr_kill(m_audthread, SIGKILL);
		m_audthread = MAXINT;
	}
	char* ptmp = (char*)m_p;
	m_p = NULL;
	m_prt = NULL;
	m_shmid = -1;
	shmdt(ptmp);
	return(GLsuccess);
}

GLretVal
MHgd::remove(const char* name)
{
	GLretVal	retval;

	MHqid myqid;
	// Find my queue id
	if((retval = MHmsgh.getMyQid(myqid)) != GLsuccess){
		return(retval);
	}

	// Get the qid of the lead MSGH process
	Short	leadcc = MHmsgh.getLeadCC();
	if(leadcc == MHempty){
		//No lead present, fail remove
		return(MHnoLead);
	};

	MHrmGd	msg;
	msg.m_Name = name;

	// Send the remove message
	if((retval = msg.send(MHMAKEMHQID(leadcc,MHmsghQ),
                        myqid, sizeof(MHrmGd), 0L)) != GLsuccess){
		return(retval);
	}
	
	union {
		Long	align;
		char 	msgbfr[MHmsgSz];
	};
	Long	msgSz = MHmsgSz;
	int	count = 0;

  MHdelAgain:
	// Receive only MHregPtyp to insure that only MSGH messages are retrieved
	if((retval = MHmsgh.receive(myqid, msgbfr, msgSz,
                              MHregPtyp, MHgdRegTimeOut  )) != GLsuccess){
		count ++;
		if(retval == MHintr && count < 10){
			goto MHdelAgain;
		}
		return(retval);
	}

	MHrmGdAck*	ack;
	ack = (MHrmGdAck *)msgbfr;
	if(ack->msgType != MHrmGdAckTyp || sizeof(MHrmGdAck) != msgSz){
		return(MHnoCom);
	}
	
	if(ack->m_RetVal != GLsuccess){
		return(ack->m_RetVal);
	}

	return(GLsuccess);
}

Void
MHgd::setSync(Bool doSync)
{
	if(m_p == NULL){
#ifdef PROC_UNDER_INIT
		INITREQ(SN_LV0, MHnoEnt, "Uninitialized GDO", IN_ABORT);
#else
		//CRERROR("Uninitialized GDO");
    printf("Uninitialized GDO");
#endif
		return;
	}
	m_p->m_doSync = doSync;
}

Void
MHgd::syncAll(U_long* progress)
{
	syncAllExt(progress, FALSE);
}

Void
MHgd::syncAllExt(U_long* progress, Bool readAllMsgs)
{
	if(m_p == NULL){
#ifdef PROC_UNDER_INIT
		INITREQ(SN_LV0, MHnoEnt, "Uninitialized GDO", IN_ABORT);
#else
		//CRERROR("Uninitialized GDO");
    printf("Uninitialized GDO");
#endif
		return;
	}
	if(m_p->m_Dist == MHGD_NONE){
		return;
	}
	char		message[80];
	MHqid		myqid;
	GLretVal	retval;
	Long		count = 0;

	m_p->m_doSync = TRUE;
	// Find my queue id
	if((retval = MHmsgh.getMyQid(myqid)) != GLsuccess){
#ifdef PROC_UNDER_INIT
		INITREQ(SN_LV0, MHnoQue, "No process queue", IN_ABORT);
#else
		//CRERROR("No process queue");
    printf("No process queue");
#endif
		return;
	}
	
	LongLong buffer[(MHmsgLimit/sizeof(LongLong)) + 1];
	short	msgSz;
	MHgdSyncReq	syncReqMsg;
	MHgdSyncReq*	psyncReq;
	syncReqMsg.m_GdId = m_p->m_GdId;

	Long	msgPriority;
	if(readAllMsgs == TRUE){
		msgPriority = 0L;
	} else {
		msgPriority = MHintPtyp;
	}
	// Do all nodes that have a need to sync this data object
	for(int i = 0; i < MHmaxHostReg; i++){
		if(i == m_prt->getLocalHostIndex() || !m_p->m_Hosts[i].m_bIsDistributed){
			continue;
		}
		syncReqMsg.m_SyncAddress = 0;
		syncReqMsg.srcQue = MHMAKEMHQID(i, MHrprocQ + \
                                    m_prt->m_gdCtl[m_p->m_GdId].m_RprocIndex);
		syncReq(&syncReqMsg, myqid);
		while(1){
			msgSz = MHmsgSz;
			if(m_prt->hostlist[i].isactive == FALSE){
				break;
			}
#ifdef PROC_UNDER_INIT
			IN_SANPEG();
#endif
			if((retval = MHmsgh.receive(myqid, (char*)buffer, msgSz,
                                  msgPriority, 1000L)) == GLsuccess){
				psyncReq = (MHgdSyncReq*)&buffer;
				if(psyncReq->msgType != MHgdSyncReqTyp && readAllMsgs == TRUE){
					continue;
				}
				if(psyncReq->msgType != MHgdSyncReqTyp || 
				   psyncReq->m_GdId != syncReqMsg.m_GdId){
					//CRERROR("Received invalid msg type 0x%x, or wrong gdo %d expected %d",
          //        psyncReq->msgType, psyncReq->m_GdId, syncReqMsg.m_GdId);
          printf("Received invalid msg type 0x%x, or wrong gdo %d expected %d",
                  psyncReq->msgType, psyncReq->m_GdId, syncReqMsg.m_GdId);
					continue;
				}
				syncReqMsg.m_SyncAddress = psyncReq->m_SyncAddress;
				count++;
				if((count & 0x1ff) == 0){
					sprintf(message,"GLOBAL DATA %s FOR HOST %s - %lld OF %lld",
                  m_p->m_Name, m_prt->hostlist[i].hostname.display(),
                  syncReqMsg.m_SyncAddress, m_p->m_Size);
					if(progress != NULL){
						(*progress)++;
#ifdef PROC_UNDER_INIT
						IN_STEP(*progress, message);
#endif
					}
#ifdef PROC_UNDER_INIT
					IN_PROGRESS(message);
#else
					//CRDEBUG_PRINT(0x1,("SYNCHING %s", message));
          printf("SYNCHING %s", message);
#endif
				}
			} else if(retval == MHtimeOut || retval == MHintr){
				// Timed out on request, if this is the start of sync
				// ask again, otherwise synching is occurring from
				// the other end and will be done eventually
				if(progress != NULL){
					(*progress)++;
#ifdef PROC_UNDER_INIT
					IN_STEP(*progress, message);
#endif
				}
				if(syncReqMsg.m_SyncAddress != 0){
					continue;
				}
			} else if(retval == MH2Big && readAllMsgs == TRUE){
				continue;
			} else {
				//CRERROR("Failed to sync %s, retval %d",m_p->m_Name, retval);
        printf("Failed to sync %s, retval %d",m_p->m_Name, retval);
				break;
			}
      if(syncReqMsg.m_SyncAddress <  m_p->m_Size &&
         syncReqMsg.m_SyncAddress >= 0){
				syncReq(&syncReqMsg, myqid);
			} else {
				// done with this host, do next one
				break;
			}
		}
	}
}

Void    
MHgd::syncReq(MHgdSyncReq* syncReq, MHqid myqid)  
{       
  GLretVal        retval;
  // if object already synched, throw this message away
  if(syncReq->m_SyncAddress >= m_p->m_Size || syncReq->m_SyncAddress < 0){
    //CRDEBUG(CRmsgh+2,("Object %s already synched syncAddress %ld >= size=%ld",       
    //                  m_p->m_Name, syncReq->m_SyncAddress, m_p->m_Size));
    printf("Object %s already synched syncAddress %ld >= size=%ld",       
           m_p->m_Name, syncReq->m_SyncAddress, m_p->m_Size);
    return;
  }
        
  // send the data
  Long nbytes = m_p->m_Size - syncReq->m_SyncAddress;
        
  if(nbytes > MHmaxDataSz){
    nbytes = MHmaxDataSz;
  }        
                
  LongLong            buffer[MHmsgLimit/sizeof(LongLong) + 1];
  MHgdSyncData*   msg = (MHgdSyncData*)buffer;
  msg->msgType = MHgdSyncDataTyp;
  msg->priType = MHintPtyp;
  msg->m_GdId = syncReq->m_GdId;
  msg->m_SyncAddress = syncReq->m_SyncAddress;
  msg->m_Length = nbytes;
  msg->srcQue = myqid;
  memcpy(msg->m_Data,
         &m_p->m_Data[syncReq->m_SyncAddress + m_p->m_bufferSz + m_p->m_msgBufSz],
         nbytes);      
  // Send unbufferred message to avoid multiple copies
  if((retval = MHmsgh.send(syncReq->srcQue, (char*)buffer, sizeof(MHgdSyncData) - 1
                           + nbytes,        
                           0L, FALSE)) != GLsuccess){
    //CRDEBUG(CRmsgh+2, ("Failed to send sync message retval %d", retval));
    printf("Failed to send sync message retval %d", retval);
  }
}

// This function does a simple checksum of GDO data.
// It also triggers audits on all the other nodes
// that this object is replicated to.
// This should always be run from the lead
// CEPs are also not allowed to run this
// any discrepancies will result in the reboot of
// the other nodes.  
// The audit can be run in report only or fix mode
// A subset of memory can be selected to allow routine audit
GLretVal
MHgd::audit(MHgdAudAction action,  const void* pStartAddress, Long alength)
{
#ifndef PROC_UNDER_INIT
	return(GLfail);
#else
	if(m_p->m_Dist == MHGD_NONE){
		return(GLsuccess);
	}
	// do the address sanity check
	if((pStartAddress != NULL) && 
	   (((char*)pStartAddress < m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz) ||
	    (((LongLong)pStartAddress & MHlongLeft) != 0) ||
	    (alength < 0) ||
      ((char*)pStartAddress + alength > m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz + m_p->m_Size))){
		//CRERROR("%s bad audit address 0x%x, length %ld, attached at 0x%x", m_p->m_Name, pStartAddress, alength, m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz);
    printf("%s bad audit address 0x%x, length %ld, attached at 0x%x", m_p->m_Name, pStartAddress, alength, m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz);
#ifdef PROC_UNDER_INIT
		INITREQ(SN_LV0, MHbadSz, "Bad Audit Address", IN_ABORT);
#else
		abort();
#endif
	}

	if(!MHmsgh.onLeadCC()){
		INITREQ(SN_LV0, MHwrongNode, "GDO AUDIT ON ACTIVE", IN_EXIT);
	}
	GLretVal 	retval;
	MHqid 		myqid;
	if((retval = MHmsgh.getMyQid(myqid)) != GLsuccess){
		INITREQ(SN_LV0, retval, "NO PROCESS QUEUE", IN_EXIT);
	}

	// For simplicity, compute checsum here first
	// and then run it on the other nodes
	// wait for them all to complete before returning
	// this function.   Timeout should allow 5Mb/sec. It is a little
	// slow but it is better to wait too long then too little
	register Long	sum = 0;
	Long*		p;
	Long*		endp;
	char*		pStart;
	LongLong	length;
	if(pStartAddress == NULL){
		pStart = m_p->m_Data + m_p->m_bufferSz + m_p->m_msgBufSz;
		length = m_p->m_Size;
	} else {
		pStart = (char*)pStartAddress;
		length = alength;
	}
	int		i;
	p = (Long*)pStart;
	endp = (Long*)(pStart + length);

	// Handle the case where the size is not a multiple of long
	int	bytesLeft = ((LongLong)endp) & MHlongLeft;
	endp = (Long*)(((LongLong)endp) & MHintAlign);

	while(p < endp){
		sum += (*p);
		p++;
	}
	char* p2 = (char *)p;
	while(bytesLeft > 0){
		sum += (*p2);
		p2 ++;
		bytesLeft --;
	}

	// Make sure that we have no updates pending before request
	// to audit the other hosts is made.

	for(i = 0; i < MHmaxHostReg; i++){
		if(!m_p->m_Hosts[i].m_bIsDistributed){
			continue;
		}
		if((m_p->m_nNextOp != m_p->m_nLastSent) || 
		   (m_p->m_curBuf != m_p->m_headBuf) ||
		   (((MHgdUpd*)(&m_p->m_Data[m_p->m_buf[m_p->m_curBuf]]))->m_Size != sizeof(MHgdUpd))){
			sleep(1);
			continue;
		}
	}

	MHaudGd	audmsg;
	audmsg.m_Name = m_p->m_Name;
	audmsg.m_Sum = sum;
	audmsg.m_GdId = m_p->m_GdId;
	audmsg.m_Action = action;
	audmsg.m_Start = pStart - m_p->m_Data;
	audmsg.m_Length = length;
	char	gotAcks[MHmaxHostReg];
	memset(gotAcks, 0x1, sizeof(gotAcks));
	int	nSent = 0;
	GLretVal result = GLsuccess;

	for(i = 0; i < MHmaxHostReg; i++){
		if(!m_p->m_Hosts[i].m_bIsDistributed ||
		   !m_prt->hostlist[i].isactive){
			continue;
		}
		if((retval = audmsg.send(MHMAKEMHQID(i, MHrprocQ + m_prt->m_gdCtl[m_p->m_GdId].m_RprocIndex), myqid, sizeof(MHaudGd), 0L)) == GLsuccess){
			gotAcks[i] = 0;
			nSent++;
		} else {
			// Do not audit that host
			//CRDEBUG_PRINT(0x1, ("Failed to send audit request for %s, host %d, retval %d", m_p->m_Name, i, retval));
      printf("Failed to send audit request for %s, host %d, retval %d", m_p->m_Name, i, retval);
		}
	}

	Long		timeout = ((m_p->m_Size/(5*1024*1024)) + 2) * 1000;
	Long		tcount = 0;
	
	long		buffer[MHmsgLimit/(sizeof(long)) + 1];
	Long		msgsz;
	MHaudGdAck*	ackmsg;
	while(nSent){
		msgsz = MHmsgLimit;
		if((retval = MHmsgh.receive(myqid, (char*)buffer, msgsz, MHregPtyp, timeout)) == GLsuccess){
			ackmsg = (MHaudGdAck*)buffer;
			if(ackmsg->msgType != MHaudGdAckTyp){
				//CRERROR("Unexpected message %d", ackmsg->msgType);
        printf("Unexpected message %d", ackmsg->msgType);
				continue;
			}
			if(strcmp(ackmsg->m_Name.display(), m_p->m_Name) != 0){
				//CRERROR("Ack for unexpected GDO got %s, expected %s", ackmsg->m_Name.display(), m_p->m_Name);
        printf("Ack for unexpected GDO got %s, expected %s", ackmsg->m_Name.display(), m_p->m_Name);
				continue;
			}
			if(result == GLsuccess){
				result = ackmsg->m_RetVal;
			}
			gotAcks[MHmsgh.Qid2Host(ackmsg->srcQue)] = 1;
		} else if(retval == MHintr){
			continue;
		} else if(retval == MHtimeOut){
			tcount++;
		}
		timeout = 3000;
		if(tcount > 3){
			// Give up waiting
			return(result);
		}
		// If all hosts done return
		for(i = 0; i < MHmaxHostReg; i++){
			if(gotAcks[i] == 0){
				break;
			}
		}
		if(i == MHmaxHostReg){
			return(result);
		}
	}
	return(result);
#endif
}
