#ifndef __MHRT_H
#define __MHRT_H

// DESCRIPTION:
//  This file defines a class that manages the routing
//  table in the shared memory

#undef  _REENTRANT
#define _REENTRANT
#include <pthread.h>
//#include <synch.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/utsname.h>
#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "hdr/GLmsgs.h"
#include "hdr/GLportid.h"
#include "cc/hdr/msgh/MHname.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/msgh/MHmsg.hh"
#include "cc/hdr/msgh/MHgd.hh"
#include "cc/hdr/init/INpDeath.hh"
#include "cc/hdr/msgh/MHlrgMsgExt.hh"

#define MH_SHM_VER (0x0a0a0000 + 1)

#define MHR26 0xc001

#define MHmaxMsgs 0x20000 // Total number of message headers in the system

const key_t MHkey = MHMSGBASE; // key used to get a shmid

// Constants & masks for decoding the mhqid
const Short MHhostShift = 11;
const Short MHhostMask = 0x3ff;
const Short MHqidMask = 0x3ff;
// Timer tag used to encode the timer for the host in reject state
#define MHrejTag 0x1000

extern MHqid MHmakeqid(int host, int qid);
// Because the number of nodes is increased. need now pack the hostid into
// two field of MHmsgbase. Those two functions convert back and forth.
extern void MHsetHostId(void* pmsg, Short hostId);
extern Short MHgetHostId(void* pmsg);

#define MHMAKEMHQID(host, qid) MHmakeqid(host, qid)
#define MHQID2HOST(mhqid) (((*(int *)(&(mhqid))) >> MHhostShift) & MHhostMask)
#define MHQID2QID(mhqid) ((*(int *)(&(mhqid))) & MHqidMask)
#define MHHTON(mhqid) htonl((*(int *)(&(mhqid))))
#define MHNTOH(mhqid) ntohl((*(int *)(&(mhqid))))
#define MHGETQ(mhqid) (*(int*)(&(mhqid)))

#define MHresetMask 0x8000
// Constants for the pool of buffers to be used to allocate and
// hash the names
const int MHtotNameSlotsSz = 32000;

// The number of names that can be registered is smaller than
// tot number of name slots to that the free list takes some
// time to go around, preserving some information for shared
// memory update race conditions
const Short MHmaxNamesReg = MHtotNameSlotsSz - 500;

// Number of buffers for sending
const Short MHmaxSQueue = 1024;

// Number of buffers for sending
const Short MHdefwindowSz = 1024;

// Default widnow size
const Short MHdefWindowsSz = 128;

// Number of global data objects
#define MHmaxGd IN_GD_MAX

#define MHmaxPilotNodes 2
#define MHmaxRack 4
#define MHmaxChassis 20
#define MHmaxSlot 16

// Memory allocator tunnings, large data chunk makes allocator more
// efficient at some cost of memory waste. Since cargo is implemented.
// most message will be larger, so it is better no to waste time
// finding space
const Short MHdataChunk = 4096; // Must be power of 2
#define MHCHUNKADDR(chunk) (chunk << 10) // This must correspond to
                                         // MHdataChunk ata is size
                                         // of shift is 2 less
#define MHGETNUMCHUNKS(size) ((size >> 12) + 1)

// maximum sequence number
const Short MHmaxSeq = 1023;

// Hash table size for the list all processes in all hosts
const Short MHtotHashTblSz = 32497; // Prime number about 2.14*MHmaxAllHosts

// Hash table size for the table of host names.
const Short MHhostHashTblSz = 2477; // Prime number about 2.14 * MHmaxAllHosts

// Hash table size for the table of names per host
const Short MHnameHashTblSz = 1193; // prime number about 1.14*MHmaxQid

// Constants used in 'msqid' field of routing table
const Short MHempty = -1;
const Short MHnone = -2;

// Unbuffered send flag
#define MHunBufferedFlg 0x80
#define MHmaxAlias 4
#define MHmaxNetDelay 16
#define MHmaxRouteSync 6
#define MH_allNodes 2

// Forward declaration
class MHinfoInt;

struct MHqitem {
  Long m_length;
  short m_dataidx;
};

struct MHhostdata {
  MHname hostname; // Logical name of this host
  Char realname[SYS_NMLN]; // Real machine name
  Char nameRCS[MHmaxRCSName+2]; // RCS name
  Bool isused; // Slot has host data inited
  Bool isactive; // Boolean for we think it's accessible
  Short spare2;
  Short SelectedNet; // Active network for communication to this host
  Short Preferrednet; // Preferred network for communication to this host
  Bool netup[MHmaxNets]; // True if that network is upShort
  sockaddr_in6 saddr[MHmaxNets]; // Socket addresses for the host
  sockaddr_in6 alias[MHmaxNets][MHmaxAlias]; // Alias socket address for host
  Short next; // Pointer for next element in host hash
  Short delay; // Delay in trying to connect hosts
  Short windowSz; // Protocol window size for this host
  Short nDelMsgs;
  U_long nMeasSeqReset; //Number of sequencing resets
  Bool isR26;
  char nRouteSync; // Number of route sync attempts
  Short fillshort;
  Short auditcount[MHmaxNets]; // Audit count for talking to other host
  Long fill[30]; // future growth
  int netDelay[MHmaxNets][MHmaxNetDelay];
  int netDelTime[MHmaxNets];
  Short hashlist[MHnameHashTblSz]; // Hash table for names for this host
  Short indexlist[MHmaxQid]; // Indx table for this host
  U_short nRcv; // receive sequence number
  U_short nSend; // send sequence number
  Short nRejects; // number of rejects send
  U_short nLastAcked; // Last acked message
  u_char nUnAcked; // Number of unacked messages
  u_long nMeasResends; // Number of messages resend to this host
  U_long nMeasRejectsRcv; // Number of reject messages received
  U_long nMeasRejectsSend; // Number of reject messages send
  U_long nMeasMsgSend; // Total number of messages send
  U_long nMeasSndMsgDropped; // Number of messages tossed from send buffer
  U_long nMeasRcvMsgDropped; // Numer of messages tossed from rcv buffer
  U_long nMeasNodeDown; // Number of times node became inaccessible
  U_long nMeasSendBufFull; // Number of times send rejected because
                           // of no send bufferes
  U_long nMeasLinkAlter; // Number of times ethernet links were alternated
  U_long nMeasFailedSend; // Number of times msgsnd failed
  U_long nMeasMsgOutOfWindow; // Messages delivered out of current window
  U_long nMeasSockDropped; // Messages dropped on socket send
  MHqitem sendQ[MHmaxSQueue]; // Circular queue of messages to be send
  MHqitem rcvQ[MHmaxSQueue]; // Circular queue of messages to be receive
  Short nInRcvQ; // number of message buffered in rcvQ
  Short ping; // Frequency in base ping cycles
              // in which to ping the this node
  Short pingTime[MHmaxNets]; // time since last ping in ping cycles
  Short maxMissed; // Maximum numer of pings allowed to be missed
};

struct MHqData {
  pthread_mutex_t m_qLock; // mutex variable associated with cv
  pthread_cond_t m_cv;
  pid_t pid; // local process ids
  Long nBytes; // Number of  bytes in queue
  Long nBytesHigh; // high water mark for number of bytes
  U_long nBytesRcvd; // Number of bytes received
  U_long nCountRcvd; // Number of messages received
  int nByteLimit; // Limit of the total number of bytes
  U_short m_qLockCnt; // mutext audit count
  U_short m_qLockCntLast; // mutext last audit count
  pid_t m_qLockPid; // Pid of last locker of the queue
  int m_msgs; // queued messages
  int m_msgstail; // end of queued messages for easy insertion
  Short nCount; // Count of messages in queue
  Short nCountHigh; // High water mark
  Short nCountLimit; // Msg Count limit
  Char auditcount; // number of times the audit has run
  Bool rcvBroadcast; // True if this queue receives broadcast mesages
  Bool inUse; // True if this queue is used
  Bool fillchar;
  Short fillshort; // future use
};

struct MHmsgHead {
  int m_next; // Index of the next message
  int m_msgBuf; // The location of the message
  int m_len; // length of the message
  short m_inUseCnt;
  short m_fill;
};

struct MHgdCtl {
  int m_RprocIndex;
  int m_shmid;
};

// The shared memory. controlled by the MSGH process, contains a
// set of routing tables. one for each host configured in /sn/msgh/msghosts.
// This table is used for (name -> mhqid or msqid)
// and (mhqid -> msqid) translations. Given an entry containing
// a registered name. its index is the mhqid associated with the name.
// in the low 9 bits of the short, while a host number is put in the
// upper 7 bits. Thus the mhqid can represent up to 128 machines.
// each with up to 512 names registered, using up the entrire addressing
// spare of the short, we probably should increase the size to long,
// but that would entail updating MANY lines of code in other subsystems.
class MHrt {
  friend class MHinfoInt;
  friend class MHinfoExt;
  friend class MHgd;
  friend class MHmeas;
  friend GLretVal MHrmQueues();
  friend void MHconncheck();
  friend GLretVal MHcheckHostIP(int);
  friend Long MHprocessmsg(MHqid&);

public:
  MHrt();

  Void rtInit(Bool shmNoExist);
  GLretVal readhostFile(Bool resetalldata);
  GLretVal readHostFileCC(Bool resetalldata);
  MHqid findName(const Char *name) const;
  int findName(Short& hostid, const Char *name, MHclusterScope scope) const;
  GLretVal getProcName(Char *name, MHqid mhqid) const;
  Short findMhqid(MHqid mhqid, Char *name) const;
  MHqid insertName(const Char *, MHqid, pid_t, Bool global,
                   Bool fullreset, Short Selectednet=0,
                   Bool RcvBroadcast=TRUE, Long q_size=-1,
                   Bool gQ=FALSE, MHqid realQ=MHnullQ,
                   Bool bKeepOnLead=FALSE, Bool dQ=FALSE,
                   short msg_limit=-1, Bool clusterGlobal=FALSE);
  Short printTbl(Bool fullflag, char* name, Long offset, Long length,
                 Bool details, Bool remove, Bool zero);
  Void gdDump(Bool fullflag, char *nam, Long offset, Long length,
              Bool details, Bool remove, Bool zero);
  Void deleteName(const MHname&, const MHqid, const pid_t);
  int match(const MHname&, const MHqid, const pid_t);
  int getMsqid(const MHqid) const;
  pid_t getPid(const MHqid) const;
  Short getLocalHostIndex() const;
  Short getActualLocalHostIndex(Short hostid) const;
  Bool isHostActive(Short hostnum) const;
  Bool isHostUsed(Short hostnum) const;
  const sockaddr_in6 *getHostAddr(Short hostnum) const;
  Bool conform(Short host, MHname names[], Short startqid, Short count,
               Bool fullreset); // make sure host conforms to qid info
  // make sure host conforms to qid info
  void gQconform(MHgQData data[], Short startqid, Short count,
                 Bool fullreset, MHqid leadQ);
  // make sure host conforms to qid info
  void dQconform(MHdQData data[], Short startqid, Short count,
                 Bool fullreset, MHqid leadQ);
  Void setGlobalQ(int gqidx, Bool bSetMsg=FALSE);
  Void setDistQ(int gqidx);
  Void enetState(Short nEnet, Bool bIsUsable);
  Void conn(Bool bNoMessage[][MHmaxNets], short hostid);
  Void hostDel(MHqid sQid, Bool bReply, Short enet, Short onEnet,
               short clusterlead);
  Void gqInitAck(MHqid gqid, GLretVal ret);
  Void inpDeath(INpDeath *msg);
  Void gQSet(MHgQSet *msg);
  Void dQSet(MHdQSet *msg);
  Short getRealHostname(const Char *hostname, Char *realName);
  Short getLogicalHostname(const Char *realName, Char *hostname);
  Bool isHostInScope(Short hostid, MHclusterScope scope) const;
  Bool rcvBroadcast(Short localqid);
  GLretVal Send(Short host, Char *msgp, Long msgsz, Bool resetSeq,
                Bool buffered=TRUE, Bool doLock=TRUE);
  GLretVal ReSend(Short host, int sndIndex, U_short count=0);
  Void CheckReject();
  Void CheckReject(int);
  Void ClearSndQueue(Short hostid, Bool setNutex=TRUE);
  Void ClearRcvQueue(Short hostid, Bool setMutex=TRUE);
  Void SetSockId(int sock_id);
  GLretVal ValidateMsg(MHmsgBase *msgp);
  Void AuditBuffers();
  Bool AuditMutex();
  Bool isCC(const char *name);
  Void sendCargo();
  MHqid allocateMHqid(const char *name);
  inline int getCargoTimer() { return(m_CargoTimer); }
  Void updateClusterLead(Short hostid, MHmsgBase *msp = NULL);
  static Bool *datafree;
  static Long* data;
  static int numHosts;
private:
  Short findNameOnGlobal(const Char *name) const;
  Short findNameOnHost(Short hostid, const Char *name) const;
  Short findHost(const Char *name) const;
  Void sendName(const Char *name, MHqid mhqid, MHqid realQ,
                Bool bKeepOnLead, Bool glob);
  Void checkNetAudit(const Char *name, MHqid mhqid, short bind,
                     Bool fullreset);
  
  // If a 'msqid' field is MHempty or MHdeleted. the
  // corresponding entry is not in use.
  int m_Version; // Shared memory version
  struct {
    MHname mhname; // name of the process
    MHqid  mhqid;  // host _ index mhqid
    Bool global; // TRUE if queue available in all nodes
    Char fill1; // Not used
    Short nextfree; // link to next free element when free
    Short hostnext; // link for host linked list
    Short globalnext; // link for global linked list
    Long fill; // future growth
  } rt[MHtotNameSlotsSz];

  MHhostdata hostlist[MHmaxAllHosts];
  MHqData localdata[MHmaxQid];
  MHmsgHead msg_head[MHmaxMsgs];

  struct {
    MHqid m_realQ[MHmaxRealQs]; // Rreal queues for this globalQ
    Long m_timestamp;
    Long m_tmridx;
    Char m_selectedQ;
    Bool m_bKeepOnLead;
    Short m_fill;
  } gqdata[MHmaxQid];

  struct {
    MHqid m_realQ[MHmaxDistQs]; // Real queues for this distributive Q
    Bool m_enabled[MHmaxDistQs]; // Real queues for this distributive Q
    Short m_maxMember;
    Short m_nextQ; // Q that should get the next send
    Long m_timestamp;
  } dqdata[MHmaxQid];

  Short RCStoHostId[MHmaxRack][MHmaxChassis][MHmaxSlot];
  MHname oam_lead[MHmaxPilotNodes];

  pthread_mutex_t m_lock; // mutex variable for ciritcal region processing

  U_long m_lockcnt; // this variable is incremented every time lock is set
  pthread_mutex_t m_msgLock;
  U_long m_msgLockCnt;
  int m_lastActive;
  int m_bufferFreedCnt; // Number of mesasge buffers freed by audit
  int m_headerFreedCnt; // Number of message headers freed by audit
  int m_freeMsgHead; // Head of free messages headers
  int m_nMeasUsed256; // Current count of 256 message buffers
  int m_nMeasUsed1024; // Current count of 1024 message buffers
  int m_nMeasUsed4096; // Current count of 4096 message buffers
  int m_nMeasUsed16384; // Current count of 16384 mesage buffers
  int m_nMeasUsedOut; // current count of outoing buffers
  int m_nMeasHigh256; // High count of 256 message buffers
  int m_nMeasHigh1024; // High count of 1024 message buffers
  int m_nMeasHigh4096; // High count of 4096 message buffers
  int m_nMeasHigh16384; // High count of 16384 message buffers
  int m_nMeasHighOut;  // High count of outgoing buffers
  int m_nSearch256; // Starting search index 256 message buffers
  int m_nSearch1024; // Starting search index 1024 message buffers
  int m_nSearch4096; // Starting search index 4096 message buffers
  int m_nSearch16384; // Starting search index 16384 message buffers
  unsigned int m_BufIndex; // Indx of last allocated chunk
  U_long nbuffFreed; // Number of wrongly busy sendbuffers
  MHenvType m_envType; // Current environment type

  Short host_hash[MHhostHashTblSz];  // hashing of host names
  Short global_hash[MHtotHashTblSz]; // global hash for all names together
  Short m_leadcc; // hostid of the lead CC
  Bool sparebool;
  int free_head; // Head of the free list
  int free_tail; // Tail of the free list of rt elems

  Short LocalHostIndex; // Index into hostname for the lcoal host.
  Short m_clusterLead; // Lead CC in a peer cluser

  Long TotalNamesReg; // Total number of names registered.
  U_long m_nMeasTotSendBufFull; // Number of times could not send
                                // because of total buffer exhaustion
  U_long m_nMeasFailedSend; // Number of times msgsnd failed
  int m_nMaxCargo; // Max number of messages to cargo at a time
  int m_MinCargoSz; // min number of bytes before cargo sent
  int m_CargoTimer; // Frequency of cargo message polling in ms
public:
  Bool m_buffered; // Controls if buffered scheme is used
  int m_n256; // Number of 256 byte buffers
  int m_n1024; // Number of 1024 byte buffers
  int m_n4096; // Number of 4096 byte buffers
  int m_n16384; // Number of 16384 byte buffers
  int m_nextQid; // Next qid to start searching from
  int m_NumChunks; // Number of outping buffer chunks
  pthread_mutex_t m_dqLock; //Distributive queue mutex
  U_long m_dqcnt; //Distributive queue mutex audit
  Long m_spare[8183]; // spare for future/su use
  Short fillshort;
  Short m_vhostActive; // Index of the active vhost
  Short m_oamLead; // Index of the oam lead
  Short SecondaryHostIndex; // second host index used in mixed clusters
                            //always peer host
  int percentLoadOnActive; // the percent of load to be distributed to active
  MHgdCtl m_gdCtl[MHmaxGd]; // Global data objects control information
};

extern Bool MHnoErr;

#define MHERROR(data)                   \
        if(MHnoErr) {                   \
                MHnoErr = FALSE;        \
                CRERROR data;           \
                MHnoErr = TRUE;         \
        }

#define MHDEBUG(data)                   \
        if(MHnoErr) {                   \
                MHnoErr = FALSE;        \
                CRDEBUG data;           \
                MHnoErr = TRUE;         \
        }

#define MHDEBUG_PRINT(data)             \
        if(MHnoErr) {                   \
                MHnoErr = FALSE;        \
                CRDEBUG_PRINT data;     \
                MHnoErr = TRUE;         \
        }

#endif


   


