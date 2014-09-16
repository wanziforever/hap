#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/times.h>
#include <limits.h>
#include <ctype.h>
#include <sys/time.h>

/* IBM murthya 20060705 include asm/param.h for 'HZ' */
#ifdef __linux
#include <asm/param.h>
#endif

#include "cc/hdr/msgh/MHgdUpd.hh"
#include "cc/hdr/msgh/MHgd.hh"
#include "cc/hdr/msgh/MHregGd.hh"
#include "cc/hdr/init/INusrinit.hh"
#include "MHinfoInt.hh"
#include "MHgdAud.hh"

#define MHgdAckTimeout	6	// Number of no acks before resending message

extern Long MHCLK_TCK;

// This function attaches to the GDO shared memory
// segment and the MSGH shared memory

GLretVal
MHgd::attach(int shmid, MHrt* rt, int lock)
{
	MHgdShm*	ptmp;
	if(m_p != NULL){
		if(m_shmid == shmid){
			// CRERROR("MHgd::attach, already attached m_shmid %d", m_shmid);
      printf("MHgd::attach, already attached m_shmid %d", m_shmid);
			return(MHexist);
		} else {
			//CRERROR("MHgd::attach, already attached m_shmid %d != shmid %d", m_shmid, shmid);
      printf("MHgd::attach, already attached m_shmid %d != shmid %d", m_shmid, shmid);
			ptmp = m_p;
			m_p = NULL;
			shmdt((char*)ptmp);
		}
	}

  if((ptmp = (MHgdShm*)shmat(shmid, 0, 0666)) == (MHgdShm*)-1){
    return(MHshmAttachFail);
  }

	int	permissions = 0666;

	shmdt((char*)ptmp);

  if((m_p = (MHgdShm*)shmat(shmid, 0, permissions)) == (MHgdShm*)-1){
		m_p = NULL;
    return(MHshmAttachFail);
  }

	m_prt = rt;
	m_shmid = shmid;
	return(GLsuccess);
}

// This function creates and replicates the GDO

GLretVal MHgd::create(MHregGd* regMsg, MHrt* rt, IN_SHM_KEY shmkey,
                      Bool bDoSynch) {
	int		shmid;
	Bool		new_flg;
	Bool		bIsLocal;
	MHregGdAck	ackMsg;
	GLretVal	retval;
	Short		srcHost = MHQID2HOST(regMsg->srcQue);
	
	ackMsg.m_RetVal = GLsuccess;
	bIsLocal = (srcHost == rt->LocalHostIndex ||
              srcHost == rt->SecondaryHostIndex);

	if(m_p != NULL){
		// object already exists and is attached
		if(bIsLocal){
			// if requester was on this machine just send the ack 
			ackMsg.m_bIsNew = FALSE; 
			if((ackMsg.m_Shmid = INshmem.getSeg(TRUE, (IN_SHM_KEY)shmkey)) < 0){
				//CRERROR("Segment does not exist shmkey %d, name %s",
        //        shmkey, regMsg->m_Name.display());
        printf("Segment does not exist shmkey %d, name %s",
                shmkey, regMsg->m_Name.display());
				return(GLsuccess);
			}
			// Check to see if still synching this object. If so
			// deny attach or create
			// There is no check of return code, if the ack fails
			// the requester will time out and presumably reinitialize
			if(m_p->m_SyncAddress != m_p->m_Size){
				ackMsg.m_RetVal = MHgdNotReady;
			}
			ackMsg.send(regMsg->srcQue, MHnullQ, (short)sizeof(ackMsg), 0L);
		} else if(rt->m_leadcc == rt->LocalHostIndex){ 
			// If lead, and GDO is distributed on the source machine
			// send the message to the source machine
			if((regMsg->m_Dist != MHGD_CCONLY) || rt->isCC(rt->hostlist[srcHost].hostname.display())){
				regMsg->m_bReplicate = FALSE;
				regMsg->m_shmkey = shmkey;
				MHmsgh.send(MHMAKEMHQID(srcHost, MHmsghQ), (char*)regMsg, sizeof(MHregGd), 0);
			} else {
				ackMsg.m_RetVal = MHwrongNode;
				ackMsg.send(regMsg->srcQue, MHnullQ, (short)sizeof(ackMsg), 0L);
			}
		}
		return(GLsuccess);
	}

	// Verify that the selected MHGDPROC exists
	pid_t	pid;
	if((retval = MHmsgh.checkPid(MHMAKEMHQID(rt->LocalHostIndex,
                                           (MHrprocQ + rt->m_gdCtl[shmkey].m_RprocIndex)),
                               pid)) != GLsuccess){
		//CRERROR("MHGDPROC %d is not alive", rt->m_gdCtl[shmkey].m_RprocIndex);
    printf("MHGDPROC %d is not alive", rt->m_gdCtl[shmkey].m_RprocIndex);
		return(MHnoSpc);
	}
	
	U_LongLong	size;
	size = regMsg->m_Size + regMsg->m_bufferSz + sizeof(MHgdShm) + regMsg->m_msgBufSz ;
	if((regMsg->m_Dist == MHGD_CCONLY) && 
     !rt->isCC(rt->hostlist[rt->LocalHostIndex].hostname.display())){
		//CRERROR("Received GDO %s create message for the wrong node", regMsg->m_Name.display());
    printf("Received GDO %s create message for the wrong node", regMsg->m_Name.display());
		return(MHwrongNode); 
	}

	if(INshmem.getSeg(TRUE, shmkey) >= 0){
		// Internal error, what to do, I think escalate
		//CRERROR("create, shared memory already exists for shmid %d", shmkey);
    printf("create, shared memory already exists for shmid %d", shmkey);
		return(MHexist);
	}

	ackMsg.m_bIsNew = TRUE;
	if((shmid = INshmem.allocSeg(IPC_PRIVATE, size, 
		regMsg->m_Permissions, new_flg, FALSE, shmkey)) < 0) {
      // failed to allocate segments, should not be possible
      // except if swap was exhausted
      ackMsg.m_RetVal = MHnoMem;
      ackMsg.send(regMsg->srcQue, MHnullQ, (short)sizeof(ackMsg), 0L);
      return(MHnoMem);
                                                         }

shmid_ds	buf;
// Attach the shared memory segment
	if((retval = attach(shmid, rt, regMsg->m_Permissions)) != GLsuccess){
		// What to do here since the most likely reason
		// is the virtual memory exhaustion
		// most likely should delete the object and
		// return failure. 
		ackMsg.m_RetVal = MHnoSpc;
		ackMsg.send(regMsg->srcQue, MHnullQ, (short)sizeof(ackMsg), 0L);
		if((retval = INshmem.deallocSeg(shmid)) != 0){
			//CRERROR("Failed to daallocate %s shmem %d, retval %d", regMsg->m_Name.display(), shmid, retval);
      printf("Failed to daallocate %s shmem %d, retval %d",
             regMsg->m_Name.display(), shmid, retval);
		}
		return(retval);
	}

	// Change the user id of the segment to the uid of the creator
	if(shmctl(shmid, IPC_STAT, &buf) == -1){
		// This should be impossible 
		//CRERROR("Shmctl failed for GDO %s shmid %d, errno %d", regMsg->m_Name.display(), shmid, errno);
    printf("Shmctl failed for GDO %s shmid %d, errno %d",
           regMsg->m_Name.display(), shmid, errno);
	}

	buf.shm_perm.uid = regMsg->m_Uid;
	buf.shm_perm.gid = regMsg->m_Uid;

	if(shmctl(shmid, IPC_SET, &buf) == -1){
		// This should be impossible 
		//CRERROR("Shmctl failed for GDO %s shmid %d, errno %d", regMsg->m_Name.display(), shmid, errno);
    printf("Shmctl failed for GDO %s shmid %d, errno %d",
           regMsg->m_Name.display(), shmid, errno);
	}
	
	memset(m_p, 0x0, sizeof(MHgdShm));
	memset(m_p->m_buf, 0xff, sizeof(m_p->m_buf));
	memset(m_p->m_delSend, 0xff, sizeof(m_p->m_delSend));
	memset(m_p->m_updHosts, 0xff, sizeof(m_p->m_updHosts));

	int i;
	for(i = 0; i < MHmaxHostReg; i++){
		m_p->m_Hosts[i].m_lastAcked = MHgdMaxBuf - 1;
		m_p->m_Hosts[i].m_nextMsg = -1;
	}
	for(i = 0; i < MHgdMaxSend; i++){
		m_p->m_delSend[i].m_size = 0;
		m_p->m_delSend[i].m_msgp = 0;
	}
	m_p->m_curBuf = 0;
	m_p->m_headBuf = 0;
	m_p->m_doSync = TRUE;

	// Set up the first buffer for sending
	MHgdUpd*	updmsg;
	updmsg = (MHgdUpd*)m_p->m_Data;
	updmsg->priType = MHintPtyp;
	updmsg->msgType = MHgdUpdTyp;
	updmsg->m_GdId = shmkey;
	updmsg->m_seq = 0;
	updmsg->m_Size = sizeof(MHgdUpd);
	m_p->m_buf[0] = 0;
	mutex_init(&m_p->m_lock, USYNC_PROCESS, NULL);

	// Setup for synching if necessary
	if(bDoSynch){
		m_p->m_SyncAddress = 0;
		m_p->m_LastSync = 0;
	} else {
		m_p->m_SyncAddress = regMsg->m_Size;
		m_p->m_LastSync = regMsg->m_Size;
	}

	strcpy(m_p->m_Name,regMsg->m_Name.display());
	m_p->m_Uid = regMsg->m_Uid;
	m_p->m_Permissions = regMsg->m_Permissions;
	m_p->m_Dist = regMsg->m_Dist;
	m_p->m_Size = regMsg->m_Size;
	m_p->m_bufferSz = regMsg->m_bufferSz;
	m_p->m_maxUpdSz = regMsg->m_maxUpdSz;
	m_p->m_msgBufSz = regMsg->m_msgBufSz;
	m_p->m_doDelaySend = regMsg->m_doDelaySend;
	m_p->m_GdId = shmkey;
	m_p->m_srcQue = MHMAKEMHQID(MHmsgh.getLeadCC(), MHrprocQ + rt->m_gdCtl[shmkey].m_RprocIndex);

	rt->m_gdCtl[shmkey].m_shmid = shmid;
	// Send message to local MHRPROC to have it reread the shared memory
	MHgdAudReq	audmsg;
	if((retval = MHmsgh.send(MHMAKEMHQID(rt->LocalHostIndex, MHrprocQ + rt->m_gdCtl[shmkey].m_RprocIndex), (char*)&audmsg, sizeof(audmsg), 0L)) != GLsuccess) {
		//CRERROR("Failed to send to MHRPROC, retval %d");
    printf("Failed to send to MHRPROC, retval %d", retval);
	}

	if(bIsLocal){
		ackMsg.m_Shmid = shmid;
		ackMsg.send(regMsg->srcQue, MHnullQ, (short)sizeof(ackMsg), 0L);
	}
	
	if(rt->m_envType == MH_peerCluster){
		return(GLsuccess);
	}

	Bool	bReplicate = regMsg->m_bReplicate;
	regMsg->m_bReplicate = FALSE;
	regMsg->m_shmkey = shmkey;

  // Determine the level of distribution here
	// Also, send the create message to all the nodes at the proper
	// replication level. There is no handshaking here, messages are
	// assumed to be delivered, there will be audit of this data
	int	hCnt = 0;
	for(i = 0; i < MHmaxHostReg; i++){
		if(i == m_prt->LocalHostIndex || i == m_prt->SecondaryHostIndex ||
       strncmp(m_prt->hostlist[i].hostname.display(), "as", 2) == 0 ||
			 m_prt->hostlist[i].isused == FALSE){
			continue;
		}
		if((m_p->m_Dist != MHGD_NONE) && (m_p->m_Dist == MHGD_ALL || m_prt->isCC(m_prt->hostlist[i].hostname.display()))){
			m_p->m_Hosts[i].m_bIsDistributed = TRUE;
			m_p->m_updHosts[hCnt]= i;
			hCnt++;
		} else {
			continue;
		}
		if(bReplicate){
			MHmsgh.send(MHMAKEMHQID(i,MHmsghQ), (char*)regMsg, sizeof(MHregGd), 0);
		}
	}

	return(GLsuccess);
}

#define MaxHexDump	2048

char *
MHhexDump(const char* p, int len)
{
	static char 	hexout[MaxHexDump];
  const char*     hexconv = "0123456789ABCDEF";
	char* 		outp = hexout;

	memset(hexout, 0x0, sizeof(hexout));
	for(int count = 0; count < len && outp < hexout + MaxHexDump - 2; p++, count ++){
		*(outp++) = hexconv[(unsigned char)(*p) >> 4];
		*(outp++) = hexconv[(unsigned char)(*p) & 0xf];
		if((count & 0xf) == 0xf){
			*(outp++) = '\n';
		} else if((count & 0x3) == 0x3){
			*(outp++) = ' ';
		}
	}
	*(outp) = 0;
	return(hexout);
}

// If bSynch is set, do not try to synch any unsynched objects
// sync is already in progress, synch only one object at a time
Void
MHgd::doUpdate(Bool& bSynch, MHqid myQid, int hostid)
{

	GLretVal	retval;
	// Check first if this object needs synching
	if(!bSynch && (m_p->m_SyncAddress < m_p->m_Size)){
		if(m_p->m_SyncAddress == m_p->m_LastSync){
			// no progress, send sync request to lead MHRPROC
			MHgdSyncReq	msg;
			msg.m_SyncAddress = m_p->m_SyncAddress;
			msg.m_GdId = m_p->m_GdId;
			msg.srcQue = myQid;
			if((retval = MHmsgh.send(m_p->m_srcQue, (char*)&msg,
                               sizeof(msg), 0L, FALSE)) != GLsuccess){
				// Print an error and try again later
				//CRERROR("Failed to send sync message, retval = %d", retval);
        printf("Failed to send sync message, retval = %d", retval);
			}
		} 

		m_p->m_LastSync = m_p->m_SyncAddress;
		bSynch = TRUE;
	}

	int lastAcked = MHgdMaxBuf;
	int i;
	int firstOp;
	MHgdUpd* pmsg;
	MHgdOp*	op;
	int	msg_full = FALSE;
	Bool	bFoundHost = FALSE;

	if(hostid != MHmaxHostReg){
		// Only send any queued updates for a specific host
		goto sendUpdate;
	}
	int	k;
	for(k = 0; (i = m_p->m_updHosts[k]) >= 0 ; k++){
		if(!m_prt->hostlist[i].isactive){
			m_p->m_Hosts[i].m_lastAcked = m_p->m_curBuf - 1;
			if(m_p->m_Hosts[i].m_lastAcked < 0){
				m_p->m_Hosts[i].m_lastAcked = MHgdMaxBuf - 1;
			}
			// If the host was down for any amount of time
			// drop sequencing and accept next GDO message
			// independent of sequencing status
			m_p->m_Hosts[i].m_nextMsg = -1;
		} else {
			bFoundHost = TRUE;
		}
		if(lastAcked == MHgdMaxBuf){
			lastAcked = m_p->m_Hosts[i].m_lastAcked;
		} else {
			if(m_p->m_Hosts[i].m_lastAcked < m_p->m_curBuf){
				if(lastAcked < m_p->m_curBuf){
					if(lastAcked > m_p->m_Hosts[i].m_lastAcked){
						lastAcked = m_p->m_Hosts[i].m_lastAcked;
					}
				}
			} else {
				if(lastAcked > m_p->m_curBuf){
					if(lastAcked > m_p->m_Hosts[i].m_lastAcked){
						lastAcked = m_p->m_Hosts[i].m_lastAcked;
					}
				} else {
					lastAcked = m_p->m_Hosts[i].m_lastAcked;
				}
			}
		}
	}

	mutex_lock(&m_p->m_lock);
	m_p->m_lockcnt++;

	if(bFoundHost == FALSE){
		lastAcked = m_p->m_curBuf - 1;
		if(lastAcked < 0){
			lastAcked = MHgdMaxBuf - 1;
		}
		while(m_p->m_nLastSent != m_p->m_nNextOp){
			m_p->m_Ops[m_p->m_nLastSent].m_type = MHGD_EMPTY;
			m_p->m_nLastSent++;
			if(m_p->m_nLastSent >= MHgdMaxOps){
				m_p->m_nLastSent = 0;
			}
		}
		// Send all the ack messages
		while(m_p->m_NextSent != m_p->m_NextMsg){
			if(m_p->m_delSend[m_p->m_NextSent].m_qid != MHnullQ && 
         (retval = MHmsgh.send(m_p->m_delSend[m_p->m_NextSent].m_qid, 
                               &m_p->m_Data[m_p->m_bufferSz + m_p->m_delSend[m_p->m_NextSent].m_msgp], 
                               m_p->m_delSend[m_p->m_NextSent].m_size, 0L)) != GLsuccess){
				//CRDEBUG(CRmsgh + 4, ("Failed to deliver notification message, retval %d",retval));
        printf("Failed to deliver notification message, retval %d",retval);
			}
			m_p->m_delSend[m_p->m_NextSent].m_qid = MHnullQ;
			m_p->m_NextSent ++;
			if(m_p->m_NextSent > MHgdMaxSend){
				m_p->m_NextSent = 0;
			}
		}
		m_p->m_StartToUpdate = m_p->m_NextMsg;
	}

	// lastAcked now contains the last buffer acked for all hosts
	// that are still connected.  Hosts that we cannot talk to are
	// ignored, once connectivity is lost the other host must
	// reboot and resynch GDO
	// Free the acked data buffers
	lastAcked += 1;
	if(lastAcked >= MHgdMaxBuf){
		lastAcked = 0;
	}

	while(m_p->m_headBuf != lastAcked){
		// Free the data
		m_p->m_buf[m_p->m_headBuf] = MHempty;
		// Send all ack messages
		while(m_p->m_NextSent != m_p->m_NextMsg &&
          m_p->m_delSend[m_p->m_NextSent].m_msgindx == m_p->m_headBuf){
			if((retval = MHmsgh.send(m_p->m_delSend[m_p->m_NextSent].m_qid, 
                               &m_p->m_Data[m_p->m_bufferSz + m_p->m_delSend[m_p->m_NextSent].m_msgp], 
                               m_p->m_delSend[m_p->m_NextSent].m_size, 0L)) != GLsuccess){
				//CRDEBUG(CRmsgh + 4, ("Failed to deliver notification message, retval %d",retval));
        printf("Failed to deliver notification message, retval %d",retval);
			}
			m_p->m_delSend[m_p->m_NextSent].m_msgindx = -1;
			m_p->m_delSend[m_p->m_NextSent].m_qid = MHnullQ;
			m_p->m_NextSent ++;
			if(m_p->m_NextSent >= MHgdMaxSend){
				m_p->m_NextSent = 0;
			}
		}
		m_p->m_headBuf++;
		if(m_p->m_headBuf >= MHgdMaxBuf){
			m_p->m_headBuf = 0;
		}
	}

	pmsg = (MHgdUpd*)&m_p->m_Data[m_p->m_buf[m_p->m_curBuf]];
	// Check all invalidate regions and send the updated data messages
	firstOp = m_p->m_nLastSent;
	while(m_p->m_nLastSent != m_p->m_nNextOp){
		op = &m_p->m_Ops[m_p->m_nLastSent];
		switch(op->m_type){
		case MHGD_INVALIDATE:
       {
         // Do validity checks to detect data corruption.

         if(op->m_invalidate.m_length < 0 || op->m_invalidate.m_length >= m_p->m_maxUpdSz - MHgdUpdHead ||
            op->m_invalidate.m_start < m_p->m_bufferSz + m_p->m_msgBufSz ||
            op->m_invalidate.m_start + op->m_invalidate.m_length > 
            m_p->m_bufferSz + m_p->m_msgBufSz + m_p->m_Size){

           //CRERROR("Corrupted GDO %s invalidate table lastSent = %d, nNextOp = %d\n%s",
           //        m_p->m_Name, m_p->m_nLastSent, m_p->m_nNextOp, MHhexDump((char*)(op - 20), 40 * sizeof(MHgdOp)));
           printf("Corrupted GDO %s invalidate table lastSent = %d, nNextOp = %d\n%s",
                  m_p->m_Name, m_p->m_nLastSent, m_p->m_nNextOp,
                  MHhexDump((char*)(op - 20), 40 * sizeof(MHgdOp)));
           break;
         }
         // Call invalidateSeq if enough space
         if(pmsg->m_Size + sizeof(MHgdOp) + sizeof(LongLong) + op->m_invalidate.m_length >= m_p->m_maxUpdSz){
           msg_full = TRUE;
           break;
         }
         invalidateSeqCmpInt(&m_p->m_Data[op->m_invalidate.m_start], op->m_invalidate.m_length, 0, -1, TRUE);
         break;
       }
		case MHGD_MEMMOVE:
       {
         // Call memMoveSeq if enough space
         if(pmsg->m_Size + sizeof(MHgdOp) + sizeof(LongLong) >= m_p->m_maxUpdSz){
           msg_full = TRUE;
           break;
         }
         memMoveSeqInt(&m_p->m_Data[op->m_memmove.m_to],&m_p->m_Data[op->m_memmove.m_from], op->m_memmove.m_length, TRUE);
         break;
       }
		case MHGD_INVCMP:
       {
         // Call invalidateSeq if enough space
         if(pmsg->m_Size + sizeof(MHgdOp) + sizeof(LongLong) + op->m_invcmp.m_length >= m_p->m_maxUpdSz){
           msg_full = TRUE;
           break;
         }
         invalidateSeqCmpInt(&m_p->m_Data[op->m_invcmp.m_start], op->m_invcmp.m_length, op->m_invcmp.m_offset, op->m_invcmp.m_qid, TRUE);
         break;
       }
		default:
			//CRERROR("Unexpected operation type %d", op->m_type);
      printf("Unexpected operation type %d", op->m_type);
			break;
		}
		if(!msg_full){
			op->m_type = MHGD_EMPTY;
			m_p->m_nLastSent++;
			if(m_p->m_nLastSent >= MHgdMaxOps){
				m_p->m_nLastSent = 0;
			}
		} else {
			break;
		}
	}

  // If there are no pending updates, just send the messages out
  if(m_p->m_headBuf == m_p->m_curBuf && firstOp == m_p->m_nLastSent){
    // Send all the ack messages
    while(m_p->m_NextSent != m_p->m_NextMsg){
      if(m_p->m_delSend[m_p->m_NextSent].m_qid != MHnullQ &&
         (retval = MHmsgh.send(m_p->m_delSend[m_p->m_NextSent].m_qid,
                               &m_p->m_Data[m_p->m_bufferSz + m_p->m_delSend[m_p->m_NextSent].m_msgp],
                               m_p->m_delSend[m_p->m_NextSent].m_size, 0L)) != GLsuccess){
        //CRDEBUG(CRmsgh+4, ("Failed to deliver notification message, retval %d",retval));
        printf("Failed to deliver notification message, retval %d",retval);
      }
      m_p->m_delSend[m_p->m_NextSent].m_qid = MHnullQ;
      m_p->m_NextSent ++;
      if(m_p->m_NextSent > MHgdMaxSend){
        m_p->m_NextSent = 0;
      }
    }
    m_p->m_StartToUpdate = m_p->m_NextMsg;
  }
	
	// Go through the deferred messages and update the msgindex
	while(m_p->m_StartToUpdate != m_p->m_NextMsg){
		if(firstOp <= m_p->m_nLastSent){
			if(m_p->m_delSend[m_p->m_StartToUpdate].m_opNum < m_p->m_nLastSent &&
         m_p->m_delSend[m_p->m_StartToUpdate].m_opNum >= firstOp){
				m_p->m_delSend[m_p->m_StartToUpdate].m_msgindx = m_p->m_curBuf;
			} else {
				break;
			}
		} else {
			if(m_p->m_delSend[m_p->m_StartToUpdate].m_opNum < m_p->m_nLastSent|| 
         m_p->m_delSend[m_p->m_StartToUpdate].m_opNum >= firstOp){
				m_p->m_delSend[m_p->m_StartToUpdate].m_msgindx = m_p->m_curBuf;
			} else {
				break;
			}
		}
		m_p->m_StartToUpdate++;
		if(m_p->m_StartToUpdate >= MHgdMaxSend){
			m_p->m_StartToUpdate = 0;
		}
	}

	mutex_unlock(&m_p->m_lock);

  sendUpdate:
	MHgdUpd* 	tmpmsg;
	int		nextmsg;
	itimerval 	oldtimer;
	struct tms	tbuf;
	clock_t		otime;
	int		tdiff;

	// Send out the message to all hosts that are supposed to get
	// it if there is anything in the buffer
	for(k = 0; (i = m_p->m_updHosts[k]) >= 0 ; k++){
		if(hostid != MHmaxHostReg && i != hostid){
			continue;
		}
		if((!m_prt->hostlist[i].isactive)){ 
			continue;
		}
		nextmsg = m_p->m_Hosts[i].m_lastAcked + 1;
		if(nextmsg >= MHgdMaxBuf){	
			nextmsg = 0;
		}
    if(m_p->m_buf[nextmsg] == MHempty){
      //CRERROR("Empty message buffer %d, GDO %.20s", nextmsg, m_p->m_Name);
      printf("Empty message buffer %d, GDO %.20s", nextmsg, m_p->m_Name);
      m_p->m_Hosts[i].m_lastAcked ++;
      if(m_p->m_Hosts[i].m_lastAcked >= MHgdMaxBuf){
        m_p->m_Hosts[i].m_lastAcked = 0;
      }
      nextmsg = m_p->m_Hosts[i].m_lastAcked + 1;
      if(nextmsg >= MHgdMaxBuf){
        nextmsg = 0;
      }
    }
		pmsg = (MHgdUpd*)&m_p->m_Data[m_p->m_buf[nextmsg]];
    if(pmsg->msgType != MHgdUpdTyp || pmsg->m_seq != nextmsg || pmsg->m_GdId !=
       m_p->m_GdId){
      //CRERROR("Incorrect send buf msgType 0x%x, m_seq %d (%d), m_GdId %d (%d)",
      //        pmsg->msgType, pmsg->m_seq, nextmsg, pmsg->m_GdId, m_p->m_GdId);
      printf("Incorrect send buf msgType 0x%x, m_seq %d (%d), m_GdId %d (%d)",
              pmsg->msgType, pmsg->m_seq, nextmsg, pmsg->m_GdId, m_p->m_GdId);
      pmsg->msgType = MHgdUpdTyp;
      pmsg->m_seq = nextmsg;
      pmsg->m_GdId = m_p->m_GdId;
    }
		// Do not resend to this host if Ack timeout did not occur yet
		if(m_p->m_Hosts[i].m_noAcks > 0 && m_p->m_Hosts[i].m_noAcks < MHgdAckTimeout){
			m_p->m_Hosts[i].m_noAcks ++;
			// If the last message was greater then a message limit
			// it has been sent buffered.  Send a small buffered
			// message to flush out any unsent buffers
			if(pmsg->m_Size >= MHmsgLimit){
				MHhostDel	delmsg;
				delmsg.mtype = MHhostDelTyp;
				delmsg.ptype = MHintPtyp;
				if((retval = MHmsgh.send(MHMAKEMHQID(i, MHrprocQ + m_prt->m_gdCtl[m_p->m_GdId].m_RprocIndex), (char*)&delmsg, sizeof(delmsg), 0L)) != GLsuccess){
					// What to do???
					//CRDEBUG(CRmsgh + 4, ("Failed to send protocol message retval %d", retval));
          printf("Failed to send protocol message retval %d", retval);
				}
			}
			continue;
		}
		if(pmsg->m_Size > sizeof(MHgdUpd) || (nextmsg != m_p->m_curBuf && pmsg->m_Size == sizeof(MHgdUpd))){
			mutex_lock(&m_p->m_lock);
			m_p->m_lockcnt++;
			if(nextmsg == m_p->m_curBuf){ 
				tmpmsg = pmsg;
				getUpdBuf(tmpmsg, FALSE);
				if(tmpmsg == NULL){
					mutex_unlock(&m_p->m_lock);
					continue;
				}
			}
			mutex_unlock(&m_p->m_lock);
			m_p->m_Hosts[i].m_noAcks = 1;
			// Send this message 
			pmsg->srcQue = myQid;

			Bool	buffered;
			if(pmsg->m_Size < MHmsgLimit){
				buffered = FALSE;
			} else {
				buffered = TRUE;
			}
			if((retval = MHmsgh.send(MHMAKEMHQID(i, MHrprocQ + m_prt->m_gdCtl[m_p->m_GdId].m_RprocIndex), (char*)pmsg, pmsg->m_Size, 0L, buffered)) != GLsuccess){
				// Print error and try again later
				//CRDEBUG(CRmsgh+4, ("Failed to send update message retval %d", retval));
        printf("Failed to send update message retval %d", retval);
				m_p->m_SendFailed++;
			}
		} else if(pmsg->m_Size < sizeof(MHgdUpd)){
			//CRDEBUG(CRmsgh+7, ("Corrupted message size %d", pmsg->m_Size));
      printf("Corrupted message size %d", pmsg->m_Size);
			pmsg->m_Size = sizeof(MHgdUpd);
		}
	}
}

Void
MHgd::syncData(MHgdSyncData* syncData, MHqid myQid)
{
	GLretVal	retval;

	// Objects previously synched, still can be forcibly resynched
	if(syncData->m_SyncAddress == 0){
		m_p->m_srcQue = syncData->srcQue;
		m_p->m_SyncAddress = 0;
	}

	// if object already synched, throw this message away
	if(m_p->m_SyncAddress >= m_p->m_Size || syncData->m_SyncAddress < 0){
		//CRERROR("Object %s already synched size=%ld", m_p->m_Name, m_p->m_Size);
    printf("Object %s already synched size=%ld", m_p->m_Name, m_p->m_Size);
		return;
	}

	// If message out of order throw it away
	if(m_p->m_SyncAddress != syncData->m_SyncAddress){
		//CRDEBUG(CRmsgh+4, ("Object %s message out of order object syncaddress = %ld != message syncaddress %ld", 
    //                   m_p->m_Name, m_p->m_SyncAddress, syncData->m_SyncAddress));
    printf("Object %s message out of order object syncaddress = %ld != message syncaddress %ld", 
                       m_p->m_Name, m_p->m_SyncAddress, syncData->m_SyncAddress);
		return;
	}

	// if the message exceeds object boundary, throw it away.  Probably should reinit.
	if(m_p->m_SyncAddress + syncData->m_Length > m_p->m_Size){
		//CRERROR("Exceeded object size SyncAddress %ld, length %ld, size %ld",
    //        m_p->m_SyncAddress, syncData->m_Length, m_p->m_Size);
    printf("Exceeded object size SyncAddress %ld, length %ld, size %ld",
            m_p->m_SyncAddress, syncData->m_Length, m_p->m_Size);
	}

	LongLong newAddress = m_p->m_SyncAddress + syncData->m_Length;

	MHgdSyncReq	msg;
	msg.m_GdId = syncData->m_GdId;
	msg.srcQue = myQid;
	msg.m_SyncAddress = newAddress;
	if((retval = MHmsgh.send(syncData->srcQue,  (char*)&msg,
                           sizeof(msg), 0L, FALSE)) != GLsuccess){
		// The other side will resend
		//CRDEBUG(0x1,("Failed to send sync message retval %d", retval));
    printf("Failed to send sync message retval %d", retval);
	}

	// Do memcpy after Ack, this will slightly speed up download
	memcpy(&m_p->m_Data[m_p->m_SyncAddress + m_p->m_bufferSz + m_p->m_msgBufSz], syncData->m_Data, syncData->m_Length);
	m_p->m_SyncAddress = newAddress;
}

#define  MHgdAckSeq	0xc001f001

void
MHgd::handleUpdate(MHgdUpd* pmsg, MHqid myQid)
{
	MHgdOp*		opp;
	Long		offset;
	Short		hostId = MHQID2HOST(pmsg->srcQue);
	GLretVal	retval;

	offset = sizeof(MHgdUpd);
	if(pmsg->m_seq == MHgdAckSeq){
		// This is just an Ack, update info and return
    // Do not allow duplicate acks to reset the seq number backwards
    if(((m_p->m_Hosts[hostId].m_lastAcked - pmsg->m_ackSeq) == 1) ||
       ((m_p->m_Hosts[hostId].m_lastAcked == 0) &&
        (pmsg->m_ackSeq == (MHgdMaxBuf - 1)))){
      return;
    }
		if(m_p->m_buf[pmsg->m_ackSeq] == MHempty){
			//CRDEBUG(CRmsgh+4,("%s last acked %d host %d, ackSeq %d", 
      //                  m_p->m_Name, m_p->m_Hosts[hostId].m_lastAcked, hostId, pmsg->m_ackSeq));
      printf("%s last acked %d host %d, ackSeq %d", 
             m_p->m_Name, m_p->m_Hosts[hostId].m_lastAcked, hostId, pmsg->m_ackSeq);
			return;
		}
		m_p->m_Hosts[hostId].m_lastAcked = pmsg->m_ackSeq;
		m_p->m_Hosts[hostId].m_noAcks = 0;
		// if this host is behind, send another message
		int nextmsg = pmsg->m_ackSeq + 1;
		if(nextmsg >= MHgdMaxBuf){
			nextmsg = 0;
		}
		if(nextmsg != m_p->m_curBuf){
			Bool bSync = TRUE;
			doUpdate(bSync, myQid, hostId);
		}
		return;
	}

	MHgdUpd		ack;
	ack.m_Size = sizeof(MHgdUpd);
	ack.m_ackSeq = pmsg->m_seq;
	ack.m_seq = MHgdAckSeq;
	ack.srcQue = myQid;
	ack.m_GdId = m_p->m_GdId;

	// Do sequence checking
	if(pmsg->m_seq != m_p->m_Hosts[hostId].m_nextMsg){
		// If object just created, first update will be
		// synchronized with the updating host
		if(m_p->m_Hosts[hostId].m_nextMsg == -1){
			m_p->m_Hosts[hostId].m_nextMsg = pmsg->m_seq;
		} else {
			m_p->m_Hosts[hostId].m_nOutOfSeq++;
			// If the sequence number is one less, this must be
			// retransmission. Just ack it immediately
			if(((m_p->m_Hosts[hostId].m_nextMsg - pmsg->m_seq) == 1) ||
         ((m_p->m_Hosts[hostId].m_nextMsg == 0) &&
          (pmsg->m_seq == (MHgdMaxBuf - 1)))){
	
				//CRDEBUG(CRmsgh+4, ("Out of sequence message from host %d, GDO %s, expected %d, got %d", hostId, m_p->m_Name, m_p->m_Hosts[hostId].m_nextMsg, pmsg->m_seq));
        printf("Out of sequence message from host %d, GDO %s, expected %d, got %d",
               hostId, m_p->m_Name, m_p->m_Hosts[hostId].m_nextMsg, pmsg->m_seq);
				if((retval = MHmsgh.send(pmsg->srcQue, (char*)&ack, sizeof(MHgdUpd), 0L, FALSE)) != GLsuccess){
					if(retval == MHnoHost){
						//CRDEBUG(CRmsgh+4, ("Host %d not accessible for update of %s", hostId, m_p->m_Name));
            printf("Host %d not accessible for update of %s", hostId, m_p->m_Name);
					} else {
						//CRERROR("Failed to send ack message retval %d gdo %s", retval, m_p->m_Name);
            printf("Failed to send ack message retval %d gdo %s", retval, m_p->m_Name);
					}
				}
			} else {
				//CRERROR("Out of sequence message from host %d, GDO %s, expected %d, got %d", hostId, m_p->m_Name, m_p->m_Hosts[hostId].m_nextMsg, pmsg->m_seq);
        printf("Out of sequence message from host %d, GDO %s, expected %d, got %d",
               hostId, m_p->m_Name, m_p->m_Hosts[hostId].m_nextMsg, pmsg->m_seq);
			}
			return;
		}
	}

	m_p->m_Hosts[hostId].m_nextMsg++;
	if(m_p->m_Hosts[hostId].m_nextMsg >= MHgdMaxBuf){
		m_p->m_Hosts[hostId].m_nextMsg = 0;
	}

	
	while(offset < pmsg->m_Size){
		// align the offset
		if((offset & MHlongAlign) != 0){
			offset = (offset + sizeof(LongLong)) & MHlongAlign;
		}
		opp = (MHgdOp*)((char*)pmsg + offset);
		offset += sizeof(MHgdOp);

		switch(opp->m_type){
		case MHGD_INVALIDATE:
       {
         char* to = m_p->m_Data + opp->m_invalidate.m_start;
         memcpy(to, (char*)pmsg + offset, opp->m_invalidate.m_length);
         offset += opp->m_invalidate.m_length;
         break;
       }
		case MHGD_MEMMOVE:
       {
         char* to = m_p->m_Data + opp->m_memmove.m_to;
         char* from = m_p->m_Data + opp->m_memmove.m_from;
         // If memory does not overlap use memcpy instead for performance
         if((to + opp->m_memmove.m_length <= from) ||
            (from + opp->m_memmove.m_length <= to)){
           memcpy(to, from, opp->m_memmove.m_length);
         } else {
           memmove(to, from, opp->m_memmove.m_length);
         }
         break;
       }
		case MHGD_INVCMP:
       {
			
         char* local = m_p->m_Data + opp->m_invalidate.m_start;
         MHqid	notifyQid;
         notifyQid.setval(opp->m_invcmp.m_qid);
         invCmpCheck(notifyQid, (char*)pmsg + offset, local, opp->m_invcmp.m_length, opp->m_invcmp.m_offset);
         offset += opp->m_invcmp.m_length;
         break;
       }
		default:
			//CRERROR("%s invalid op type %d", m_p->m_Name, opp->m_type);
      printf("%s invalid op type %d", m_p->m_Name, opp->m_type);
			break;
		}
	}

	// Send the ack back, unbuffered
	if((retval = MHmsgh.send(pmsg->srcQue, (char*)&ack, sizeof(MHgdUpd), 0L, FALSE)) != GLsuccess){
		if(retval == MHnoHost){
			//CRDEBUG(CRmsgh+4, ("Host %d not accessible for update of %s", hostId, m_p->m_Name));
      printf("Host %d not accessible for update of %s", hostId, m_p->m_Name);
		} else {
			//CRERROR("Failed to send ack message retval %d gdo %s", retval, m_p->m_Name);
      printf("Failed to send ack message retval %d gdo %s", retval, m_p->m_Name);
		}
	}
}

// This function is called to ack updates for a short duration
// during initialization when node is reachable but GDO
// information is not synched yet.  

void
MHgd::updateNone(MHgdUpd* pmsg)
{
	GLretVal	retval;
	// Send the ack back
	MHgdUpd		ack;
	ack.m_Size = sizeof(MHgdUpd);
	ack.m_ackSeq = pmsg->m_seq;
	ack.m_seq = MHgdAckSeq;
	ack.srcQue = MHMAKEMHQID(MHmsgh.getLocalHostIndex(), MHrprocQ + 1);
	ack.m_GdId = pmsg->m_GdId;
	
	if((retval = MHmsgh.send(pmsg->srcQue, (char*)&ack, sizeof(MHgdUpd), 0L, FALSE)) != GLsuccess){
		//CRERROR("Failed to send ack message retval %d", retval);
    printf("Failed to send ack message retval %d", retval);
	}
}

Void
MHgd::audMutex()
{
	if(m_p == NULL){
		return;
	}
  // If lock count incremented, return FALSE
  if(m_p->m_lockcnt != m_p->m_lastlockcnt){
    m_p->m_lastlockcnt = m_p->m_lockcnt;
		return;
  }

  if(mutex_trylock(&m_p->m_lock) == 0){
		// Mutex is free
    mutex_unlock(&m_p->m_lock);
  } else {
    // This was forcible unlock
    mutex_unlock(&m_p->m_lock);
		//CRERROR("Forced unlock of mutex for GDO %s", m_p->m_Name);
    printf("Forced unlock of mutex for GDO %s", m_p->m_Name);
  }
}

void
MHgd::audit(MHaudGd* audmsg, MHqid myqid)
{
	register Long	sum = 0;
	register Long*	p;
	register Long*	endp;
	char*		pStart;

	pStart = m_p->m_Data + audmsg->m_Start;
	p = (Long*)pStart;
	endp = (Long*)(pStart + audmsg->m_Length);

  // Handle the case where the size is not a multiple of long
  int     bytesLeft = ((LongLong)endp) & MHlongLeft;
	endp = (Long*)(((LongLong)endp) & MHlongAlign);

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

	MHaudGdAck	ackmsg;
	ackmsg.m_Name = m_p->m_Name;
	ackmsg.srcQue = myqid;
	if(audmsg->m_Sum == sum){
		ackmsg.m_RetVal = GLsuccess;
	} else {
		ackmsg.m_RetVal = GLfail;
	}

	MHmsgh.send(audmsg->srcQue, (char*)&ackmsg, sizeof(ackmsg), 0L);
	if(ackmsg.m_RetVal != GLsuccess && audmsg->m_Action == MHgdBoot){
		// Reboot this node
		//CRERROR("Failed GDO audit for %s, address 0x%x, length %d",
    //        m_p->m_Name, pStart, audmsg->m_Length);
    printf("Failed GDO audit for %s, address 0x%x, length %d",
            m_p->m_Name, pStart, audmsg->m_Length);
		INITREQ(SN_LV4, MHgdFailedAudit, "FAILED GDO AUDIT", IN_EXIT);
	}
	/*
		20061107 IBM murthya : The author of the change in the following
		line is binzhuhu
	*/
	m_audthread = (thread_t)(-1);
}


#define MHrtdbCntrLength	4

//
//RULE:
//   1. if the foreign CC counter is less then local CC counter,
//      conflict resolution needed.
//      NO update happened to counter or tuple.
//      prepare message contains kept value(from local CC) and drop value (from foreign CC)
//      send out conflict notification message to RPROC_QID.
//
//   2. if the foreign CC counter equal to local CC counter
//      conflict resolution needed.
//      if local is lead CC then
//         drop foreign CC value.
//         prepare message contains new value(from local CC) and old value (from foreign CC)
//         send out conflict notification message to RPROC_QID.  
//     else normal update the value from foreign CC.
//         no message sent.
//
//   3. if the foreign CC counter greater then the local cc.
//      normal update to counter and tuple
//      no message sent.
//

void  
MHgd::invCmpCheck(MHqid notifyQid, const void* remote, void* local, Long length, int offset)
{
  char    frCounterBuf[5], lcCounterBuf[5];
  int    frCounter, lcCounter, fr, lc;

  memcpy( frCounterBuf, (char*)remote + offset, MHrtdbCntrLength );
  memcpy( lcCounterBuf, (char*)local + offset, MHrtdbCntrLength );
	frCounterBuf[MHrtdbCntrLength] = 0;
	lcCounterBuf[MHrtdbCntrLength] = 0;
        
  // Assumption: 
  // after SPA used new libary, all foreign update will contain new counter format.
  // local counter is old format like '3B97EF00' then just overwrite it.
  int i;
  for( i = 0; i < MHrtdbCntrLength; i++ ){
    if( !isdigit(lcCounterBuf[i]) || !isdigit(frCounterBuf[i]) ){
      memcpy( local, remote, length );
      return;
    }
  }
                        
  fr = frCounter = atoi(frCounterBuf);
  lc = lcCounter = atoi(lcCounterBuf);
        
  Bool reportConflict = FALSE;

  // same race condition check rules are used in RDBr::chkRaceUpd(), too.
  if ( (lc != 0) && (fr != 0) ) {
    // max counter value is '9000'.
    if ( (fr > 6000) && (lc < 3000) ) {
      lc += 9000;
    }

    if ( (lc > 6000) && (fr < 3000) ) {
      fr += 9000;
    }

    // RULE 1: if the foreign CC counter is less then local CC counter
    if (fr < lc ) {
      reportConflict = TRUE;
    }
    // RULE 2: if the foreign CC counter equal to local CC counter at LEAD CC
    else if ( fr == lc ) {
      if (MHmsgh.onLeadCC() == TRUE) { 
        reportConflict = TRUE; 
      }
    }
  }
        
  if ( reportConflict == TRUE ) {
    MHraceNotifyMsg GDOraceMsg;
    GDOraceMsg.setKeptRecord(local,length);
    GDOraceMsg.setDiscardedRecord(remote,length);
    GDOraceMsg.localCounter = lcCounter;
    GDOraceMsg.foreignCounter = frCounter;

    GDOraceMsg.send(notifyQid, MHnullQ, GDOraceMsg.getSendSize(), 0 );
  } else {
    memcpy( local, remote, length );
  }

  return;
}
