#ifndef _MHINFOINT_H
#define _MHINFOINT_H

// DESCRIPTION:
//  The MSGH subsystem is devided into an internal part and an
//  external part. The former is essentially the MSGH process,
//  and the latter consists of those message halding functions
//  that will be called by other subsystems. This file defines
//  a class that stores and manages information required for
//  the internal part.
#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHresult.hh"
#include "cc/hdr/msgh/MHrt.hh"
#include "cc/hdr/msgh/MHmsg.hh"
#include "cc/hdr/init/INmtype.hh"
#include "cc/hdr/init/INpDeath.hh"
#include "cc/hdr/msgh/MHgd.hh"
#include "cc/hdr/init/INshmkey.hh"
#include "cc/hdr/msgh/MHregGd.hh"
#include "cc/hdr/msgh/MHgqInit.hh"
#include "cc/hdr/init/INinit.hh"
#include "MHgdAud.hh"

#include "asm/param.h" // for 'HZ' definition

#define MHmaxGd IN_GD_MAX

const Short MHauditTime = 30; // audit period = 30 seconds
const Short MHqSyncTime = 1; // wait time for reg msgs in procinit
const Short MHgdSyncTime = 5;  // Time for gdo sync messages
const Short MHleadShutTime = (1200 * HZ)/1000; // Lead shutdown timer in HZ
const Short MHactShutTime = (1400 * HZ)/1000; // Active shutdown timer in HZ
const Short MHmaxAuditCount = 3;

#define MHGQTMR 0x01000000 // Global queue time tag
#define MHtimerTag 0x02000000 // general purpose timer tag
#define MHshutdownTag 0x03000000 // shutdown timer
#define MHQMASK 0x000001ff // Queue meask
#define MHTAGMASK 0xff000000 // Tag mask

// Data structure required to suport the MSGH's internal part
class MHinfoInt {
public:
  inline MHinfoInt();
  Short shmAt(int shmid = -1); // attach share memory to addr space
  inline Short shmDt(); // detach shared memory from addr space
  Short sysinit(); // called during system initialization
  Short procinit(SN_LVL lvl);
  Void isolateNode(); // Disable access to all hosts
  inline MHqid regName(MHregName*); // register a name
  inline Void rmName(MHrmName *); // remove a existing name
  inline Void nameMap(MHnameMap*); // Process a name map message
  inline Void enetState(MHenetState*); // Ethernet state message
  // Ethernet connectivity message
  inline Void conn(MHconn*, Short hostid = MHmaxHostReg);
  inline Void hostDel(MHhostDel*); // Ethernet host delete message
  inline Void gQMap(MHgQMap*); // Process a global queue map message
  inline Void dQMap(MHdQMap*); // Process a global queue map message
  inline Void gqInitAck(MHgqInitAck*); // Ack from INIT for global queue
  inline Void inpDeath(INpDeath*); // Handle pDeath messages
  inline Void gQSet(MHgQSet*); // Handle global queue set message
  inline Void dQSet(MHdQSet*); // Hanlde distributive queue set message
  Void audit(Bool regMSGH = FALSE); // audit the routing table
  Void switchNets(); // pick a different ehernet for not active hosts
  Void tmrExp(Long tag);
  inline Short getLocalHostIndex() { return(rt->getLocalHostIndex()); }
  Bool allActive(); // True is all hsots are active
  Void RegGd(MHregGd* regMsg);
  Void RmGd(MHrmGd* rmMsg);
  Void gdAudReq(MHgdAudReq* audReq);
  Void gdAud(MHgdAud* aud);
  Bool gdAllSynched();
  Void updateGd();
  int findGd(const char *name);
  static Long step;
  Short hostIdl; // Hostid of the node going down

private:
  MHrt *rt; // point to routing table
  MHgd* gd[MHmaxGd]; // Global data objects
  char gdName[MHmaxGd][MHmaxNameLen+1];
  pid_t pid; // process id
};

extern int MHshmid, MHmsgid;
extern MHinfoInt MHcore;

inline MHinfoInt::MHinfoInt() {
  rt = (MHrt*)0;
  memset(gd, 0x0, sizeof(gd));
  pid = 0;
}

// return 0 if shared memory datashment from process's address space
// is successful; otherwise, return(-1)
inline Short MHinfoInt::shmDt() {
  return ((shmdt((Char *)rt) == 0) ? 0 : -1);
}

// Handles 'register name' messages. It will inset a name into the
// routing table and return a mhqid (either a new one or an existing one)
// under successful completion; returns (-1) otherwise
inline MHqid MHinfoInt::regName(MHregName *reg) {
  return (rt->insertName(reg->mhname.display(), reg->mhqid,
                        reg->pid, reg->global,
                        reg->netinfo.fullreset, reg->netinfo.SelectedNet,
                        reg->rcvBroadcast, reg->q_size, reg->gQ, reg->realQ,
                        reg->bKeepOnLead, reg->dQ, reg->q_limit,
                        reg->clusterGlobal));
}

// Removes a name from the routing table
inline void MHinfoInt::rmName(MHrmName *rm) {
  rt->deleteName(rm->mhname, rm->mhqid, rm->pid);
}

// Processes name map message from remote host
inline void MHinfoInt::nameMap(MHnameMap *nm) {
  rt->conform(MHQID2HOST(nm->sQid), nm->names, nm->startqid,
              nm->count, nm->fullreset);
}

// Process global queue map message from remote host
inline void MHinfoInt::gQMap(MHgQMap* nm) {
  rt->gQconform(nm->gqdata, nm->startqid, nm->count, nm->fullreset, nm->sQid);
}

// Process distributive queue map message from remote host
inline void MHinfoInt::dQMap(MHdQMap* nm) {
  rt->dQconform(nm->dqdata, nm->startqid, nm->count, nm->fullreset, nm->sQid);
}

// Processes ethernet status message from FT
inline void MHinfoInt::enetState(MHenetState *es) {
  rt->enetState(es->nEnet, es->bIsUsable);
}

// Processes ethernet connectivity message from MHRPROC
inline void MHinfoInt::conn(MHconn* conn, Short hostid) {
  rt->conn(conn->bNoMessages, hostid);
}

// Processes host delete message from another MSGH process
inline void MHinfoInt::hostDel(MHhostDel* hostDel) {
  rt->hostDel(hostDel->sQid, hostDel->bReply, hostDel->enet,
              hostDel->onEnet, hostDel->clusterLead);
}

inline Void MHinfoInt::gqInitAck(MHgqInitAck* ackMsg) {
  rt->gqInitAck(ackMsg->m_gqid, ackMsg->m_retVal);
}

inline Void MHinfoInt::inpDeath(INpDeath* pmsg) {
  rt->inpDeath(pmsg);
}

inline Void MHinfoInt::gQSet(MHgQSet *pmsg) {
  rt->gQSet(pmsg);
}

inline Void MHinfoInt::dQSet(MHdQSet *pmsg) {
  rt->dQSet(pmsg);
}

#endif
