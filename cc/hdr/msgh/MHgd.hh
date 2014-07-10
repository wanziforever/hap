#ifndef _MHGD_H
#define _MHGD_H

// This header file defines all the data structures for
// the global data object

#include <string.h>
#include <pthread.h>
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHname.hh"
#include "cc/hdr/msgh/MHmtypes.hh"
#include "cc/hdr/init/INsharedMem.hh"
#include "cc/hdr/msgh/MHdefs.hh"

#define MHgdMaxOps  4096  // Maximum number of gdo operations outstanding
                          // must be power of 2
#define MHgdMaxSend 1024 // Maximum number of outstanding delayed sends
#define MHgdMaxBuf 256

#define MHgdUpdHead 64 // Sizeof update message header
#define MHdefUpdSz 32688
#define MHgdMaxSz 0x7FFFFFFFFLL // Maximum size of a singe GDO

enum MHgdDist {
  MHGD_CCONLY,  // distribute global data on CCs only
  MHGD_ALL,  // distribbute global data on all nodes
  MHGD_NONE  // Do not distribute this GDO at all
};

enum MHgdOpType {
  MHGD_EMPTY,  // Empty operation record, should always be 0
  MHGD_INVALIDATE, // Invalidate operation
  MHGD_MEMOVE, // Memmove operation
  MHGD_INVCMP, // Invalidate compare
  MHGD_INVOP   // Invalid operation
};

struct MHgdInvalidate {
  LongLong m_start; // Start of invalidated region
  Long m_length; // Length of the invalidated region
};

struct MHgdInvCmp {
  LongLong m_start; // Start of invlidated region
  Long m_length; // Length of the invalidated region
  int m_qid;  // Qid to be notified, has to be int because of union reset
  int m_offset; // Offset into the invalidate region to be compared
  int m_fill;
};

struct MHgdMemMove {
  LongLong m_to; // Move to location
  LongLong m_from; // Move from location
  Long m_length; //Length of the data to copy
};

struct MHgdInvalidateExt {
  void* m_start; // Start of invalidated region
  Long m_length; // Length of the invalidated region
};

struct MHgdInvCmpExt {
  void* m_start; // Start of invalidated regiion
  Long m_length; //Length of the invalidated region
  int m_qid; //Qid to be notified, int is used because of union restrictions
  int m_offset; // Offset into the invalidate region to be compared
};

struct MHgdMemMoveExt {
  void* m_to; // Move to location
  void* m_from;  // Move from location
  Long m_length; // Length of the data to copy
};

struct MHgdOp {
  MHgdOpType m_type;
  union {
    struct MHgdInvalidate m_invalidate;
    struct MHgdInvCmp m_invcmp;
    struct MHgdMemMove m_memmove;
#ifdef NOTIMP
    struct MHgdMemMoveExt m_memmoveExt;
    struct MHgdInvalidateExt m_invalidateExt;
    struct MHgdInvCmpExt m_invcmpExt;
#endif
  };
};

struct MHgdHost {
  int m_lastAcked; // Last update acked
  int m_nextMsg; // Next update to be sent
  int m_noAcks;  // Number of intervals without an Ack
  U_short m_nOutOfSeq; // Number of messages out of sequence
  Bool m_bIsDistributed; // TRUE if updates distributed to this host
  Bool m_fill; // TRUE if updates distributed to this host
};

struct MHgdMsg {
  Long m_size;
  int m_opNum;
  MHqid m_qid;
  Long m_msgp;
  int m_msgindx;
  int m_fill; // to insure 32/64 bit alignment
};

struct MHgdShm {
  LongLong m_LastSync;  // Sync address as checked last time
  LongLong m_SyncAddress; // Address object synched up to
  LongLong m_size;  // Size of the data space
  pthread_mutex_t m_lock; // mutex for this GDO
  int m_lockcnt;
  int m_lastlockcnt;
  char m_Name[MHmaxNameLen+1]; // name of the GDO
  Bool m_doSync; // True if invalidates should not be synched
  Bool m_doDelaySend; // True if delayed send is used
  char m_fill1;
  MHqid m_srcQue; // Source queue for synching
  uid_t m_Uid;  // Following data items are needed since
  int m_Permissions; // MSGH process will create replicated copies
  MHgdDist m_Dist; // Distribution type
  int m_GdId; // Id of this GDO
  int m_nLastSent; // Last op buffered
  int m_nNextOp; // Next operation to be inserted
  int m_Nextmsg; // Next dalayed message to be inserted
  int m_NextSent; // Next delayed message to be sent
  int m_StartToUpdate; // Start index to update
  MHgdHost m_Hosts[MHmaxHostReg]; // Per host info
  Short m_updHosts[MHmaxHostReg]; // list of hosts to distribute
  int m_curBuf; // Buffer currently being filled
  int m_headBuf; // The oldest outstanding buffer
  Long m_buf[MHgdMaxBuf]; // Message buffer pointers
  Long m_maxUpdSz; // Maximum size of a single update
  Long m_msgBufSz; // Size of message update buffer
  Long m_bufferSz; // Size of data buffer for seqential updates
  U_long m_nInvalidates; // number of invalidates
  U_long m_InvalidateBytes; // bytes invalidated
  U_long m_InvalidateGig; // Gig invalidated
  U_long m_nMoves; // number of memmoves
  U_long m_MoveBytes; // bytes moved
  U_long m_MoveGig; // Gig Moved
  U_long m_nWaitBuf; // Number of times waitting for buffers
  U_long m_nWaitOp; // number of times waiting for Operations
  U_long m_nWaitMsgBuf; // Number of times waiting for buffers
  U_long m_nMsgWaitOp; // Number of times waiting for msg send
  U_long m_SendFailed; // Number of failed sends
  MHgdMsg m_delSend[MHgdMaxSend]; // Delayed send messages
  MHgdOp m_Ops[MHgdMaxOps]; // Table of pending operations should be
                            // next to last
  Char m_Data[1]; // Start of the real shared memory data area
};

enum MHgdAudAction {
  MHgdRptOnly,
  MHgdFix,
  MHgdBoot
};

struct MHregGd;
struct MHgdSyncReq;
struct MHgdSyncData;
struct MHgdUpd;
struct MHaudGd;
struct MHrt;

class MHgd {
  friend class MHinfoInt;
  friend void MHprocessmsg(MHmsgBase*, Long);
public:
  MHgd();
  GLretVal attach(const char* name, Bool bCreate, Bool& isNew,
                  int permissions,
                  LongLong size,
                  char *& pAttached,
                  Long bufferSz = 3 * MHdefUpdSz,
                  void* AtAddress = (void*)NULL,
                  const MHgdDist dist = MHGD_CCONLY,
                  Long maxUpdSz = MHdefUpdSz,
                  Long msgBufSz = 0,
                  Bool delaySend = TRUE);
  void invalidateCmp(const void* pStartAddress, Long length);
  inline void invalidateCmp(const void *pStartAddress, Long length, int cmpOffset, MHqid notifyQid) {
    invalidateCmpInt(pStartAddress, length, cmpOffset, notifyQid);
  }
  inline void invalidateSeq(const void *pStartAddress, Long length) {
    invalidateSeqInt(pStartAddress, length, FALSE);
  }
  void memMove(const void* to, const void* from, Long length);
  inline void memMoveSeq(const void* to, const void* from, Long length) {
    memMoveSeqInt(to, from , length, FALSE);
  }
  void send(MHqid qid, char* msgp, Long size);
  GLretVal detach();
  Void setSync(Bool doSync);
  Void syncAll(U_long* progress = NULL);
  Void syncAllExt(U_long* progress = NULL, Bool ReadAllMsgs = FALSE);
  static GLretVal remove(const char *name);
  GLretVal audit(MHgdAudAction action = MHgdBoot,
                 const void* pStartAddress = NULL, Long length=0);
  pthread_t m_audthread; // Audit thread

private:
  void invalidateCmpInt(const void * pStartAddress, Long length , int cmoOffset, MHqid notifyQid);
  void invalidateSeqInt(const void* pStartAddress, Long length, Bool bLocked);
  void invalidateSeqCmpInt(const void* pStartAddress, Long length, int comOffs, int notifyQid, Bool bLocked);
  void memMoveSeqInt(const void* to, const void* from, Long length, Bool bLocked);
  void getUpdBuf(MHgdUpd*& msgp, Bool bWait=TRUE);
  void invCmpCheck(MHqid notifyQid, const void* remote, void* local, Long length, int offset);
  MHrt* m_prt; // Pointer to MSGH shared memory
  int m_shmid; // shared memory id used by this GDO
  MHgdShm * m_p; // Pointer to the object shared memory

public: // The following mehtods are implemented in MSGH and MHRPROC
  // processes and cannot be called from the library
  GLretVal create(MHregGd* msg, MHrt* rt, IN_SHM_KEY shmkey, Bool bDoSynch=FALSE);
  GLretVal attach(int shmid, MHrt* rt, int lock=-1);
  Char* getName() {
    if (m_p != NULL) {
      return (m_p->m_Name);
    } else {
      return (NULL);
    }
  }
  Void syncReg(MHgdSyncReq* syncReg, MHqid myqid);
  Void syncData(MHgdSyncData* syncData, MHqid myQid);
  Void doUpdate(Bool &bSync, MHqid mhQid, int hostid=MHmaxHostReg);
  Void handleUpdate(MHgdUpd* updmsg, MHqid myQid);
  static void updateNone(MHgdUpd* updmsg);
  void audMutex();
  void audit(MHaudGd * audmsg, MHqid myqid);
};

// same size used in RDBtuple.hh
#define MAXRDBTUPLESIZE 2300

class MHraceNotifyMsg: public MHmsgBase {
public:
  MHraceNotifyMsg() {
    priType = MHoamPtyp;
    msgType = MHraceNotifyTyp;
    srcQue = MHnullQ;
    recLength = 0;
    foreignCounter = 0;
    localCounter = 0;
    buffer[0] = '\0';
  };
  void setKeptRecord(const void* addr, Long length) {
    memcpy(buffer, addr, length);
    recLength = length;
  };
  void setDiscardedRecord(const void * addr, Long length) {
    memcpy(buffer+length, addr, length);
    recLength = length;
  };
  void * getKeptRecord() { return buffer; }
  void * getDiscardedRecord() {return (buffer + recLength);}
  short getSendSize() {
    return (sizeof(MHraceNotifyMsg) - (MAXRDBTUPLESIZE * 2) + (recLength * 2));
  }

public:
  short recLength;
  unsigned short foreignCounter;
  unsigned short localCounter;
  char buffer[MAXRDBTUPLESIZE * 2];
};

#endif
