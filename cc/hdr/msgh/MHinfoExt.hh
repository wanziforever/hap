#ifndef __MHINFOEXT_H
#define __MHINFOEXT_H

// DESCRIPTION:
//  (1) Definition of maximal MSGH message size (MHmsgSz), and other
//      constaints used by MSGH.
//  (2) Definition of a class - MHinfoExt, which stored and manages
//      information required to support the MSGH communication mechanisms
//  (3) External declaration of an instance (MHmsgh) of this class.
//
//  NOTES:
//   All the MSGH functions will be invoked via the member functions of
//   MHmsgh - MHmsgh.attach(..), MHmsgh.detach(..), MHmsgh.getMhqid(..),
//   MHmsgh.receive(..), MHmshg.regName(..), MHmsgh.rmName(..),
//   MHmsgh.send(..), and MHmsgh.broadcast(..).


#include <sys/types.h>
#include <netinet/in.h>
#include "pthread.h"
//#include "synch.h"
#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHresult.hh"
#include "cc/hdr/msgh/MHnames.hh"
#include "cc/hdr/msgh/MHqid.hh"

class MHrt; // routing table class

// maximal recommended MSGH message size
#define MHmsgSz 3800
// maxinal message size that can use standard IPC mechanism
#define MHmsgLimit 32752
// maximal number of registered names allowed for a process
#define MHmaxNameLoc 20
// max no of error values
#define MHmaxErrVal 150
// Broadcast to all nodes in all clusters
#define MHallClusters 126
// Maximum nodes per system
#define MHmaxRCSName 6

enum MHregisterTyp {
  MH_GLOBAL, /* Register within a system if defined , otherwise register
                in all known nodes */
  MH_LOCAL,  /* Register on this machine only. */
  MH_DEFAULT, /* Register to default - GLOBAL for libplat users,
                 LOCAL for libplatn users */
  MH_CLUSTER_GLOBAL /* Register within all nodes in the cluster regardless
                       if system is defined or not */
};

enum MHenvType {
  MH_standard, /* Standard, non-CSNclustered environment */
  MH_TScluster, /* Clustered TS environment - not supported */
  MH_peerCluster /* Peer to peer communication only */
};

enum MHoamMemberType {
  MH_oamAll, /* All members of OA&M cluster */
  MH_oamLead, /* Return only members capable of being OA&M lead */
  MH_oamOther /* Return only members not capable of being OA&M lead */
};

enum MHclusterScope {
  MH_scopeSystemAll, /* All member of a system */
  MH_scopeSystemPilot, /* Only oam lead (pilot) nodes */
  MH_scopeSystemOther, /* Only ono-pilot nodes */
  MH_scopeAll /* All nodes in all systems */
};

#define MH_clusterLocal 0 /* Local global queue */
#define MH_clusterGlobal 1 /* Global global queue */
#define MH_systemGlobal 2 /* System global queue */

struct MHqStats {
  Long nBytes; /* Bytes currently in the queue (could be negative) */
  Long nBytesHigh; /*  high water mark for number of bytes */
  U_long nBytesRcvd; /* Number of bytes received */
  U_long nCountRcvd; /* Number of messages received */
  Short nCount; /* Number of message in the queue */
  Short nCountHigh; /* High water mark to number of messages */
  Short nCountLimit; /* Limit on number of message allowed */
};

class MHinfoExt {
public:
  friend class MHgd;
  friend class MHinfoInt;
  MHinfoExt();
  GLretVal attach(char *attach_address = 0); // attach to MSGH subsystem
  GLretVal detach(); // detach from the MSGH subsystem
  GLretVal getMhqid(const Char *name, MHqid &mhqid, Bool unused=FALSE) const;
  GLretVal getMhqidOnNextHost(Short& hostid, const Char *name,
                              MHqid &mhqid) const;
  GLretVal getMhqidOnNextHost(Short& hostid, const Char *name, MHqid &mhqid,
                              MHclusterScope scope) const;
  GLretVal getName(MHqid mhqid, Char *name, Bool unused=FALSE) const;
  GLretVal regName(const Char *name, MHqid &mhqid, Bool isCondReg=TRUE,
                   Bool getnewQ=FALSE, Bool LargeMsg=FALSE,
                   MHregisterTyp=MH_DEFAULT, Bool RcvBroadcast=TRUE,
                   int *sigFlg=(int*)0);
  // clusterGlobal argument is defined as Bool, for SU compatiblity it is
  // left as is however it can take 3 values, MH_clusterLocal, MH_clusterGlobal
  // and MH_systemGlobal
  GLretVal regGlobalName(const Char *name, MHqid realQ, MHqid& globalQ,
                         Bool bkeepOnLead=TRUE,
                         Bool clusterGlobal=MH_clusterLocal);
  // register for distributive queue
  GLretVal regDistName(const Char *name, const MHqid realQ, MHqid &distQueue);
  // Enable distributive queue state
  GLretVal setDistState(const MHqid distQueue, const MHqid realQ, const Bool enable);
  // Get distributive queue enable status
  GLretVal getDistState(const MHqid distQueue, const MHqid realQ, Bool& enable);
  GLretVal getNextQ(const MHqid queue, int& index, MHqid& realQ, Bool enableOnly=TRUE);
  int sendToAllQueues(const MHqid queue, char *msgp, Long msgsz, Bool enableOnly=TRUE,
                      Long time=0, Bool buffered=TRUE);
  // Gets the real queue currently to this global queue
  GLretVal global2Real(MHqid globalQ, MHqid &realQ);
  // get logical name of my host i.e cc0
  GLretVal getMyHostName(char* name);
  // return logical name for a specific hostid
  GLretVal hostid2Name(Short hostid, char* name);
  // get hostid from logical name
  GLretVal name2HostId(Short& hostid, const char* name);
  // returns host id of the lead CC
  Short getLeadCC();
  // TRUE if onl lead CC
  Bool onLeadCC();
  const char *myNodeState();
  // Private, can only be called by INIT
  Void setLeadCC(MHqid leadcc);
  // Private, can only called by INIT
  Void setOamLead(MHqid oamlead);
  // Private, can only be called by INIT
  Void setActiveVhost(MHqid vhostlead);
  GLretVal rmName(MHqid mhqid, const Char *name, Bool isUncondRm=TRUE);
  GLretVal send(MHqid, Char *msgp, Long msgsz, Long time=0,
                Bool buffered=TRUE) const;
  GLretVal send(const Char *name, Char *msgp, Long msgsz, Long time=0,
                Bool buffered=TRUE) const;
  GLretVal sendToNextHost(Short& hostid, const Char *name, Char *msgp,
                          Long msgsz, Long time=0, Bool buffered=TRUE) const;
  GLretVal sendToNextHost(Short& hostid, const Char *name, Char *msgp,
                          Long msgsz, MHclusterScope scope, Long time=0,
                          Bool buffered=TRUE) const;
  int sendToAllHosts(const Char *name, Char *msgp, Long msgsz, Long time=0,
                     Bool buffered=TRUE) const;
  int sendToAllHosts(const Char *name, Char *msgp, Long msgsz,
                     MHclusterScope scope, Long time=0, Bool buffered=TRUE) const;
  GLretVal receive(MHqid mhqid, Char *msgp, Short &msgsz, Long ptype=0,
                   Long time=0) const;
  GLretVal receive(MHqid mhqid, Char *msgp, Long &msgsz, Long ptype=0,
                   Long time=0) const;
  GLretVal broadcast(MHqid sQid, Char *msgp, Short msgsz, Long time=0,
                     Bool bAllNodes=FALSE);
  GLretVal broadcast(MHqid sQid, Char *msgp, Short msgsz,
                     MHclusterScope scope, Long time=0, Bool bAllNodes=FALSE);
  GLretVal checkPid(MHqid msgh_qid, pid_t &pid) const;
  GLretVal setEnetState(short nEnet, Bool bIsUsable);
  GLretVal setQueLimits(MHqid qid, int nMessages, int nBytes);
  // this function return the real hostname when given logical name
  GLretVal getRealHostname(const Char *hostname, Char* realname);
  GLretVal getLogicalHostname(const Char* realname, Char* hostname);
  static GLretVal IsReg(const Char *name);

  // isHostActive takes the hostname as an argument, and if
  // it returns GLsuccess, will set isactive to true if the machine
  // is active (false otherwise)
  GLretVal isHostActive(const Char *hostname, Bool &isactive) const;

  // This function gets the local host index
  Short getLocalHostIndex();
  GLretVal Status(int hostid, Bool &isUsed, Bool& isActive);
  // Extract hostid from the queue id
  Short Qid2Host(MHqid mhqid);
  // Extract qid component from the queue it
  Short Qid2Qid(MHqid mhqid);
  // only used by MHrproc, not public usage
  MHrt* getRt();
  // obtains socket for intermachine communication, only
  static GLretVal GetSocket();
  // Only INIT process calls this method
  Bool audMutex();
  GLretVal getNetState(Short &hostid, Bool &net0, Bool &net1);
  GLretVal getEnvType(MHenvType &env);
  GLretVal getIpAddress(const Char *lname, unsigned int inum, sockaddr_in6& addr);
  // TRUE if the logical host name is a CC
  Bool isCC(const char *name);

  // Rretreive measurement statistics for a queue
  GLretVal getQidStats(MHqid qid, MHqStats& qdata);
  // Rreset queue statistics
  GLretVal resetQidStats(MHqid qid);
  GLretVal emptyQueue(MHqid qid);
  int getPercentLoadOnActive();
  GLretVal setPreferredEnv(MHenvType env);
  GLretVal convQue(MHqid& que, Short hostid);
  Bool isOAMLead();
  Bool isVhostActive();
  Short getOAMLead();
  Short getActiveVhost();
  GLretVal systemToLogical(const Char* system, Char* logical);
  GLretVal logicalToSystem(const Char* logical, char* system);
  GLretVal flush(MHqid mhqid);

private:
  GLretVal getMyQid(MHqid& mhqid);
  GLretVal regQueue(const Char* name, MHqid realQ, MHqid& virtualQ,
                    Bool bKeepOnLead, Bool ClusterGlobal, Bool isGlobal);
  // Map the UNIX error to an MSGH error
  static GLretVal MHmapError(int, short, MHqid qid=MHnullQ);
  static Bool isAttach; // =TRUE if attached to MSGH
  static MHrt *rt; // point to routing tables
  static MHqid msghMhqid; // MSGH process's mhqid
  static pid_t pid; // process id
  static U_short nameCount; // number of names for this process
  static int sock_id; // Socket number for UDP comm
  static U_short lmsgid; // long message id
  static int *pSigFlg; // Flag to check prior to calling msgrcv
  static pthread_mutex_t m_lock; // Used to make send thread safe
  static char* m_buffers; // Address of buffer shared memory

  static char *m_free256;
  static char *m_free1024;
  static char *m_free4096;
  static char *m_free16384;
  static char *m_buffer256;
  static char *m_buffer1024;
  static char *m_buffer4096;
  static char *m_buffer16384;

  static MHenvType prefEnv; // Preferred environment type
};

// Macro and typedef for declaring a buffer to receive messages.
// Forces alignment. Use MHlocBuffer for locally declared buffers
// and MHglobBuffer for globally declared buffers. Note that for
// global buffers you must append a ".buffer" onto the variable
// name to use the buffer.

// IBM waynepar 20061115 syntax
//
// IBM swerbner 20061203
// Make Wayne's linux change common to solaris y linux.
// Eliminate l_align, d_align, p_align names. Preserve anomynous union
#define MHlocBuffer(buffer)                     \
  union {                                       \
    long     l_align;                           \
    double   d_align;                           \
    void *   p_align;                           \
    char     buffer[MHmsgSz];                   \
  };
// IBM Swerbner 20051121 end change
// IBM waynepar 20061115 end change
  
typedef union {
  long     l_align;
  double   d_align;
  void*    p_align;
  char     buffer[MHmsgSz];
} MHglobBuffer;

extern MHinfoExt MHmsgh; // information control object

#endif
