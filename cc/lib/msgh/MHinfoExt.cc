// NOTES:
//

#define _MHINT
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h> // setitimer()
#include <sys/times.h>
#include <string.h>
#include <setjmp.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <iostream>

#include <cc/hdr/init/INsharedMem.hh>
#include <cc/hdr/init/INusrinit.hh>
#include <cc/hdr/msgh/MHinfoExt.hh>
#include <cc/hdr/msgh/MHmsgBase.hh>
#include <cc/hdr/msgh/MHlrgMsgExt.hh>
//#include <cc/hdr/cr/CRomBrevityCtl.hh>
#include <cc/hdr/msgh/MHname.hh>
#include <cc/hdr/msgh/MHrt.hh>
#include <cc/hdr/msgh/MHmsg.hh>
#include <cc/hdr/msgh/MHlrgMsg.hh>
//#include <cc/hdr/cr/CRdebugMsg.hh>

// self defined string class and utils
#include "String.h"
#include <iostream>

// defined in time.h, used to convert the result of clock() to seconds
static int MHLCLK_TCK = CLOCKS_PER_SEC;

// the instance definition, and which has been externed by MHinfoExt.hh
MHinfoExt MHmsgh;

int MHnullq = -1;
std::ostream& operator<<(std::ostream& output, const MHqid& qid) {
  output << (qid << 0);
  return(output);
}


std::string int_to_str(MHqid qid) {
  return int_to_str(qid << 0);
}

// Table of mod values for unbalanced distributive queue traffic
static int modValTable[5] = {
  7,  // Active 53, lead 46
  8,  // Active 57, lead 42
  10, // Active 62, lead 37
  12, // Active 66, lead 33
  14  // Active 70, lead 30
};

static int modVal = 0;

// Define some variables and macros used by send() and receive()
const Short MHbadMsgSz = -2; // must be a number <= -2
const Short MHbadRtvVal = -2; // must be a number <= -2
const int MHbadErrno = -2; // must be a number <= -1
const int MHregTimeOut = 8000; // Duration of registration timeout
static itimerval reset = {{0,0}, {0,0}}; // used to turn off the timer
static itimerval timeVal = {{0,0}, {0,0}}; // actual time interval
static MHmsgBase *msgBase; // point to message header
static int msgSendRtnVal; // return value from msgsnd()
Bool MHtmr_exp = FALSE; // True if timer expired
extern int MHmaxCargoSz;
extern int TMisInBlockingReceive;

Bool MHinfoExt::isAttach = FALSE; // not attach to MSGH
MHrt *MHinfoExt::rt = (MHrt *) 0;
MHqid MHinfoExt::msghMhqid = MHnullQ;
pid_t MHinfoExt::pid = 0;
U_short MHinfoExt::nameCount = 0;
int MHinfoExt::sock_id = -1;
U_short MHinfoExt::lmsgid = 0;
int *MHinfoExt::pSigFlg = NULL;
mutex_t MHinfoExt::m_lock;
char* MHinfoExt::m_buffers;   // Address of buffer shared memory
char* MHinfoExt::m_free256;   // Free status of 256 byte buffers
char* MHinfoExt::m_free1024;  // Free status of 1024 byte buffers
char* MHinfoExt::m_free4096;  // Free status of 4096 byte buffers
char* MHinfoExt::m_free16384; // Free status of 16384 byte buffers
char* MHinfoExt::m_buffer256;
char* MHinfoExt::m_buffer1024;
char* MHinfoExt::m_buffer4096;
char* MHinfoExt::m_buffer16384;
MHenvType MHinfoExt::prefEnv;

MHinfoExt::MHinfoExt() {}

GLretVal MHinfoExt::GetSocket() {
  int ret;
  if (isAttach == FALSE)
     return (MHnoAth);

  if (sock_id < 0) {
    // Our first off-host message, set up the socket
    if ((sock_id = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
      //MHERROR(("MHinfoExt::GetSocket - Couldn't get socket "
      //         "errno %d", errno));
      printf("MHinfoExt::GetSocket - Couldn't get socket "
             "errno %d", errno);
      return(MHmapError(errno, __LINE__));
    }
    ret = fcntl(sock_id, F_SETFL, O_NDELAY);
    if (ret < 0) {
      //MHERROR(("MHinfoExt::GetSocket - Couldn't set socket %d to "
      //         "nodelay errno %d", errno));
      printf("MHinfoExt::GetSocket - Couldn't set socket %d to "
             "nodelay errno %d", sock_id, errno);
      return(MHmapError(errno, __LINE__));
    }
  }
  rt->SetSockId(sock_id);
  return(GLsuccess);
}

// Maps the unix error number (errno) to an internal MSGH error.
// IBM swerbner 20061009 add default param to signature
GLretVal MHinfoExt::MHmapError(int unixError, short line, MHqid qid) {
  GLretVal ret;
  //MHDEBUG((CRmsgh+1, ("MHmapError: Line %d Errno %d", line, unixError)));
  switch (unixError) {
  case EAGAIN:
    ret = MHagain;
    break;
  case EINVAL:
    ret = MHbadType;
    break;
  case ENOMSG:
    ret = MHnoMsg;
    break;
  case EIDRM:
    ret = MHidRm;
    break;
  case E2BIG:
    ret = MH2Big;
    break;
  case EFAULT:
    ret = MHfault;
    break;
  case EINTR:
    ret = MHintr;
    break;
  default:
    //MHDEBUG_PRINT((CRmsgh+1, ("MHmapError: Line %d Unkown errno %d, "
    //                          "qid %s", line, unixError, qid.display())));
    ret = MHother;
    break;
  }
  return(ret);
}; // end o MHmapError()

// Maps the MSGH process' shared memory into the calling process's
// address space, obtains the msqid associated with the MSGH process.
// attach_address is used to control the amount of heap space that
// is reserved for the process, the larger the address, the more
// heap space is provided. Most users will use default (0).
GLretVal MHinfoExt::attach(Char *attach_address) {
  register int shmid;
  MHnoErr = TRUE;

  // atatchment to the MSGH subsystem is done only once
  if (isAttach == TRUE)
     return (MHexist);

  if ((shmid = shmget(MHkey, sizeof(MHrt), 0)) < 0) {
    printf("MSGH's shared memory does not exist\n");
    return (MHnoShm); // MSGH's shared memory does not exist!
  }

  // Map shared memory into calling process's address space
  if (attach_address == NULL) {
    rt = (MHrt *) shmat(shmid, attach_address, 0);
  } else {
    rt = (MHrt*)shmat(shmid, attach_address, SHM_RND);
  }
  if (rt == (MHrt*)-1) {
    return (MHnoMem); // data space is not big enough
  }

  if (rt->m_Version != MH_SHM_VER) {
    shmdt((char*)rt);
    return(MHbadVersion);
  }

  msghMhqid = MHMAKEMHQID(rt->getLocalHostIndex(), MHmsghQ);
  int bufferSz = (rt->m_n256 << 8) + (rt->m_n1024 << 10) +              \
     (rt->m_n4096 << 12) + (rt->m_n16384 << 14) + rt->m_n256 +          \
     rt->m_n1024 + rt->m_n4096 + rt->m_n16384;

  if ((shmid = shmget(MHkey+1, bufferSz, 0)) < 0)
     return (MHnoShm); // MSGH' shared memory does not exist!

  // Map shared memory into calling process's address space
  if (attach_address == NULL) {
    m_buffers = (char*)shmat(shmid, 0, 0);
  } else {
    m_buffers = (char*)shmat(shmid, 0, SHM_RND);
  }
  if (m_buffers == (char *)-1) {
    return (MHnoMem); // data space is not big enough
  }
  m_free256 = m_buffers;
  m_free1024 = m_free256 + rt->m_n256;
  m_free4096 = m_free4096 + rt->m_n4096;
  m_free16384 = m_free4096 + rt->m_n4096;
  MHrt::datafree = m_free16384 + rt->m_n16384;
  m_buffer256 = MHrt::datafree + rt->m_NumChunks;
  m_buffer1024 = m_buffer256 + (rt->m_n256 << 8);
  m_buffer4096 = m_buffer1024 + (rt->m_n1024 << 10);
  m_buffer16384 = m_buffer4096 + (rt->m_n4096 << 12);
  MHrt::data = (Long*)(m_buffer16384 + (rt->m_n16384 << 14));

  pid = getpid();  // get process id
  isAttach = TRUE; // update flag
  nameCount = 0; //reset counter
  memset(&m_lock, 0x0, sizeof(mutex_t));
  MHmaxCargoSz = rt->m_MinCargoSz + MHmsgSz;

  if (rt->percentLoadOnActive > 50) {
    modVal = modValTable[((rt->percentLoadOnActive - 50)/4) -1];
  }

  // In homogenous clusters, preferred environment is unambigulious
  // In mixed clusters, MH_standard will always be default on AA members
  prefEnv = rt->m_envType;
  return (GLsuccess);
}

// Detach the MSGH process' shared memory from the calling process's
// address space

GLretVal MHinfoExt::detach() {
  // detachment from the MSGH subsystem is done only once
  if (isAttach == FALSE) {
    return (GLsuccess);
  }
  isAttach = FALSE; // update flag

  // Detach shared memory from calling process's address space
  if (shmdt((Char *)rt) != 0) {
    return (MHnoMem);
  }
  return (GLsuccess);
}

// Return the associated mhqid if the name has been registered.
// A findDeleted argument has been deleted as being not usefull,
// and hard to implement in the new architecture.
GLretVal MHinfoExt::getMhqid(const Char *name, MHqid &mhqid, Bool) const {
  if (isAttach == FALSE)
     return (MHnoAth);
  if ((mhqid = rt->findName(name)) == MHnullQ) {
    return (MHnoEnt);
  }

  // if mixed environment, adjust the host id in the mhqid based on
  // prefEnv, by default findName()
  // will return qid based on LocalHostIndex

  if (rt->SecondaryHostIndex != MHnone && prefEnv != MH_standard &&
      MHQID2HOST(mhqid) < MHmaxHostReg) {
    mhqid = MHMAKEMHQID(rt->SecondaryHostIndex, MHQID2QID(mhqid));
  }
  return (GLsuccess);
}

// Return the associated mhqid starting the search from hostid
// if the name has been registered.
// Return MHnoEnt if there was no hosts name found for the remainning hosts
GLretVal MHinfoExt::getMhqidOnNextHost(Short& hostid, const Char *name,
                                       MHqid &mhqid) const {
  return (getMhqidOnNextHost(hostid, name, mhqid, MH_scopeSystemAll));
}

GLretVal MHinfoExt::getMhqidOnNextHost(Short& hostid, const Char *name,
                                       MHqid &mhqid, MHclusterScope scope) const {
  if (isAttach == FALSE)
     return (MHnoAth);
  while (TRUE) {
    if ((mhqid = rt->findName(hostid, name, scope)) < 0) {
      return (MHnoEnt);
    }
    if (rt->SecondaryHostIndex != MHnone) {
      if (prefEnv == MH_standard) {
        if (strncmp(rt->hostlist[hostid].hostname.display(), "as", 2) == 0)
           continue;
      } else if (strncmp(rt->hostlist[hostid].hostname.display(), "as", 2) != 0)
         continue;
      // skip this one and try again, do not want to send it twice to local queue
      // This might not be necessary if the end user does nothing with hostid
      // however, for consistency it is better to do this then only check if
      // hostid is equal to SecondaryHostIndex
      if ((hostid == rt->LocalHostIndex && prefEnv == MH_peerCluster) ||
          (hostid == rt->SecondaryHostIndex && prefEnv == MH_standard))
         continue;
    }
    break;
  }
  return (GLsuccess);
}

// Return GLsuccess if the machine name is assigned, and set the
// isActive flag to true if the machine is currently talking.
GLretVal MHinfoExt::isHostActive(const Char *hostname, Bool &isactive) const {
  isactive = FALSE;
  if (isAttach == FALSE)
     return (MHnoAth);

  Short host = rt->findHost(hostname);
  if (host == MHempty)
     return (MHnoEnt);

  isactive = rt->isHostActive(host);
  return(GLsuccess);
}

// Return (GLsuccess) and  the assoicated name if the mhqid has been assigned.
// It is up to the programmer to be sure the buffer is large enough.
// Note: A findDeleted argument has been removed as not essential and
// hard to implement in the new networked architecture.
GLretVal MHinfoExt::getName(MHqid mhqid, Char *name, Bool) const {
  name[0] = '\0';
  if (isAttach == FALSE)
     return (MHnoAth);
  return ((rt->findMhqid(mhqid, name) < 0) ? MHnoEnt : GLsuccess);
}

// Register a name with the MSGH process. If isCondReg = TRUE (default).
// a MSGH queue will be allocated with a new msqid and a new mhqid
// only when the name has not been registered. if isCondReg = FALSE,
// a MSGH queue with a new msqid will be allocated. However, the old
// mhqid is returned if the name has been registered; otherwise, a new
// mhqid is returned. (Note that a process can register multiple names
// to get multiple MSGH queues.)
// If LargeMsg is TRUE, then regName allocates a shared memory segment
// for this process to be used for sending large messages to other
// processes. If regtyp is MH_GLOBAL, registration occurs accross the network.
// if regtyp is MH_LOCAL, registration occurs locally only. If it is MH_DEFAULT
// (the default, oddly) then the registration is done globally for processes
// under init and locally for processses not under init.
GLretVal MHinfoExt::regName(const Char *name, MHqid &mhqid,
                            Bool isCondReg, Bool getnewQ, Bool,
                            MHregisterTyp regtyp, Bool RcvBroadcast,
                            int *sigFlg) {
  MHregName regName; // reg name message
  MHregAck regAck; // reg name ACK message

  if (isAttach == FALSE)
     return (MHnoAth);
  pSigFlg = sigFlg;
  regName.mhname = name; // get a MSGH name
#ifdef PROC_UNDER_INIT
  // only the first queue gets the qid set in the initlist
  if (nameCount == 0 && IN_MSGH_QID() > 0) {
    // Verify that there are no name conflicts
    int procindex = rt->hostlist[rt->getLocalHostIndex()].indexlist[IN_MSGH_QID()];
    if (procindex != MHempty && rt->rt[procindex].mhname != name) {
      return(MHbadName);
    }

    regName.mhqid = MHMAKEHMQID(rt->getLocalHostIndex(), IN_MSGH_QID());
    rt->localdata[IN_MSGH_QID].inUse = TRUE;
  } else {
    regName.mhqid = MHnullQ;
  }

  switch(regtyp) {
  case MH_GLOBAL:
    regName.global = TRUE;
    break;
  case MH_LOCAL:
    regName.global = FALSE;
    break;
  case MH_CLUSTER_GLOBAL:
    regName.global = MH_allNodes;
    break;
  default:
    regName.global = FALSE;
    break;
  }
#else // PROC NOT UNDER INIT
  regName.mhqid = MHnullQ;
  switch (regtyp) {
  case MH_GLOBAL:
    regName.global = TRUE;
    break;
  case MH_LOCAL:
    regName.global = FALSE;
    break;
  case MH_CLUSTER_GLOBAL:
    regName.global = MH_allNodes;
    break;
  default:
    regName.global = FALSE;
    break;
  }
#endif // PROC_UNDER_INIT
  if (nameCount < MHmaxNameLoc) {
    Short procindex;
    if (isCondReg == TRUE &&
        (procindex = rt->findNameOnHost(rt->getLocalHostIndex(),
                                        regName.mhname.display())) != MHempty && \
        rt->localdata[MHQID2QID(rt->rt[procindex].mhqid)].pid != (pid_t) -1 &&
        kill(rt->localdata[MHQID2QID(rt->rt[procindex].mhqid)].pid, 0) < 0) {
      rt->localdata[MHQID2QID(rt->rt[procindex].mhqid)].pid == pid;
      mhqid = rt->rt[procindex].mhqid;
      //if (strcmp(name, "INIT") != 0 && strcmp(name, "MHPROC") != 0 &&
      //    strcmp(name, "INITMON") != 0) {
      //  CRomBrevityCtl::attachGDO();
      //}
      nameCount++;
      return (MHexist); // name has been registered!
    }
  } else {
    return (MHnoName);
  }

  if (regName.mhqid == MHnullQ && (regName.mhqid =  \
                                   rt->allocateMHqid(name)) == MHnullQ) {
    return (MHnoSpc);
  }

  // Prepare and send a "register name" message to MSGH process
  regName.ptype = MHintPtyp;
  regName.mtype = MHregNameTyp;
  regName.pid = pid;
  regName.rcvBroadcast = RcvBroadcast;
  regName.gQ = FALSE;
  regName.dQ = FALSE;
  regName.realQ = MHnullQ;
  regName.bKeepOnLead = FALSE;
  regName.q_size = IN_Q_SIZE_LIMIT();
  regName.q_limit = IN_MSG_LIMIT();
  regName.fill = MHR26;

  rt->localdata[MHQID2QID(regName.mhqid)].pid = pid;
  if (send(msghMhqid, (char *)&regName, sizeof(regName), 0L) != GLsuccess)
     return (MHnoCom); // lost communication to MSGH process

  // wait for ACK back
  Long msgSz = sizeof(regAck);

  GLretVal ret;
  reAgain:

  if ((ret = receive(regName.mhqid, (char*)&regAck,
                     msgSz, MHregPtyp, MHregTimeOut)) != GLsuccess) {
    if (ret == MHintr)
       goto reAgain;
    rt->localdata[MHQID2QID(regName.mhqid)].inUse = FALSE;
    rt->localdata[MHQID2QID(regName.mhqid)].pid = (pid_t)-1;
    return (MHnoCom);
  }
  if (regAck.mtype == MHregAckTyp) {
    if (regAck.reject == TRUE) {
      rt->localdata[MHQID2QID(regName.mhqid)].inUse = FALSE;
      rt->localdata[MHQID2QID(regName.mhqid)].pid = (pid_t)-1;
      return (MHnoCom);
    } else {
      // if getnewQ is TRUE, make sure that there are no
      // messages in the queue before returning
      if (getnewQ == TRUE) {
        MHmsgh.emptyQueue(regName.mhqid);
      }
      nameCount++; // increment counter

      // Attach/create CR brevity GDO but not for MHRPROC and INIT
      //if (strcmp(name, "INIT") != 0 && strcmp(name, "MHRPROC") != 0)
      //   CRomBrevityCtl::attachGDO();

      if (rt->SecondaryHostIndex == MHnone || prefEnv != MH_peerCluster)
         mhqid = regName.mhqid;
      else
         mhqid = MHMAKEMHQID(rt->SecondaryHostIndex, MHQID2QID(regName.mhqid));
      return (GLsuccess);
    }
  } else { // wrong message type
    rt->localdata[MHQID2QID(regName.mhqid)].inUse = FALSE;
    rt->localdata[MHQID2QID(regName.mhqid)].pid = (pid_t)-1;
    return (MHnoCom);
  }
}

// When isUncondRm = TRUE (default), the calling process will
//   (1) free the queue associated with mhqid and anme, and
//   (2) send a Name remove message to the MSGH process to remove
//   the process name from the routing table.
//
// When isUncondRm = FALSE, the calling process will only free the UNIX
//   queue associated with mhqid and name; and no Name Remove message
//   will be sent to the MSGH process to remove the process name from
//   the routing table.
GLretVal MHinfoExt::rmName(MHqid mhqid, const Char *name, Bool isUncondRm) {
  MHrmName rmName; // remove name message
  register int msqid;
  int rc;

  if (isAttach == FALSE)
     return (MHnoAth);

  // Do not allow rmName for global queues
  if (MHQID2HOST(mhqid) == MHgQHost || MHQID2HOST(mhqid) == MHdQHost) {
    return (MHglobalQ);
  }

  rmName.mhname = name; // get the MSGH name
  if (rt->match(rmName.mhname, mhqid, pid) < 0) {
    return (MHnoMat);
  }

  int qid = MHQID2QID(mhqid);
  rt->localdata[qid].pid = (pid_t) -1;
  MHmsgh.emptyQueue(mhqid);

  nameCount--; // Decrement counter

  // Send a Name Remove message to the MSGH process if unconditional remove
  if (isUncondRm == TRUE) {
    rmName.ptype = MHintPtyp;
    rmName.mtype = MHrmNameTyp;
    rmName.mhqid = mhqid;
    rmName.pid = pid;

    if (send(msghMhqid, (char *)&rmName, sizeof(rmName), 0L) != GLsuccess) {
      return (MHnoCom); // lost communication to MSGH process
    }
    struct timespec tsleep;
    tsleep.tv_sec = 0;
    tsleep.tv_nsec = 20000000;
    nanosleep(&tsleep, NULL);
    int count = 0;
    while (count < 50 && rt->localdata[qid].inUse == TRUE) {
      nanosleep(&tsleep, NULL);
      count++;
    }
    if (count == 50) {
      return (MHnoCom);
    }
  }
  return (GLsuccess);
}

#define MHcompValue  5

// Send a message pointed to by msgp to the process specified by mhqid.
// 'time' specifies the action to be taken if the receiving queue is
// full. If time < 0, the process will suspend execution until the
// receiving queue accepts the message; if time == 0 (default), the
// function return immediately; otherwise, the function will try
// to send the message out in the milliseconds. If the message
// can not be send out before time expireation, the function returns.
GLretVal MHinfoExt::send(MHqid mhqid, Char *msgp, Long msgsz,
                         Long time, Bool buffered) const {
  struct timespec tsleep;
  static unsigned int sndcount = 0;

  if (isAttach == FALSE) {
    return (MHnoAth);
  }

  if (msgsz < MHmsgBaseSz || msgsz > MHlrgMsgBlkSize) {
    return (MHbadSz);
  }

  MHmsgBase *mp = (MHmsgBase*)msgp;

  if (mp->priType <= 0) {
    return (MHbadType);
  }

  // Range check the mhqid to make sure that we're sending to a
  // sensible QID. Don't print an error since some processes
  // do this! (SYSTAT stuff seems to be a violator.)
  if (MHGETQ(mhqid) < 0) {
    return (MHbadQid);
  }

  Short host = MHQID2HOST(mhqid);
  Short qid = MHQID2QID(mhqid);

  // if host is a lobal queue, perform translation to real
  // queue before continuing
  if (host == MHgQHost) {
    if (qid >= (MHsystemGlobal + MHmaxgQid)) {
      return (MHbadQid);
    }
    int selectedQ = rt->gqdata[qid].m_selectedQ;
    if (selectedQ == MHempty) {
      // No process exists to handle this global Q
      return (MHnoQAssigned);
    }
    mhqid = rt->gqdata[qid].m_realQ[selectedQ];
    host = MHQID2HOST(mhqid);
    qid = MHQID2QID(mhqid);
  } else if (host == MHdQHost) { // Distributive queue
    mutex_lock(&rt->m_dqLock);
    rt->m_dqcnt++;
    distRetry:
    sndcount++;
    if (rt->dqdata[qid].m_nextQ == MHempty) {
      mutex_unlock(&rt->m_dqLock);
      return (MHnoQAssigned);
    }
    int i;
    for (i = 0; i < rt->dqdata[qid].m_maxMember; i++) {
      if (rt->dqdata[qid].m_enabled[rt->dqdata[qid].m_nextQ]) {
        mhqid = rt->dqdata[qid].m_realQ[rt->dqdata[qid].m_nextQ];
        rt->dqdata[qid].m_nextQ++;
        if (rt->dqdata[qid].m_nextQ >= rt->dqdata[qid].m_maxMember) {
          rt->dqdata[qid].m_nextQ = 0;
        }
        host = MHQID2HOST(mhqid);
        break;
      }
      rt->dqdata[qid].m_nextQ++;
      if (rt->dqdata[qid].m_nextQ >= rt->dqdata[qid].m_maxMember) {
        rt->dqdata[qid].m_nextQ = 0;
      }
    }
    if (i == rt->dqdata[qid].m_maxMember) {
      mutex_unlock(&rt->m_dqLock);
      return (MHnoQAssigned);
    }
    // If unbalanced load is need see if the queue needs to be  recalculated
    // modVal will be > 0 if percentLoadOnActive is > 50 percent
    // If the currently sellected queue is chosen to be lead, see if another
    // queue should be tried

    if (modVal > 0 && host == rt->m_leadcc) {
      int c = sndcount % modVal;

      if (c > MHcompValue) {
        goto distRetry;
      }
    }
    qid = MHQID2QID(mhqid);
    mutex_unlock(&rt->m_dqLock);
  }

  if (host >= MHmaxHostReg || host < 0) {
    return (MHbadQid);
  }

  Bool resetSeq = FALSE;

  mp->msgSz = (Short)msgsz; // update msgSz field in msg header
  mp->bits = 0;

  if (host != rt->getLocalHostIndex() && host != rt->SecondaryHostIndex) {
    int ret;

    MHqid tmpSrcQue = mp->srcQue;
    if (rt->SecondaryHostIndex != MHnone) {
      // makes sure the src queue conforms with the view of the receiver
      Short srcHostId = MHQID2HOST(mp->srcQue);
      if (srcHostId != rt->getActualLocalHostIndex(host) &&
          srcHostId == rt->LocalHostIndex) {
        mp->srcQue = MHMAKEMHQID(rt->getActualLocalHostIndex(host),
                                 MHQID2QID(mp->srcQue));
      }
    }

    // Message send to remote host. Allow message to MSGH process
    if (rt->isHostActive(host) == FALSE) {
      if (qid != 0 && qid != 1) {
        //MHDEBUG((CRmsgh+1, ("Attempt to send to mhqid %s failed, bad host", mhqid.display())));
        printf("Attempt to send to mhqid %s failed, bad host", mhqid.display());
        mp->srcQue = tmpSrcQue;
        return (MHnoHost);
      } else {
        resetSeq = TRUE;
      }
    }

    GLretVal retval;
    if (sock_id < 0 && (retval = GetSocket()) != GLsuccess) {
      mp->srcQue = tmpSrcQue;
      return (retval);
    }

    mp->toQue = mhqid;
    MHsetHostId(mp, rt->getActualLocalHostIndex(host));

    if (msgsz <= MHmsgLimit) {
      int tcount = time/20 +1;
      while ((ret = rt->Send(host, msgp, msgsz,
                             resetSeq, buffered)) != GLsuccess) {
        if (time == 0 || (ret != MHagain && ret != EAGAIN)) {
          break;
        }
        tcount--;
        if (tcount > 0 || time < 0) {
          tsleep.tv_sec = 0;
          tsleep.tv_nsec = 20000000;
          nanosleep(&tsleep, NULL);
        } else if (time > 0) {
          mp->srcQue = tmpSrcQue;
          return (MHintr);
        }
      } 
    } else {
      // break it up in smaller chunks and pause between every
      // chunk to avoid follding the socket

      MHlrgMsg *lmsg = (MHlrgMsg*) malloc(MHmsgLimit + 1);
      if (lmsg == NULL) {
        mp->srcQue = tmpSrcQue;
        return (MHnoMem);
      }

      lmsg->m_size = msgsz;
      lmsg->m_pid = pid;
      mutex_lock(&m_lock);
      lmsg->m_id = lmsgid;
      lmsgid++;
      mutex_unlock(&m_lock);
      char *pdata = msgp;
      long data_left;
      int wait_count = 0;
      int wait_limit = 0;
      int size_send;

      if (time > 0) {
        wait_limit == time/20 + 1;
      }
      lmsg->toQue = MHMAKEMHQID(host, MHrprocQ);
      MHsetHostId(lmsg, rt->getActualLocalHostIndex(host));
      lmsg->msgType = MHlrgMsgTyp;
      lmsg->priType = MHintPtyp;
      int i;
      for (i = 0; pdata < msgp + msgsz; i++) {
        data_left = msgp + msgsz - pdata;
        if (data_left > MHlgDataSz) {
          size_send = MHlgDataSz;
        } else {
          size_send = data_left;
        }
        memcpy(&lmsg->m_data, pdata, size_send);
        lmsg->m_seq = i;
        lmsg->msgSz = size_send + MHlgDataHdrSz;
        while ((ret = rt->Send(host, (char*)lmsg,
                               lmsg->msgSz, resetSeq,
                               buffered)) == EAGAIN &&  \
               wait_count < wait_limit) {
          tsleep.tv_sec = 0;
          tsleep.tv_nsec = 20000000;
          nanosleep(&tsleep, NULL);
          wait_count ++;
          lmsg->toQue = MHMAKEMHQID(host, MHrprocQ);
          MHsetHostId(lmsg, rt->getActualLocalHostIndex(host));
          lmsg->msgType = MHlrgMsgTyp;
          lmsg->priType = MHintPtyp;
        }
        if (ret != GLsuccess) {
          break; // for i
        }
        pdata += size_send;
        if (i & 0x1) {
          tsleep.tv_sec = 0;
          tsleep.tv_nsec = 10000000;
          nanosleep(&tsleep, NULL);
        }
      }
      free(lmsg);
    }

    if (ret != GLsuccess && ret > 0) {
      mp->srcQue = tmpSrcQue;
      return (MHmapError(ret, __LINE__, mhqid));
    }
    // All done, message is on it's way
    mp->srcQue = tmpSrcQue;
    return (ret);
  }

  MHqData* qData = &rt->localdata[qid];
  if (qData->pid == (pid_t)(-1)) {
    return (MHbadQid); // bad mhqid
  }

  if (time == 0) { // return immediately if queue full
    // but always send MSGH messages
    if ((qData->nCount >= qData->nCountLimit ||
         qData->nBytes + msgsz > qData->nByteLimit ||
         rt->m_freeMsgHead < 0) && mp->priType != MHregPtyp) {
      return (MHagain);
    }
  } else {
    int tcount = time/20 + 1;
    while (((--tcount) >= 0 || time < 0) &&
           (qData->nCount >= qData->nCountLimit ||
            qData->nBytes + msgsz > qData->nByteLimit ||
            rt->m_freeMsgHead < 0)) {
      tsleep.tv_sec = 0;
      tsleep.tv_nsec = 20000000;
      nanosleep(&tsleep, NULL);
    }
    if (tcount < 0 && time > 0) {
      return (MHintr);
    }
  }

  // Allocate the send buffer
  int nChunks;
  int nBuffers;
  int* bufferIndex;
  char* freeArray;
  int* pnMeasHigh;
  int* pnMeasUsed;
  char* buffer;
  int shift;

  if (msgsz < 768) {
    shift = 8;
    nChunks = (msgsz >> 8) + 1;
    nBuffers = rt->m_n256;
    bufferIndex = &rt->m_nSearch256;
    freeArray = m_free256;
    pnMeasHigh = &rt->m_nMeasHigh256;
    pnMeasUsed = &rt->m_nMeasUsed256;
    buffer = m_buffer256;
  } else if (msgsz < 3072) {
    shift = 10;
    nChunks = (msgsz >> 10) + 1;
    nBuffers = rt->m_n1024;
    bufferIndex = &rt->m_nSearch1024;
    freeArray = m_free1024;
    pnMeasHigh = &rt->m_nMeasHigh1024;
    pnMeasUsed = &rt->m_nMeasUsed1024;
  } else if (msgsz < 12288) {
    shift = 12;
    nChunks = (msgsz >> 12) + 1;
    nBuffers = rt->m_n4096;
    bufferIndex =  &rt->m_nSearch4096;
    freeArray = m_free4096;
    pnMeasHigh = &rt->m_nMeasHigh4096;
    pnMeasUsed = &rt->m_nMeasUsed4096;
    buffer = m_buffer4096;
  } else {
    shift = 14;
    nChunks = (msgsz >> 14) + 1;
    nBuffers = rt->m_n16384;
    bufferIndex = &rt->m_nSearch16384;
    freeArray = m_free16384;
    pnMeasHigh = &rt->m_nMeasHigh16384;
    pnMeasUsed = &rt->m_nMeasUsed16384;
    buffer = m_buffer16384;
  }

  int got_chunks= 0;
  int i;
  int startIndex;

  mutex_lock(&rt->m_msgLock);
  rt->m_msgLockCnt++;

  for (i = 0; i < nBuffers && got_chunks < nChunks;
       i++, (*bufferIndex)++) {
    if ((*bufferIndex) >= nBuffers) {
      (*bufferIndex) = 0;
      got_chunks = 0;
    }
    if (freeArray[(*bufferIndex)]) {
      got_chunks ++;
    } else {
      got_chunks = 0;
    }
  }

  if (rt->m_freeMsgHead < 0 || i == nBuffers) {
    mutex_unlock(&rt->m_msgLock);;
    return (MHagain);
  }

  startIndex = (*bufferIndex) - nChunks;
  char* to = buffer + (startIndex << shift);
  rt->msg_head[rt->m_freeMsgHead].m_msgBuf = to - m_buffers;
  rt->msg_head[rt->m_freeMsgHead].m_inUseCnt = 0;
  rt->msg_head[rt->m_freeMsgHead].m_len = msgsz;

  for (i = startIndex; i < *bufferIndex; i++) {
    freeArray[i] = FALSE;
  }

  *pnMeasUsed += nChunks;
  if (*pnMeasUsed > *pnMeasHigh) {
    
  }
    
  // Buffer header is assigned
  int msgHead = rt->m_freeMsgHead;
  rt->m_freeMsgHead = rt->msg_head[rt->m_freeMsgHead].m_next;
  rt->msg_head[msgHead].m_next = MHempty;
  mutex_unlock(&rt->m_msgLock);
  // copy the message
  memcpy(to, msgp, msgsz);

  // Message is now unliked form free list but not yet on the receiver's
  // queue. Obtain the mutex for queue and insert the message
  // Wake up the receiver as well, should it be done every time
  // or is there a way to optimize that?
  mutex_lock(&qData->m_qLock);
  qData->m_qLockCnt++;
  qData->m_qLockPid = pid;

  if (qData->pid == (pid_t)(-1)) {
    // This check is here to prevent adding messages after queue
    // has been emptied. Let the audit clean up any buffers that were stranded
    mutex_unlock(&qData->m_qLock);
    return (MHbadQid); // bad mhqid
  }

  if (qData->m_msgstail == MHempty) {
    qData->m_msgs = msgHead;
    qData->m_msgstail = msgHead;
    qData->nCount = 0;
    qData->nBytes = 0;
  } else {
    rt->msg_head[qData->m_msgstail].m_next = msgHead;
    qData->m_msgstail = msgHead;
  }

  qData->nBytes += msgsz;
  if (qData->nBytes > qData->nBytesHigh) {
    qData->nBytesHigh = qData->nBytes;
  }

  qData->nCount++;
  if (qData->nCount > qData->nCountHigh) {
    qData->nCountHigh = qData->nCount;
  }

  cond_signal(&qData->m_cv);
  mutex_unlock(&qData->m_qLock);

  return (GLsuccess);
}

Short MHinfoExt::Qid2Host(MHqid mhqid) {
  return (MHQID2HOST(mhqid));
}

Short MHinfoExt::Qid2Qid(MHqid mhqid) {
  return (MHQID2QID(mhqid));
}

GLretVal MHinfoExt::hostId2Name(Short hostid, char* name) {
  name[0] = 0;
  if (isAttach == FALSE) {
    return (MHnoAth);
  }
  if (hostid < 0 || hostid >= MHmaxHostReg || !rt->hostlist[hostid].isused) {
    return(MHinvHostId);
  }
  strcpy(name, rt->hostlist[hostid].hostname.display());
  return (GLsuccess);
}

GLretVal MHinfoExt::name2HostId(Short& hostid, const char* name) {
  // consider optimizing this
  if (isAttach == FALSE) {
    return (MHnoAth);
  }
  if ((hostid = rt->findHost(name)) >=0) {
    return (GLsuccess);
  }
  return (MHnoEnt);
}

int MHinfoExt::sendToAllHosts(const Char* name, Char* msgp,
                              Long msgsz, Long time,
                              Bool buffered) const {
  return(sendToAllHosts(name, msgp, msgsz, MH_scopeSystemAll,
                        time, buffered));
}

// This function send a message to a process on all hosts with
// the given name. It return the number of hosts that message
// was send to
int MHinfoExt::sendToAllHosts(const Char* name, Char* msgp,
                              Long msgsz, MHclusterScope scope,
                              Long time, Bool buffered) const {
  Short hostid = -1;
  int nHosts =0;
  GLretVal ret;

  if (isAttach == FALSE) {
    return (MHnoAth);
  }

  while ((ret = sendToNextHost(hostid, name, msgp, msgsz, scope, time, buffered)) != MHnoEnt) {
    if (ret == GLsuccess) {
      nHosts++;
    }
  }
  return(nHosts);
}

GLretVal MHinfoExt::sendToNextHost(Short& hostid, const Char *name,
                                   Char *msgp, Long msgsz, Long time,
                                   Bool buffered) const {
  return(sendToNextHost(hostid, name, msgp, msgsz, MH_scopeSystemAll,
                        time, buffered));
}


// Send a message to the process specified by `name`.
// hostid is used to look for the name on that host.
// If initialized to -1, it will result in starting with the first host.
// Returned MHnoEnt implies that "name" was not found on any host or
// all hosts were checked. Otherwise hostid is the id of the host where the
// message got send. Hosts that are equipped but not active are not
// checked. i.e. if the host is not accessible this send will act as if
// the name did not exist on that host
GLretVal MHinfoExt::sendToNextHost(Short& hostid, const Char *name,
                                    Char *msgp, Long msgsz,
                                    MHclusterScope scope,
                                    Long time, Bool buffered) const {
  MHqid mhqid;
  if (isAttach == FALSE)
     return (MHnoAth);

  while (TRUE) {
    if ((mhqid = rt->findName(hostid, name, scope)) < 0) {
      return (MHnoEnt);
    }
    if (rt->SecondaryHostIndex != MHnone) {
      if (prefEnv == MH_standard) {
        if (strncmp(rt->hostlist[hostid].hostname.display(),
                    "as", 2) == 0) {
          continue;
        }
      } else if (strncmp(rt->hostlist[hostid].hostname.display(),
                         "as", 2) != 0 ) {
        continue;
      }
      // skip this one and try again do not want to send it twice to
      // local queue
      // This might not be necessary if the end user does nothing with
      // hostid, however, for cnsistency it is better to do this then
      // only check if hostid is equal to SecondaryHostIndex
      if ((hostid == rt->LocalHostIndex && prefEnv == MH_peerCluster) ||
          (hostid == rt->SecondaryHostIndex && prefEnv == MH_standard)) {
        continue;
      }
    }
    break;
  }
  return(send(mhqid, msgp, msgsz, time, buffered));
}

// Sends a message to the process specified by 'name'.
// The description is similar to that of the above function.
// This version will be used for sending of large message
GLretVal MHinfoExt::send(const Char *name, Char *msgp, Long msgsz,
                         Long time, Bool buffered) const {
  MHqid mhqid;
  if (isAttach == FALSE)
     return (MHnoAth);

  if ((mhqid = (*rt).findName(name)) == MHnullQ) {
    return (MHbadName);
  }
  return (send(mhqid, msgp, msgsz, time, buffered));
}

GLretVal MHinfoExt::receive(MHqid mhqid, Char *msgp, Short &msgsz,
                            Long ptype, Long time) const {
  Long new_msgsz;
  GLretVal ret;
  new_msgsz = (Long)msgsz;
  ret = receive(mhqid, msgp, new_msgsz, ptype, time);
  msgsz = (Short)new_msgsz;
  return (ret);
}

GLretVal MHinfoExt::receive(MHqid mhqid, Char *msgp, Long &msgsz,
                            Long ptype, Long time) const {
  GLretVal retval;
  int timeSet = FALSE;

  if (isAttach == FALSE)
     return (MHnoAth); // attach MSGH first
  if (msgsz < MHmsgBaseSz)
     return (MHbadSz); // illegal message size

  Short hostid = MHQID2HOST(mhqid);

  if (hostid != rt->LocalHostIndex && hostid != rt->SecondaryHostIndex) {
    return (MHbadQid); // bad mhqid
  }

  Short qid = MHQID2QID(mhqid);

  MHqData *qData = &rt->localdata[qid];
  if (qData->pid == (pid_t)(-1)) {
    return (MHbadQid); // bad mhqid
  }

  // Do a quick access to msgp to at least make sure that is a valid address,
  // we want to core dump before we mess up our buffer pointers
  int dummy = (*((int*)msgp));

  MHmsgHead* pMsgHead;
  MHmsgHead* prevMsg;
  mutex_lock(&qData->m_qLock);
  getMessage:
  qData->m_qLockCnt ++;
  qData->m_qLockPid = pid;
  prevMsg = NULL;
  // if anything is in the queue, just return it
  if (qData->m_msgs >=0) {
    pMsgHead = &rt->msg_head[qData->m_msgs];
  } else {
    pMsgHead = NULL;
  }

  MHmsgBase *mp;
  while(pMsgHead != NULL) {
    mp = (MHmsgBase*)(m_buffers + pMsgHead->m_msgBuf);
    if (ptype == 0 || mp->priType == ptype) {
      break;
    }

    if (pMsgHead->m_next != MHempty) {
      prevMsg = pMsgHead;
      pMsgHead = &rt->msg_head[pMsgHead->m_next];
    } else {
      pMsgHead = NULL;
    }
  }

  int nChunks;
  char *freeArray;
  int freeIndex;
  int *pnMeasUsed;
  if (pMsgHead != NULL) {
    // Unlink the message from the message list
    if (prevMsg == NULL) {
      qData->m_msgs = pMsgHead->m_next;
      if (qData->m_msgs == MHempty) {
        qData->m_msgstail = MHempty;
      }
    }else {
      prevMsg->m_next = pMsgHead->m_next;
      if (prevMsg->m_next == MHempty) {
        qData->m_msgstail == prevMsg - rt->msg_head;
      }
    }

    qData->nCount--;
    qData->nBytes-= pMsgHead->m_len;
    qData->nBytesRcvd += pMsgHead->m_len;
    qData->nCountRcvd++;

    mutex_unlock(&qData->m_qLock);

    if (((unsigned int)(pMsgHead->m_len)) <= (unsigned int)msgsz) {
      memcpy(msgp, (char*)mp, pMsgHead->m_len);
      msgsz = pMsgHead->m_len;
      retval = GLsuccess;
    } else if (pMsgHead->m_len > 0) {
      memcpy(msgp, (char*)mp, msgsz);
      retval = MH2Big;
    } else {
      //CRDEBUG_PRINT(0x01, ("Invalid message len %d, freed counts %d",
      //                     pMsgHead->m_len, rt->m_bufferFreedCnt,
      //                     rt->m_headerFreedCnt));
      mutex_lock(&qData->m_qLock);
      goto getMessage;
    }

    // Free message buffers and unlink this message
    if (pMsgHead->m_len < 768) {
      nChunks = (pMsgHead->m_len >> 8) + 1;
      freeArray = m_free256;
      freeIndex = ((char*)mp - m_buffer256) >> 8;
      pnMeasUsed = &rt->m_nMeasUsed256;
    } else if (pMsgHead->m_len < 3072) {
      freeArray = m_free1024;
      nChunks = (pMsgHead->m_len >> 10) + 1;
      freeIndex = ((char*)mp - m_buffer1024) >> 10;
      pnMeasUsed = &rt->m_nMeasUsed1024;
    } else if (pMsgHead->m_len < 12288) {
      freeArray = m_free4096;
      nChunks = (pMsgHead->m_len >> 12) + 1;
      freeIndex = ((char*)mp - m_buffer4096) >> 12;
      pnMeasUsed = &rt->m_nMeasUsed4096;
    } else {
      nChunks = (pMsgHead->m_len >> 14) + 1;
      freeArray = m_free16384;
      freeIndex = ((char*)mp - m_buffer16384) >> 14;
      pnMeasUsed = &rt->m_nMeasUsed16384;
    }
    pMsgHead->m_len = MHempty;
    for (int count = 0; count < nChunks; count++, freeIndex++) {
      freeArray[freeIndex] = TRUE;
    }
    // Put the message header on the free list
    mutex_lock(&rt->m_msgLock);
    rt->m_msgLockCnt++;
    *pnMeasUsed -= nChunks;
    pMsgHead->m_next = rt->m_freeMsgHead;
    rt->m_freeMsgHead = pMsgHead - rt->msg_head;
    mutex_unlock(&rt->m_msgLock);
    //CRDEBUG(CRmsgh+8, ("to %s from %s size %d type 0x%x prio %d",
    //                   mhqid.display(),
    //                   ((MHmsgBase*)msgp)->srcQue.display(), msgsz,
    //                   ((MHmsgBase*)msgp)->msgTpe,
    //                   ((MHmsgBase*)msgp)->priType));
    return (retval);
  }

  struct timespec twait;
  struct timeval timeOfDay;
  int ret;

  if (time > 0) {
    clock_t startticks;
    struct tms atms;

    if (!timeSet) {
      twait.tv_sec == time/1000;
      twait.tv_nsec = (time % 1000) * 1000000;
      startticks = times(&atms);
      timeSet = TRUE;
    } else {
      // For retries, calculate how much time is left
      // or we will wait too long if we get an error or a message
      int deltaticks = times(&atms) - startticks;
      twait.tv_sec = (time/1000) - (deltaticks / MHLCLK_TCK);
      twait.tv_nsec = ((time % 1000) * 1000000) -
         ((deltaticks % MHLCLK_TCK) * (1000000000 / MHLCLK_TCK));
      if (twait.tv_nsec < 0) {
        twait.tv_nsec += 1000000000;
        twait.tv_sec--;
      }
      if (twait.tv_sec < 0) {
        mutex_unlock(&qData->m_qLock);
        return(MHtimeOut);
      }
    }

    if (pSigFlg != NULL && *pSigFlg != 0) {
      mutex_unlock(&qData->m_qLock);
      return(MHintr);
    }
    TMisInBlockingReceive = TRUE;

    if ((ret = cond_timedwait(&qData->m_cv,
                                      &qData->m_qLock,
                                      &twait)) != 0) {
      TMisInBlockingReceive = FALSE;
      if (ret == ETIME) {
        mutex_unlock(&qData->m_qLock);
        return(MHtimeOut);
      } if (ret == EINTR) {
        mutex_unlock(&qData->m_qLock);
        return(MHintr);
      } else if (ret == EWOULDBLOCK) {
        // IGNORE this and treat as a retry (Linux only case)
      } else {
        mutex_unlock(&qData->m_qLock);
        //CRDEBUG(CRmsgh+14, ("Unexpected return from cond_reltimedwait,"
        //                    "errno %d", ret));
        return(MHother);
      }
    }
    TMisInBlockingReceive = FALSE;
    goto getMessage;
  } else if (time == 0) {
    mutex_unlock(&qData->m_qLock);
    return(MHnoMsg);
  } else {
    // Block forever for a message
    TMisInBlockingReceive = TRUE;
    if ((ret = cond_wait(&qData->m_cv, &qData->m_qLock)) != 0) {
      TMisInBlockingReceive = TRUE;
      if (ret == EINTR) {
        mutex_unlock(&qData->m_qLock);
        return(MHintr);
      } else {
        mutex_unlock(&qData->m_qLock);
        return(MHother);
      }
    }
    TMisInBlockingReceive = FALSE;
    goto getMessage;
  }
  return (GLsuccess);
}

GLretVal MHinfoExt::emptyQueue(MHqid mhqid) {
  if (isAttach == FALSE)
     return (MHnoAth); // attach MSGH first

  Short qid = MHQID2QID(mhqid);
  Short hostid =  MHQID2HOST(mhqid);

  if (hostid != rt->LocalHostIndex && hostid != rt->SecondaryHostIndex) {
    return (MHbadQid); // bad mhqid
  }

  MHqData *qData = &rt->localdata[qid];
  MHmsgHead* pMsgHead;
  MHmsgHead* ptmpMsgHead;

  mutex_lock(&qData->m_qLock);
  qData->m_qLockCnt++;
  if (qData->m_msgs >= 0) {
    pMsgHead = &rt->msg_head[qData->m_msgs];
  } else {
    pMsgHead = NULL;
  }

  int  nChunks;
  char *freeArray;
  int  freeIndex;
  int  *pnMeasUsed;
  while(pMsgHead != NULL && qData->nCount > 0) {
    // Free message buffers and unlink this message
    if (pMsgHead->m_len < 768) {
      nChunks = (pMsgHead->m_len >> 8) + 1;
      freeArray = m_free256;
      freeIndex = ((m_buffers + pMsgHead->m_msgBuf) - m_buffer256) >> 8;
      pnMeasUsed = &rt->m_nMeasUsed256;
    } else if (pMsgHead->m_len < 3072) {
      freeArray = m_free1024;
      nChunks = (pMsgHead->m_len >> 10) + 1;
      freeIndex = ((m_buffers + pMsgHead->m_msgBuf) - m_buffer1024) >> 10;
      pnMeasUsed = &rt->m_nMeasUsed1024;
    } else if (pMsgHead->m_len < 12288) {
      freeArray = m_free4096;
      nChunks = (pMsgHead->m_len >> 12) + 1;
      freeIndex = ((m_buffers + pMsgHead->m_msgBuf) - m_buffer4096) >> 12;
      pnMeasUsed = &rt->m_nMeasUsed4096;
    } else {
      nChunks = (pMsgHead->m_len >> 14) + 1;
      freeArray = m_free16384;
      freeIndex = ((m_buffers + pMsgHead->m_msgBuf) - m_buffer16384) >> 14;
      pnMeasUsed = &rt->m_nMeasUsed16384;
    }
    for (int count = 0; count < nChunks; count++, freeIndex++) {
      freeArray[freeIndex] = TRUE;
    }
    // Put the message header on the free list
    ptmpMsgHead = pMsgHead;
    if (pMsgHead->m_next != MHempty) {
      pMsgHead = &rt->msg_head[pMsgHead->m_next];
    } else {
      pMsgHead = NULL;
    }
    qData->nCount--;
    ptmpMsgHead->m_next = rt->m_freeMsgHead;
    rt->m_freeMsgHead = ptmpMsgHead - rt->msg_head;
    ptmpMsgHead->m_len = MHempty;
    *pnMeasUsed -= nChunks;
  }
  qData->m_msgstail = MHempty;
  qData->m_msgs = MHempty;
  qData->nCount = 0;
  qData->nBytes = 0;
  qData->nBytesRcvd = 0;
  qData->nCountRcvd = 0;
  mutex_unlock(&qData->m_qLock);
  mutex_unlock(&rt->m_msgLock);
  return(GLsuccess);
}

// broadcast a msg to all registered processes except the sending process
GLretVal MHinfoExt::broadcast(MHqid sQid, Char *msgp,
                              Short msgsz, Long time,
                              Bool bAllNodes) {
  return (broadcast(sQid, msgp, msgsz, MH_scopeSystemAll, time, bAllNodes));
}

GLretVal MHinfoExt::broadcast(MHqid sQid, Char *msgp,
                              Short msgsz, MHclusterScope scope,
                              Long time, Bool bAllNodes) {
  GLretVal failCount = 0; // Number of failed sends
  if (isAttach == FALSE) {
    return (MHnoAth);
  }
  if (msgsz < MHmsgBaseSz || msgsz > MHmsgSz)
     return (MHbadSz);

  ((MHmsgBase *)msgp)->msgSz = msgsz; // update msgSz field in msg header

  Short i;
  for (i = 0; i < MHmaxQid; i++) {
    MHqid mhqid = MHMAKEMHQID(rt->getLocalHostIndex(), i);
    if (rt->rcvBroadcast(i) && rt->localdata[i].pid != -1 && mhqid != sQid) {
      if ((send(mhqid, msgp, msgsz, time)) < 0) failCount--;
    }
  }
  if (bAllNodes == FALSE) {
    return (failCount);
  }

  union {
    Long align;
    char buffer[MHmsgLimit];
  };
  MHbcast *bmsg = (MHbcast*)buffer;
  Bool isUsed;
  Bool isActive;

  bmsg->ptype = MHintPtyp;
  bmsg->mtype = MHbcastTyp;
  bmsg->msgSz = sizeof(MHbcast) -1 + msgsz;
  bmsg->time = time;
  memcpy(bmsg->data, msgp, msgsz);

  for (i = 0;i < MHmaxHostReg; i++) {
    Status(i, isUsed, isActive);
    if (!isUsed || !isActive || i == rt->LocalHostIndex ||
        i == rt->SecondaryHostIndex) {
      continue;
    }
    if (bAllNodes != MHallClusters && rt->SecondaryHostIndex != MHnone &&
        ((strncmp(rt->hostlist[i].hostname.display(), "as", 2) == 0 &&
          prefEnv != MH_peerCluster) ||
         (strncmp(rt->hostlist[i].hostname.display(), "as", 2) != 0 &&
          prefEnv == MH_peerCluster))) {
      continue;
    }

    if (!rt->isHostInScope(i, scope)) {
      continue;
    }
    // Send to MHRPROC on each host
    if (send(MHMAKEMHQID(i, MHrprocQ), buffer, bmsg->msgSz, time) < 0) {
      failCount --;
    }
  }
  return (failCount);
}

// Set the unsability staus of ethernets on the local host
GLretVal MHinfoExt::setEnetState(Short nEnet, Bool bIsUsable) {
  MHenetState stateMsg;
  if (isAttach == FALSE)
     return (MHnoAth);

  stateMsg.ptype = MHintPtyp;
  stateMsg.mtype = MHenetStateTyp;
  stateMsg.nEnet = nEnet;
  stateMsg.bIsUsable = bIsUsable;

  if (send(msghMhqid, (char*)&stateMsg, sizeof(stateMsg), 0) < 0) {
    return (MHnoCom); // lost communication to MSGH process
  }

  return (GLsuccess);
}

GLretVal MHinfoExt::getRealHostname(const Char* hostname, Char *realname) {
  if (isAttach == FALSE)
     return (MHnoAth);
  if (rt->getRealHostname(hostname, realname) == MHempty) {
    return(MHnoMat);
  }
  return (GLsuccess);
}

GLretVal MHinfoExt::getLogicalHostname(const Char *realname, Char *hostname) {
  if (isAttach == FALSE)
     return (MHnoAth);
  if (rt->getLogicalHostname(realname, hostname) == MHempty) {
    return(MHnoMat);
  }
  return(GLsuccess);
}

// Get the local host index out of the rt structure.
Short MHinfoExt::getLocalHostIndex() {
  if (isAttach == FALSE)
     return (MHnoAth);
  if (rt->SecondaryHostIndex == MHnone) {
    return (rt->LocalHostIndex);
  } else {
    if (prefEnv == MH_peerCluster) {
      return (rt->SecondaryHostIndex);
    } else {
      return (rt->LocalHostIndex);
    }
  }
}

GLretVal MHinfoExt::Status(int hostid, Bool& isUsed, Bool& isActive) {
  if (isAttach == FALSE || hostid < 0 || hostid >= MHmaxHostReg) {
    return(MHnoAth);
  } else {
    isUsed = rt->hostlist[hostid].isused;
    isActive = rt->hostlist[hostid].isactive;
  }
  return(GLsuccess);
}

// This function will provide a pid and also check to see fi that
// id is alive, if the QID is invalid, this function fails automatically
// However, if the QID is valid, the PID is returned. The PID is checked
// to see if it is really valid. if so, success is returned. if not,
// a failure is returned.

GLretVal MHinfoExt::checkPid(MHqid msgh_qid, pid_t &argpid) const {
  argpid = 0;
  Short host = MHQID2HOST(msgh_qid);
  Short qid = MHQID2QID(msgh_qid);

  if (host != rt->getLocalHostIndex() && host != rt->SecondaryHostIndex) {
    return (MHbadQid);
  }

  if (argpid <= 0) {
    argpid = -1;
    return (MHprocDead);
  }

  // Check to see if process is dead or alive

  int retval;
  int saverr;

  retval = kill(argpid, 0);
  saverr = errno;
  if (retval == 0)
     return (GLsuccess);
  switch (saverr) {
    // process is alive, but does not allow signals
  case EPERM:
    return (GLsuccess);
    // process is not alive
  case ESRCH:
    break;
  default:
    break;
  }
  return (MHprocDead);
}
  
// The following method will determine whether a given
// MSGH name passed as an arg. to the method is a
// registered MSGH name.
GLretVal MHinfoExt::IsReg(const Char *name) {
  return ((rt->findName(name)) == MHnullQ ? GLfail : GLsuccess);
}

GLretVal MHinfoExt::regGlobalName(const Char *name, MHqid realQ,
                                  MHqid &globalQ, Bool bKeepOnLead,
                                  Bool clusterGlobal) {
  return(regQueue(name, realQ, globalQ, bKeepOnLead, clusterGlobal, TRUE));
}

GLretVal MHinfoExt::global2Real(MHqid globalQ, MHqid& realQ) {
  if (isAttach == FALSE) {
    return(MHnoAth);
  }
  Short host = MHQID2HOST(globalQ);
  Short qid = MHQID2QID(globalQ);

  // if host is a global queue, perform translation to real
  // queue before continuing
  if (host == MHgQHost) {
    int selectedQ = rt->gqdata[qid].m_selectedQ;
    if (selectedQ == MHempty) {
      return(MHnoQAssigned);
    }
    realQ = rt->gqdata[qid].m_realQ[selectedQ];
  } else {
    return(MHnotGlobalQ);
  }
  return(GLsuccess);
}

GLretVal MHinfoExt::getMyHostName(char *name) {
  if (isAttach == FALSE) {
    return (MHnoAth);
  }
  Short local_index = rt->getLocalHostIndex();
  if (rt->SecondaryHostIndex != MHnone && prefEnv == MH_peerCluster) {
    local_index = rt->SecondaryHostIndex;
  }
  strcpy(name, rt->hostlist[local_index].hostname.display());
  // If msghosts file not setup, the logical name always defaults to cc0
  if (name[0] == 0) {
    strcpy(name, "cc)");
  }
  return(GLsuccess);
}

// Returns hostid of the lead CC
Short MHinfoExt::getLeadCC() {
  if (isAttach == FALSE) {
    return(MHnoAth);
  }
  return(rt->m_leadcc);
}

// Returns true if on lead
Bool MHinfoExt::onLeadCC() {
  if (isAttach == FALSE) {
    return(FALSE);
  }
  if (rt->m_leadcc == rt->LocalHostIndex) {
    return (TRUE);
  } else {
    return(FALSE);
  }
}

// Returns mhqid for the current process
// Need to check if this function needs to change
GLretVal MHinfoExt::getMyQid(MHqid& mhqid) {
  if (isAttach == FALSE) {
    return (MHnoAth);
  }
  // If attached, pid is initialized, however it is possible not
  // to have a queue
  int i;
  for (i = 0; i < MHmaxQid; i++) {
    if (rt->localdata[i].pid == pid) {
      break;
    }
  }

  if (i < MHmaxQid) {
    mhqid = MHMAKEMHQID(getLocalHostIndex(), i);
    return (GLsuccess);
  } else {
    return(MHnoEnt);
  }
}

const char* MHinfoExt::myNodeState() {
  if (isAttach == FALSE || rt->m_leadcc == MHempty) {
    return("UNKOWN");
  } else if (rt->m_leadcc == rt->LocalHostIndex) {
    return("LEAD");
  } else {
    return("ACTIVE");
  }
}

GLretVal MHinfoExt::setDistState(const MHqid distQ,
                                 const MHqid realQ,
                                 const Bool enable) {
  if (isAttach == FALSE) {
    return(MHnoLead);
  }
  GLretVal retval;
  Short host = MHQID2HOST(distQ);
  Short qid = MHQID2QID(distQ);

  MHqid leadMhqid = MHMAKEMHQID(rt->m_leadcc, MHmsghQ);

  if (host == MHdQHost) {
    // Look for match
    int i;
    for (i = 0; i < MHmaxDistQs; i++) {
      if (rt->dqdata[qid].m_realQ[i] == realQ) {
        rt->dqdata[qid].m_enabled[i] = enable;
        // Send the message to lead
        MHdQSet setmsg;
        setmsg.mtype = MHdQSetTyp;
        setmsg.ptype = MHintPtyp;
        setmsg.sQid = MHMAKEMHQID(getLocalHostIndex(), MHmsghQ);
        memcpy(setmsg.realQs, rt->dqdata[qid].m_realQ,
               sizeof(setmsg.realQs));
        memcpy(setmsg.enabled, rt->dqdata[qid].m_enabled,
               sizeof(setmsg.enabled));
        setmsg.nextQ = 0;
        setmsg.updatedQ = i;
        setmsg.dqid = distQ;
        if ((retval = MHmsgh.send(leadMhqid,
                                  (Char *)&setmsg,
                                  sizeof(setmsg),
                                  0L)) != GLsuccess) {
          return(retval);
        }
        break;
      }
    }
    if (i == MHmaxDistQs) {
      return(MHnoQue);
    }
  } else {
    return(MHnotDistQ);
  }
  return(GLsuccess);
}

GLretVal MHinfoExt::getDistState(const MHqid distQ,
                                 const MHqid realQ,
                                 Bool& enable) {
  if (isAttach == FALSE) {
    return(MHnoAth);
  }

  Short host = MHQID2HOST(distQ);
  Short qid = MHQID2QID(distQ);

  if (host == MHdQHost) {
    // Look for match
    int i;
    for (i = 0; i < MHmaxDistQs; i++) {
      if (rt->dqdata[qid].m_realQ[i] == realQ) {
        enable = rt->dqdata[qid].m_enabled[i];
        break;
      }
    }
    if (i == MHmaxDistQs) {
      return(MHnoQue);
    }
  } else {
    return(MHnotDistQ);
  }

  return(GLsuccess);
}

GLretVal MHinfoExt::getNextQ(const MHqid queue, int& index,
                             MHqid& realQ, Bool enabledOnly) {
  if (isAttach == FALSE)
     return(MHnoAth);

  Short host = MHQID2HOST(queue);
  Short qid = MHQID2QID(queue);

  if (host == MHgQHost) {
    while (index < MHmaxRealQs) {
      index++;
      if (rt->gqdata[qid].m_realQ[index] != MHnullQ) {
        if (enabledOnly && rt->gqdata[qid].m_selectedQ != index) {
          continue;
        }
        realQ = rt->gqdata[qid].m_realQ[index];
        break;
      }
    }
    if (index >= MHmaxRealQs) {
      index = MHmaxRealQs;
      return (MHnoEnt);
    }
  } else if (host == MHdQHost) {
    while (index < MHmaxDistQs) {
      index++;
      if (rt->dqdata[qid].m_realQ[index] != MHnullQ) {
        if (enabledOnly && !rt->dqdata[qid].m_enabled) {
          continue;
        }
        realQ = rt->dqdata[qid].m_realQ[index];
        break;
      }
    }
    if (index >= MHmaxDistQs) {
      index = MHmaxDistQs;
      return (MHnoEnt);
    }
  } else {
    if (index >= MHmaxDistQs) {
      index = MHmaxDistQs;
      return (MHnoEnt);
    }
    // No looping here
    index = MHmaxDistQs;
    realQ = queue;
  }
  return(GLsuccess);
}

int MHinfoExt::sendToAllQueues(const MHqid queue, char* msgp,
                               Long msgsz, Bool enabledOnly,
                               Long time, Bool buffered) {
  int index = -1;
  MHqid realQ;
  int count = 0;

  while (getNextQ(queue, index, realQ, enabledOnly) == GLsuccess) {
    if (send(realQ, msgp, msgsz, time, buffered) == GLsuccess) {
      count++;
    }
  }
  return (count);
}


GLretVal MHinfoExt::regQueue(const Char *name, MHqid realQ,
                             MHqid& virtualQ, Bool bKeepOnLead,
                             Bool ClusterGlobal, Bool isGlobal) {
  MHregName regName; // reg name message
  MHregAck regAck; // reg name ACK message
  MHqid leadMhqid;
  GLretVal retval;

#ifndef PROC_UNDER_INIT
  return(MHother);
#endif
  if (isAttach == FALSE) {
    return (MHnoAth);
  }
  if (ClusterGlobal == MH_systemGlobal) {
    if (rt->oam_lead[0].display()[0] == 0) {
      ClusterGlobal = MH_clusterLocal;
    } else if (rt->m_oamLead == MHempty) {
      return(MHnoLead);
    } else {
      leadMhqid = MHMAKEMHQID(rt->m_oamLead, MHmsghQ);
    }
  }
  if (ClusterGlobal == MH_clusterGlobal) {
    if (rt->m_clusterLead == MHempty) {
      return(MHnoLead);
    }
    leadMhqid = MHMAKEMHQID(rt->m_clusterLead, MHmsghQ);
    // Only peerCluster centric Qids are acceptable
    if (rt->SecondaryHostIndex != MHnone &&
        strncmp(rt->hostlist[MHQID2HOST(realQ)].hostname.display(),
                "as", 2) != 0) {
      return(MHbadQid);
    }
    leadMhqid = MHMAKEMHQID(rt->m_leadcc,MHmsghQ);
  }
  if (MHGETQ(realQ) < 0) {
    return(MHbadQid);
  }

  // Prepare and send a "register name" message to MSGH process
  regName.mhname = name;
  regName.ptype = MHintPtyp;
  regName.mtype = MHregNameTyp;
  regName.pid = pid;
  regName.rcvBroadcast = FALSE;
  regName.q_size = -1;
  regName.mhqid = MHnullQ;
  regName.realQ = realQ;
  regName.bKeepOnLead = bKeepOnLead;
  regName.gQ = isGlobal;
  regName.dQ = !isGlobal;
  regName.clusterGlobal = ClusterGlobal;
  regName.fill = MHR26;

  if ((retval = MHmsgh.send(leadMhqid, (Char *)&regName,
                            sizeof(MHregName), 0L)) != GLsuccess) {
    return(retval);
  }

  Long msgSz = sizeof(regAck);
  int count = 0;

  MHregAgain:
  if ((retval = MHmsgh.receive(realQ, (Char*)&regAck, msgSz,
                               MHregPtyp, MHregTimeOut)) != GLsuccess) {
    count++;
    if (retval == MHintr && count < 4) {
      goto MHregAgain;
    }
    return(retval);
  }

  if (regAck.mtype != MHregAckTyp) {
    return(MHnoCom);
  }

  if (regAck.reject == TRUE) {
    return(MHnoSpc);
  }

  // Need to do this for backward compatiblitity
  // 127 was old virtual host id for global queues
  if (MHQID2HOST(regAck.mhqid) != 127) {
    virtualQ = regAck.mhqid;
  } else {
    virtualQ = MHMAKEMHQID(MHgQHost, MHQID2QID(regAck.mhqid));
  }
  return(GLsuccess);
}

#define MHqidDisp 6
#define MHqidNameLength 32
const char* MHqid::display() const {
  static char buffer[MHqidDisp][MHqidNameLength];
  static unsigned int index = 0;

  if (id == MHempty) {
    return("MHnullQ");
  }
  index++;
  if (index >= MHqidDisp) {
    index = 0;
  }
  sprintf(buffer[index], "%03d/%04d", MHQID2HOST(id), MHQID2QID(id));
  return(buffer[index]);
}

// This function returns the up staus of the enet networks
// given the host logical name, i.e, ts0, ts1, etc.
GLretVal MHinfoExt::getNetState(Short &hostid, Bool& net0, Bool& net1) {
  if (isAttach == FALSE) {
    return (MHnoAth);
  }
  if (hostid < -1) {
    return(MHinvHostId);
  }

  while((++hostid) < MHmaxHostReg && rt->hostlist[hostid].isused == FALSE);

  if (hostid >= MHmaxHostReg) {
    return(MHnoEnt);
  }

  net0 = rt->hostlist[hostid].netup[0];
  net1 = rt->hostlist[hostid].netup[1];
  return (GLsuccess);
}

GLretVal MHinfoExt::getEnvType(MHenvType& env) {
  if (isAttach == FALSE) {
    return (MHnoAth);
  }

  env = rt->m_envType;

  if (rt->SecondaryHostIndex != MHnone && prefEnv == MH_peerCluster) {
    env = prefEnv;
  }

  return(GLsuccess);
}

GLretVal MHinfoExt::getIpAddress(const char* lname, unsigned int inum,
                                 sockaddr_in6& addr) {
  if (isAttach == FALSE) {
    return (MHnoAth);
  }

  if (inum >= MHmaxNets) {
    return(MHbadSz);
  }

  Short hostid;
  if ((hostid = rt->findHost(lname)) == MHempty) {
    return(MHnoHost);
  }

  addr = rt->hostlist[hostid].saddr[inum];
  return(GLsuccess);
}

Bool MHinfoExt::isCC(const char *name) {
  return(rt->isCC(name));
}

GLretVal MHinfoExt::getQidStats(MHqid qid, MHqStats& qdata) {
  if (isAttach == FALSE) {
    return (MHnoAth);
  }

  if (MHGETQ(qid) < 0 || MHQID2HOST(qid) == MHdQHost ||
      MHQID2HOST(qid) == MHgQHost ||
      (MHQID2HOST(qid) != rt->LocalHostIndex &&
       MHQID2HOST(qid) != rt->SecondaryHostIndex)) {
    return (MHbadQid);
  }

  Short qindex = MHQID2QID(qid);
  if (qindex >= MHmaxQid) {
    return(MHbadQid);
  }

  qdata.nBytes = rt->localdata[qindex].nBytes;
  qdata.nBytesHigh = rt->localdata[qindex].nBytesHigh;
  qdata.nBytesRcvd = rt->localdata[qindex].nBytesRcvd;
  qdata.nCountRcvd = rt->localdata[qindex].nCountRcvd;
  qdata.nCount = rt->localdata[qindex].nCountHigh;
  qdata.nCountHigh = rt->localdata[qindex].nCountHigh;
  qdata.nCountLimit = rt->localdata[qindex].nCountLimit;
  return(GLsuccess);
}
  
GLretVal MHinfoExt::resetQidStats(MHqid qid) {
  if (isAttach == FALSE) {
    return (MHnoAth);
  }
  if (MHGETQ(qid) < 0 || MHQID2HOST(qid) == MHdQHost ||
      MHQID2HOST(qid) == MHgQHost ||
      (MHQID2HOST(qid) != rt->LocalHostIndex &&
       MHQID2HOST(qid) != rt->SecondaryHostIndex)) {
    return(MHbadQid);
  }

  Short qindex = MHQID2QID(qid);
  if (qindex >= MHmaxQid) {
    return(MHbadQid);
  }

  rt->localdata[qindex].nCountHigh = 0;
  rt->localdata[qindex].nBytesHigh = 0;
  rt->localdata[qindex].nBytesRcvd = 0;
  rt->localdata[qindex].nCountRcvd = 0;
  return (GLsuccess);
}

int MHinfoExt::getPercentLoadOnActive() {
  if (isAttach == FALSE) {
    return (50);
  } else {
    return(rt->percentLoadOnActive);
  }
}

// This function is called  to determine the perspective that the
// the other interfaces should take when returning values like
// MHqid in mixed clusters. Since some calls can be ambiguos depending
// on the perspective of the caller, this method will ened to be called
// to disambiguate the intention of the caller.

GLretVal MHinfoExt::setPreferredEnv(MHenvType env) {
  if (isAttach == FALSE)
     return(MHnoAth);

  if (env != MH_standard && env != MH_peerCluster)
     return(MHbadEnv);

  prefEnv = rt->m_envType;
  if (rt->SecondaryHostIndex == MHnone) {
    // This is not meixed mode cluster, only return suscess if the
    // requested mode matches the current mode  sicne alternative mode
    // does not make sense.
    if (env != rt->m_envType) {
      return(MHenvMismatch);
    }
  } else {
    // Do not allow setting of MH_peerCluster on active side of AA pair
    if (!onLeadCC() && env == MH_peerCluster)
       return(MHbadEnv);
    prefEnv = env;
  }
  return(GLsuccess);
}


GLretVal MHinfoExt::convQue(MHqid& qid, Short hostid) {
  if (isAttach == FALSE)
     return(MHnoAth);

  if (rt->SecondaryHostIndex == MHnone ||
      MHQID2HOST(qid) != rt->LocalHostIndex) {
    return(GLsuccess);
  }
  qid = MHMAKEMHQID(rt->getActualLocalHostIndex(hostid), MHQID2QID(qid));
  return(GLsuccess);
}

GLretVal MHinfoExt::setQueLimits(MHqid qid, int nMessages, int nBytes) {
  if (isAttach == FALSE)
     return (MHnoAth);

  if (MHGETQ(qid) < 0 || MHQID2HOST(qid) == MHdQHost ||
      MHQID2HOST(qid) == MHgQHost ||
      (MHQID2HOST(qid) != rt->LocalHostIndex &&
       MHQID2HOST(qid) != rt->SecondaryHostIndex)) {
    return(MHbadQid);
  }

  // Set the limits, no checking will be performed. In fact, can set
  // limits for someone else

  Short qindex = MHQID2QID(qid);
  if (qindex >= MHmaxQid) {
    return(MHbadQid);
  }

  rt->localdata[qindex].nCountLimit = nMessages;
  rt->localdata[qindex].nByteLimit = nBytes;
  return (GLsuccess);
}

Short MHinfoExt::getOAMLead() {
  if (isAttach == FALSE)
     return (MHnoAth);

  return (rt->m_oamLead);
}

Short MHinfoExt::getActiveVhost() {
  if (isAttach == FALSE)
     return (MHnoAth);

  return(rt->m_vhostActive);
}

Bool MHinfoExt::isOAMLead() {
  if (isAttach == FALSE)
     return(FALSE);

  return(rt->LocalHostIndex == rt->m_oamLead ||
         (rt->m_oamLead != MHempty &&
          rt->SecondaryHostIndex == rt->m_oamLead));
}

Bool MHinfoExt::isVhostActive() {
  if (isAttach == FALSE)
     return(FALSE);

  return (rt->LocalHostIndex == rt->m_vhostActive ||
          (rt->m_vhostActive != MHempty &&
           rt->SecondaryHostIndex == rt->m_vhostActive));
}

GLretVal MHinfoExt::systemToLogical(const Char *system, Char *logical) {
  if (isAttach == FALSE)
     return(MHnoAth);

  short hostid;
  if ((hostid = rt->findHost(system)) >= 0) {
    strcpy(logical, rt->hostlist[hostid].hostname.display());
    return(GLsuccess);
  }

  logical[0] = 0;
  return(MHnoHost);
}

GLretVal MHinfoExt::flush(MHqid mhqid) {
  if (isAttach == FALSE)
     return(MHnoAth);

  Short hostid;
  GLretVal ret;

  if ((hostid = MHQID2HOST(mhqid)) < MHmaxHostReg &&
      rt->hostlist[hostid].isactive) {
    if (hostid == rt->LocalHostIndex || hostid == rt->SecondaryHostIndex) {
      return (GLsuccess);
    }
    ret = rt->Send(hostid, NULL, 0, FALSE, TRUE, TRUE);
    if (ret != GLsuccess && ret > 0) {
      return(MHmapError(ret, __LINE__, mhqid));
    }
  }else {
    return(MHinvHostId);
  }
  return(ret);
}
