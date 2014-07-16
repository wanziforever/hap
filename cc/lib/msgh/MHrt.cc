#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include "cc/hdr/msgh/MHrt.hh"
#include "cc/hdr/msgh/MHcargo.hh"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

int MHmaxCargoSz; // Maximal size of cargo

int MHsock_id;
Void MHrt::SetSockId(int sock_id) {
  MHsock_id = sock_id;
}

Bool MHnoErrr = TRUE;

MHqid MHmakeqid(int hostid, int qid) {
  int tmp = (((hostid) << MHhostShift) | qid);
  return (*((MHqid*)&tmp));
}

void MHsetHostId(void* pmsg, Short hostId) {
  ((MHregName*)pmsg)->HostId = (U_char)(hostId & 0x7f);
  ((MHregName*)pmsg)->bits =
     (((MHregName*)pmsg)->bits & 0xf8) | ((hostId >> 7) && 0x7);
}

Short MHgetHostId(void* pmsg) {
  return((short)((((MHregName*)pmsg)->HostId) & 0x7f) | \
         (((Short)(((MHregName*)pmsg)->bits) & 0x7) << 7));
}

Bool* MHrt::datafree;
Long* MHrt::data;
int MHrt::numHosts = 0;

Bool MHrt::isHostInScope(Short hostid, MHclusterScope scope) const {
  if (oam_lead[0].display()[0] != 0) {
    switch(scope) {
    case MH_scopeSystemAll:
    case MH_scopeSystemPilot:
    case MH_scopeSystemOther:
       {
         int i;
         for (i = 0; i < MHmaxPilotNodes; i ++) {
           if (hostlist[hostid].hostname == oam_lead[i]) {
             break;
           }
         }
         if (i == MHmaxPilotNodes) {
           return (FALSE);
         }
         break;
       }
    case MH_scopeAll:
       {
         break;
       }
    }
  }

  return (TRUE);
}

// Search 'mhname' from the routing table. If the name is found,
// return the associated mhqid
// Search for the name on all active hosts starting from hostid
int MHrt::findName(Short& hostid, const Char *name,
                   MHclusterScope scope) const {
  Short elemid;
  if (hostid < -1 || hostid >= MHmaxAllHosts) {
    return (MHinvHostId);
  }

  hostid++;
  for (; hostid < MHmaxAllHosts; hostid++) {
    if (!hostlist[hostid].isused ||             \
        !hostlist[hostid].isactive ||           \
        (!isHostInScope(hostid, scope))) {
      continue;
    }
    if ((elemid = findNameOnHost(hostid, name)) != MHempty) {
      break;
    }
  }
  if (hostid == MHmaxAllHosts) {
    return(MHnoEnt);
  }
  return (MHGETQ(rt[elemid].mhqid));
}

// This function, given a MHname, provides the caller with mhqid
MHqid MHrt::findName(const Char *name) const {
  const Char *colonptr;
  Short elemid;
  MHqid mhqid;

  if ((colonptr = strchr(name, ':')) != NULL) {
    // OK, We're looking up a name on a particular host
    Short hostid;
    Char hostname[MHmaxNameLen];
    Short len = MIN(colonptr - name, MHmaxNameLen-1);

    strncpy(hostname, name, len);
    hostname[len] = 0;

    if ((hostid = findHost(hostname)) == MHempty) {
      // No such machine
      printf("findName: can't find host %s for name %s", hostname, name);
      return(MHnullQ);
    }
    if (hostid == SecondaryHostIndex) {
      hostid = LocalHostIndex;
    }

    // Ok, got a good host, look it up in host
    if ((elemid = findNameOnHost(hostid, colonptr+1)) == MHempty) {
      // No such Name
      printf("findName: Name %s found on host %s(%d), queue id %s",
             name, hostname, hostid, rt[elemid].mhqid.display());
      return(rt[elemid].mhqid);
    } else {
      // OK, we're looking for a name without a machine specifier.
      // First look in local table, then on OAM lead, then in global.
      if ((elemid = findNameOnHost(LocalHostIndex, name)) != MHempty) {
        printf("findqids: name %s local, qid %s", name,
               rt[elemid].mhqid.display());
        return(rt[elemid].mhqid);
      }
    }
    if (m_oamLead != MHempty) {
      if ((elemid = findNameOnHost(m_oamLead, name)) != MHempty) {
        printf("findName: name %s oamLead, qid %s",
               name, rt[elemid].mhqid.display());
        return(rt[elemid].mhqid);
      }
    }
    if ((elemid = findNameOnGlobal(name)) != MHempty) {
      printf("findqids: name %s global , qid %s",
             name, rt[elemid].mhqid.display());
      mhqid = rt[elemid].mhqid;
      Short host = MHQID2HOST(mhqid);

      if (host == MHgQHost &&
          gqdata[MHQID2QID(mhqid)].m_selectedQ == MHempty) {
        return(MHnullQ);
      }
      return(mhqid);
    }
  }
  printf("findName: Could not find name %s", name);
  return(MHnullQ);
}

Short MHrt::findHost(const Char *name) const {
  if ((!isdigit(name[0])) || (!isdigit(name[2])) || (!isdigit(name[4])) ||
      name[1] != '-' || name[3] != '-' || (!isdigit(name[5]) && name[5] != 0)) {
    MHname hostname(name);
    Short key = (Short) (hostname.foldKey() % MHhostHashTblSz); // Hash key
    Short hostindex = host_hash[key];

    while ((hostindex != MHempty) &&
           (hostname != hostlist[hostindex].hostname)) {
      hostindex = hostlist[hostindex].next;
    }
    return(hostindex);
  }

  int rack = (name[0] & 0xf);
  int chassis = (name[2] & 0xf);
  int slot = atoi(&name[4]);

  if (rack < MHmaxRack && chassis < MHmaxChassis && slot < MHmaxSlot) {
    return (RCStoHostId[rack][chassis][slot]);
  }
  return(MHempty);
}

Short MHrt::getRealHostname(const Char* hostname, Char *realname) {
  short hostindex;
  if ((hostindex = findHost(hostname)) == MHempty) {
    return(MHempty);
  }
  strcpy(realname, hostlist[hostindex].realname);
  return(hostindex);
}

// This code is not efficient but should not be used frequently
// There is no hasing of real host names provided.
Short MHrt::getLogicalHostname(const Char* realname, Char *hostname) {
  int i;
  for (i = 0; i < MHmaxHostReg; i++) {
    if (hostlist[i].isused == FALSE) {
      continue;
    }
    if (strcmp(hostlist[i].realname, realname) == 0) {
      break;
    }
  }
  if (i < MHmaxHostReg) {
    strcpy(hostname, hostlist[i].hostname.display());
  } else {
    i = MHempty;
  }
  return(i);
}

Short MHrt::findNameOnHost(Short hostid, const Char *name) const {
  MHname procname(name);

  if (hostid == SecondaryHostIndex) {
    hostid = LocalHostIndex;
  }

  Short key = (Short) (procname.foldKey() % MHnameHashTblSz); // Hash key
  Short procindex = hostlist[hostid].hashlist[key];

  while ((procindex != MHempty) &&
         (procname != rt[procindex].mhname)) {
    procindex = rt[procindex].hostnext;
  }

  // Will be MHempty if no name found.
  return(procindex);
}

MHqid MHrt::allocateMHqid(const char* name) {
  MHqid mhqid;
  Short procindex = MHempty;

  pthread_mutex_lock(&m_msgLock);
  m_msgLockCnt ++;
  if ((procindex = findNameOnHost(LocalHostIndex, name)) == MHempty) {
    // Not found, search for name through free area
    int i;
    for (i = MHminTempProc-1; i> MHmaxPermProc; i--, m_nextQid--) {
      if (m_nextQid <= MHmaxPermProc) {
        m_nextQid = MHminTempProc -1;
      }
      if (localdata[m_nextQid].inUse == FALSE &&
          hostlist[LocalHostIndex].indexlist[m_nextQid] == MHempty) {
        localdata[m_nextQid].m_msgs = MHempty;
        mhqid = MHMAKEMHQID(LocalHostIndex, m_nextQid);
        localdata[m_nextQid].inUse = TRUE;
        break;
      }
    }
    if (i == MHmaxPermProc) {
      pthread_mutex_unlock(&m_msgLock);
      printf("AllocateMHqid: No free slots avaliable for name %s", name);
      return(MHnullQ);
    } else {
      m_nextQid--;
      pthread_mutex_unlock(&m_msgLock);
      printf("AllocateMhqid: Found free slot available for name %s(%s)",
             name, mhqid.display());
    }
  } else {
    pthread_mutex_unlock(&m_msgLock);
    mhqid = rt[procindex].mhqid;
    if (localdata[MHQID2QID(mhqid)].pid != MHempty &&
        kill(localdata[MHQID2QID(mhqid)].pid, 0) >= 0) {
      mhqid = MHnullQ;
    }
    printf("AllocateMhqid: re-using slot available for name %s(%s)",
           name, mhqid.display());
  }
  return(mhqid);
}

Bool MHrt::rcvBroadcast(Short qid) {
  return(localdata[qid].rcvBroadcast);
}

Short MHrt::findNameOnGlobal(const Char *name) const {
  MHname procname(name);
  Short key = (Short) (procname.foldKey() % MHtotHashTblSz); // Hash key
  Short procindex = global_hash[key];

  while ((procindex != MHempty) &&
         (procname != rt[procindex].mhname)) {
    procindex = rt[procindex].globalnext;
  }

  return(procindex);
}

int MHrt::match(const MHname &mhname, const MHqid mhqid, const pid_t pid) {
  Short host = MHQID2HOST(mhqid);
  Short qid = MHQID2QID(mhqid);
  if (qid > MHmaxQid || host >= MHmaxHostReg) {
    return (-1);
  }

  if (host == SecondaryHostIndex) {
    host = LocalHostIndex;
  }

  Short elemid = hostlist[host].indexlist[qid];

  if ((host == LocalHostIndex) && (elemid >= 0)) {
    if ((rt[elemid].mhname != mhname) ||
        (localdata[qid].pid != pid)) {
      return (-1);
    } else {
      return(0);
    }
  }
  return(-1);
}

// return 0 and the associated name of a queue that is in
// use, otherwise return -1

Short MHrt::findMhqid(MHqid mhqid, Char *name) const {
  if (MHGETQ(mhqid) < 0) {
    return(GLfail);
  }
  Short host = MHQID2HOST(mhqid);
  Short qid = MHQID2QID(mhqid);

  if (host == SecondaryHostIndex) {
    host = LocalHostIndex;
  }

  Short elemid = hostlist[host].indexlist[qid];

  int ret;
  ret = GLfail;

  // If the entry is not empty, then continue on.

  if (elemid != MHempty) {
    name = rt[elemid].mhname.dump(name);
    ret = GLsuccess;
  }
  return (ret);
}

// This function given the internal MSGH qid value
// finds the "real" process name
GLretVal MHrt::getProcName(Char *name, MHqid mhqid) const {
  Short host = MHQID2HOST(mhqid);
  Short qid = MHQID2QID(mhqid);

  if (host == SecondaryHostIndex) {
    host = LocalHostIndex;
  }

  Short elemid = hostlist[host].indexlist[qid];

  if (elemid == MHempty) {
    return(GLfail);
  } else {
    name = rt[elemid].mhname. dump(name);
    return(GLsuccess);
  }
}

Short MHrt::getLocalHostIndex() const {
  return (LocalHostIndex);
}

const sockaddr_in6 * MHrt::getHostAddr(Short hostnum) const {
  return(&hostlist[hostnum].saddr[hostlist[hostnum].SelectedNet]);
}

Bool MHrt::isHostActive(Short hostnum) const {
  if (hostlist[hostnum].isused) {
    return(hostlist[hostnum].isactive);
  } else {
    return(FALSE);
  }
}

Bool MHrt::isHostUsed(Short hostnum) const {
  return(hostlist[hostnum].isused);
}

// msgsz of 0 indicates just cargo flush, no new message
// if msgs > 0 allocate this much space after send locked
// is true if cuntion is called recursively
GLretVal MHrt::Send(Short hostid, Char *msgp, Long msgsz,
                    Bool resetSeq, Bool buffered, Bool doLock) {
  static MHcargo cmsg;
  // If old style unbufferred messaging is used, just send immediately
  if (!buffered || !m_buffered) {
    ((MHmsgBase*)msgp)->HostId != MHunBufferedFlg;
    // send to the socket
    GLretVal ret = sendto(MHsock_id, (Char *)msgp, msgsz, 0,
                          (sockaddr *)getHostAddr(hostid),
                          sizeof(sockaddr_in6));

    if (ret == msgsz) {
      return(GLsuccess);
    } else {
      return(errno);
    }
  }

  MHhostdata *pHD = &hostlist[hostid];
  GLretVal ret;

  if (doLock) {
    pthread_mutex_lock(&m_lock);
    m_lockcnt++;
  }

  // Check if exceeded window size
  if (!resetSeq &&
      ((pHD->nSend < pHD->nLastAcked ||
        pHD->nSend - pHD->nLastAcked >= pHD->windowSz) &&
       (pHD->nSend >= pHD->nLastAcked ||
        pHD->nLastAcked - pHD->nSend <= MHmaxSQueue - pHD->windowSz))) {
    if (msgsz > 0) {
      pHD->nMeasSendBufFull++;
    }
    int nMsg = pHD->nLastAcked + 1;
    if (nMsg > MHmaxSeq) {
      nMsg = 0;
    }
    int cnt = 0;
    while (cnt <= MHmaxSeq && pHD->sendQ[nMsg].m_length == MHempty) {
      nMsg++;
      cnt++;
      if (nMsg > MHmaxSeq) {
        nMsg = 0;
      }
      pHD->nLastAcked ++;
      if (pHD->nLastAcked > MHmaxSeq) {
        pHD->nLastAcked = 0;
      }
    }

    if (cnt > MHmaxSeq) {
      pHD->nSend = pHD->nLastAcked;
      if (doLock) {
        pthread_mutex_unlock(&m_lock);
      }
      return(MHagain);
    }

    if (msgp != NULL && ((MHmsgBase*)msgp)->msgType == MHhostDelTyp &&
        (++(pHD->nDelMsgs)) > 2) {
      if (pHD->sendQ[nMsg].m_length != MHempty &&
          datafree[pHD->sendQ[nMsg].m_dataidx] == FALSE) {
        MHmsgBase *pMsg;
        pMsg = (MHmsgBase*)&data[MHCHUNKADDR(pHD->sendQ[nMsg].m_dataidx)];
        if (pHD->nDelMsgs > 5) {
          pMsg->seq = nMsg | MHresetMask;
          pHD->nMeasSeqReset++;
        } else {
          pMsg->seq = nMsg;
        }
        pMsg->rSeq = pHD->nRcv;
        // send to the socket
        sendto(MHsock_id, (Char *)pMsg, pMsg->msgSz, 0,
               (sockaddr*)getHostAddr(hostid), sizeof(sockaddr_in6));
      }
    }
    if (doLock) {
      pthread_mutex_unlock(&m_lock);
    }
    return(MHagain);
  }

  pHD->nDelMsgs = 0;
  int i;
  if ((pHD->sendQ[pHD->nSend].m_length == MHempty) && (msgsz > 0) && !resetSeq) {
    int allocSz;
    int cpySz;
    int need_chunks;
    if (msgsz < m_MinCargoSz - sizeof(MHcargo) - sizeof(Long)) {
      allocSz = MHmaxCargoSz;
      cpySz = sizeof(MHcargo);
      MHsetHostId(&cmsg, getActualLocalHostIndex(hostid));
      cmsg.toQue = MHMAKEMHQID(hostid, MHrprocQ);
    } else {
      allocSz = msgsz;
      cpySz = msgsz;
    }

    // Figure out number of chunks needed. This can result in one
    // chunk being wasted if msgsz is a multiple or MHdataChunk
    // but this is overall more efficient
    need_chunks = MHGETNUMCHUNKS(allocSz);
    int got_chunks = 0;
    // Find a contiguous free space
    for (i = 0; i < m_NumChunks && got_chunks < need_chunks;
         i++, m_BufIndex++) {
      if (m_BufIndex >= m_NumChunks) {
        m_BufIndex = 0;
        got_chunks = 0;
      }
      if (datafree[m_BufIndex]) {
        got_chunks ++;
      } else {
        got_chunks = 0;
      }
    }

    if (i == m_NumChunks) {
      m_nMeasTotSendBufFull++;
      if (doLock) {
        pthread_mutex_unlock(&m_lock);
      }
      return (MHagain);
    }

    Char *smsgp = msgp;
    // Put in the cargo header if this is to be cargoed
    if (cpySz == sizeof(MHcargo)) {
      smsgp = (Char*)&cmsg;
    }
    int start_chunk;
    start_chunk = m_BufIndex - need_chunks;
    for (i = start_chunk; i < m_BufIndex; i++) {
      datafree[i] = FALSE;
    }
    m_nMeasUsedOut += need_chunks;
    if (m_nMeasUsedOut > m_nMeasHighOut) {
      m_nMeasHighOut = m_nMeasUsedOut;
    }
    memcpy(&data[MHCHUNKADDR(start_chunk)], smsgp, cpySz);
    pHD->sendQ[pHD->nSend].m_length = allocSz;
    pHD->sendQ[pHD->nSend].m_dataidx = start_chunk;
  } else if (pHD->sendQ[pHD->nSend].m_length == MHempty && msgsz == 0) {
    // Flush called with no data in cargo
    if (doLock) {
      pthread_mutex_unlock(&m_lock);
    }
    return(GLsuccess);
  }

  MHcargo* pCargo;
  if (pHD->sendQ[pHD->nSend].m_length != MHempty) {
    pCargo = (MHcargo*)&data[MHCHUNKADDR(pHD->sendQ[pHD->nSend].m_dataidx)];
  } else {
    pCargo = NULL;
  }
  // Check for space in cargo and if cargo is allocated
  // If cargo is not allocated, just send
  if (pCargo != NULL && pCargo->msgType == MHcargoTyp) {
    if (((pCargo->msgSz + msgsz + sizeof(Long)) < MHmaxCargoSz)) {
      // Add this message to existing cargo
      // Add code to deal with alignment
      if (msgsz > 0) {
        pCargo->m_count ++;
        if ((pCargo->msgSz & MHintAlign) != 0) {
          pCargo->msgSz = (pCargo->msgSz + sizeof(Long)) & MHintAlign;
        }
        memcpy((char*)pCargo + pCargo->msgSz, msgp, msgsz);
        pCargo->msgSz += msgsz;
        // if not ready wait for more messages
        if (!resetSeq &&
            pCargo->m_count < m_nMaxCargo
            && pCargo->msgSz < m_MinCargoSz) {
          if (doLock) {
            pthread_mutex_unlock(&m_lock);
          }
          return(GLsuccess);
        }
      }
      // Cargo is full, send it
      msgp = (char*)pCargo;
      msgsz = pCargo->msgSz;
    } else {
      // Could not fit it in, flush cargo and resend this one
      Send(hostid, NULL, 0, resetSeq, TRUE, FALSE);
      // Call the send with the new message size
      ret = Send(hostid, msgp, msgsz, resetSeq, buffered, FALSE);
      if (doLock) {
        pthread_mutex_unlock(&m_lock);
      }
      return(ret);
    }
  }

  if (msgp == NULL) {
    if (doLock) {
      pthread_mutex_unlock(&m_lock);
    }
    return(MHnoMsg);
  }

  ((MHmsgBase*)msgp)->rSeq = pHD->nRcv;
  if (!resetSeq) {
    ((MHmsgBase*)msgp)->seq = pHD->nSend;
  } else {
    ((MHmsgBase*)msgp)->seq = pHD->nSend | MHresetMask;
  }

  int tmpnSend = pHD->nSend;
  pHD->nSend++;
  if (pHD->nSend > MHmaxSeq) {
    pHD->nSend = 0;
  }

  ret = sendto(MHsock_id, (Char*)msgp, msgsz, 0,
               (sockaddr*)getHostAddr(hostid), sizeof(sockaddr_in6));

  // If the send fails, lose the recrod of this buffer message, waht will
  // happen is the if the socket is not completely hosted up, the next
  // successfull message will be out of sequence so that the far end
  // will reject it, Since, nothing is buffered, we will send a default
  // host message.
  if (ret != msgsz) {
    int need_chunks;
    if (pHD->sendQ[tmpnSend].m_length != MHempty) {
      need_chunks = MHGETNUMCHUNKS(pHD->sendQ[tmpnSend].m_length);
      for (i = pHD->sendQ[tmpnSend].m_dataidx;
           i < pHD->sendQ[tmpnSend].m_dataidx + need_chunks; i++) {
        datafree[i] = TRUE;
      }
      pHD->sendQ[tmpnSend].m_length = MHempty;
    }
    pHD->nMeasSockDropped++;
    if (doLock) {
      pthread_mutex_unlock(&m_lock);
    }
    return(errno);
  }

  pHD->nUnAcked = 0;
  pHD->nMeasMsgSend ++;
  if (doLock) {
    pthread_mutex_unlock(&m_lock);
  }
  return(GLsuccess);
}

Bool MHrt::isCC(const char* name) {
  if ((strncmp(name, "cc", 2) == 0) || m_envType == MH_peerCluster ||
         (m_envType == MH_standard &&
          SecondaryHostIndex != MHnone &&
          strncmp(name, "as", 2) == 0)) {
    return (TRUE);
  } else {
    return(FALSE);
  }
}

void MHrt::sendCargo() {
  int i;
  for (i = 0; i < MHmaxHostReg; i++) {
    if (!hostlist[i].isused) {
      continue;
    }
    Send(i, NULL, 0, FALSE, TRUE, TRUE);
  }
}

Short MHrt::getActualLocalHostIndex(Short hostid) const {
  if (SecondaryHostIndex == MHnone ||
      strncmp(hostlist[hostid].hostname.display(), "as", 2) != 0) {
    return(LocalHostIndex);
  } else {
    return(SecondaryHostIndex);
  }
}

