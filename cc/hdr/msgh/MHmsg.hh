#ifndef __MHMSG_H
#define __MHMSG_H

// DESCRIPTION:
//  The MSGH subsystem is divided into an internal part and an
//  external part. The former is essentially the MSGH process,
// and the latter consists of some message handling functions
// that will be called by other subsystems. This file defines
// data structures of some messages required to communicate
// between these two parts

#include <sys/types.h>
#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHdefs.hh"
#include "cc/hdr/msgh/MHmtypes.hh"
#include "cc/hdr/msgh/MHname.hh"

const Short MHintPtyp = MHmsghPtyp; // MSGH internal msg priority type
const Short MHregPtyp = MHmsghPtyp + 1; // MSGH registration priority type
                                        // always delivered
#define MHmaxMsg 64000 // Maximum message

// Note: The structures in this file are messages and should have the
// some message beader structure are MHmsgbase even though they so not
// inherit from it

struct MHnetinfo { // Structure of infor for network registration
  Char fill;
  Bool fullreset; // True says to reset list for this host (SNLV4)
  Short SelectedNet;
};

// Register Name Message
struct MHregName {
  Long ptype;   // priority type
  MHqid sQid;   // Source queue
  MHqid toQue;  // The qid send To - used by MSGH for network sends
  Short mtype;  // message type
  Short msgSz;  // message size - used by MSGH only
  U_short seq;  // Sequence number of messages
  U_short rSeq;
  U_char bits;
  U_char HostId;
  U_short fill;
  union {
    pid_t pid; // process id - for local registration
    MHnetinfo netinfo; // Structure of info for network registration
  };
  MHname mhname; // a MSGH name
  Bool global;  // QID sent globally or not
  MHqid mhqid;  // requested QID (-1 for assgin one to me)
  Long q_size;  // Size of this queue
  Short q_limit; // limit for number of messages
  Bool rcvBroadcast; // True if this queue receives broadcast messages
  Bool gQ;  // True if global queue
  Bool dQ;  // True if distributive queue
  MHqid realQ;  // realQ queue for mapping of global qid
  Bool bKeepOnLead; // TRUE if queue should be kept on lead
  Bool clusterGlobal;  // True if cluster global in peer cluster
  Short fill2;
};


// Register Name Ack Message
struct MHregAck {
  Long ptype; // priority type
  MHqid sQid; // Source queue
  MHqid toQue; // The qid send To - used by MSGH for network sends
  Short mtype; // message type
  Short msgSz; // message size - used by MSGH only
  U_short seq;  // sequence number of messages
  U_short rSeq;
  U_char bits;
  U_char HostId;
  Short fill;
  MHqid mhqid; // calling process's mhqid
  Bool reject; // TRUE if registraion failed
};


// Remove Name Message
struct MHrmName {
  Long ptype;  // priority type
  MHqid sQid;  // Source queue
  MHqid toQueue;  // The qid send TO - used by MSGH for network sends
  Short mtype; // message type
  Short msgSz; // message size - used by MSGH only
  U_short seq; // Sequence number of messages
  U_short rSeq;
  U_char bits;
  U_char HostId;
  Short fill;
  MHqid mhqid;
  Short fill2;  // Fill to keep alignments
  pid_t pid;  // process id
  MHname mhname; // a MSGH name
  Long fill4;
};

// Name map audit message - send to make sure all MSGH processes
// are registered ok. Only containes registered global names
struct MHregMap {
  Long ptype;  // priority type
  MHqid sQid; // Source queue
  MHqid toQue; // The qid sent TO - used by MSGH for network sends
  Short mtype; // message type
  Short msgSz;  // message size - used by MSGH only
  U_short seq;  // Sequence number of messages
  U_short rSeq;
  U_char bits;
  U_char HostId;
  short fill;
  Bool fullreset; 
  Bool charfill; 
  Short count;
  int fillint;  // For 8 byte 64 bit alignment
  // the actual map of processes in use on this processor
  char names[MHmaxQid * (sizeof(MHname) + sizeof(Short))];
};

#define MHaudEntries (MHmaxQid/2) + 1

// Name map audit message - send to make sure all MSGH processes
// are registered ok.
struct MHnameMap {
  Long ptype;  // priority type
  MHqid sQid;  // Source queue
  MHqid toQue; // The qid send TO - used by MSGH for network sends
  Short mtype; // message type
  Short msgSz; // message size - used by MSGH only
  U_short seq;
  U_short rSeq;
  U_char bits;
  U_char HostId;
  Short fill;
  Short startqid; // Starting qid for queue information
  Short count; // Number of entries
  Bool fullreset;
  Bool charfill;
  Short fill2; // room for growth
  // the actual map of processes in use on this processor
  MHname names[MHaudEntries]; 
};

struct MHgQData {
  MHname mhname;
  MHqid m_realQ[MHmaxRealQs];
  Char m_selectedQ;
  Bool m_bKeepOnLead;
};

struct MHdQData {
  MHname mhname;
  MHqid m_realQ[MHmaxDistQs];
  Bool m_enabled[MHmaxDistQs];
  Short m_maxMember;
  Short m_nextQ;
};

// Change this in the future if the number of global and distributive
// queues cannot fit in one message
#define MHgQaudEntries (MHmaxgQid + 1)
#define MHdQaudEntries (MHmaxdQid/4)

// Name map audit message - sent to make sure all MSGH processes
// are registered ok.
struct MHgQMap {
  Long ptype;  // priority type
  MHqid sQid;  // Source queue
  MHqid toQue; // The qid sent TO -used by MSGH for network sends
  Short mytpe; // message type
  Short msgSz; // mesasge size - used by MSGH only
  U_short seq;  // Sequence number of messages
  U_short rSeq;
  U_char HostId;
  Short fill;
  Short startqid; // Starting qid for queue information
  Short count; // Number of entries
  Bool fullreset;
  Short fill2;
  // the actual map of processes in use on this processor
  MHgQData gqdata[MHgQaudEntries];
};

// name map audit message - sent to make sure all MSGH processes
// are registered ok.
struct MHdQMap {
  Long ptype;  // priority type
  MHqid sQid;  // Source queue
  MHqid toQue; // The qid send TO - used by MSGH for network sends
  Short mtype; // message type
  Short msgSz; // message size - used by MSGH only
  U_short seq;
  U_short rSeq;
  U_char bits;
  U_char HostId;
  Short fill;
  Short startqid; // Starting qid for queue information
  Short count;   // Number of entries
  Bool fullreset;
  Bool charfill;
  Short fill2;
  // the actual map of processes in use on this processor
  MHdQData dqdata[MHdQaudEntries]; 
};

struct MHgQSet {
  Long ptype;   // priority type
  MHqid sQid;   // Source queue
  MHqid toQue;  // The qid sent TO - used by MSGH for network sends
  Short mtype;  // message type
  Short msgSz;  // message size - used by MSGH only
  U_short seq;
  U_short rSeq;
  U_char bits;
  U_char HostId;
  Short fill;
  MHqid gqid;   // Global queue id
  Short selectedQ;  // selected index
  MHqid realQs[MHmaxRealQs];
};

struct MHdQSet {
  Long ptype;  // priority type
  MHqid sQid;  // Source queue
  MHqid toQue; // The qid sent TO - used by MSGH for network sends
  Short mtype; // message type
  Short msgSz; // message size - used by MSGH only
  U_short seq; // Sequence number of messages
  U_short rSeq;
  U_char bits;
  U_char HostId;
  Short fill;
  MHqid dqid;   // Distributive queue it
  MHqid realQs[MHmaxDistQs];
  Bool enabled[MHmaxDistQs];
  int nextQ;
  Short updatedQ; // set to one queue updated
};

// Change enet state message
struct MHenetState {
    Long ptype;		// priority type
    MHqid sQid;		// Source queue
    MHqid toQue;	// The qid sent TO - used by MSGH for network sends
    Short mtype;	// message type 
    Short msgSz;	// message size - used by MSGH only
    U_short seq;				// Sequence number of messages
    U_short rSeq;
    U_char bits;			
    U_char HostId;
    Short fill;
    Short nEnet;	// enet number
    Bool bIsUsable;	// usability status of enet
    Char fill2;		
};

// Connectivity message
struct MHconn {
    Long ptype;		// priority type
    MHqid sQid;		// Source queue
    MHqid toQue;	// The qid sent TO - used by MSGH for network sends
    Short mtype;	// message type 
    Short msgSz;	// message size - used by MSGH only
    U_short seq;				// Sequence number of messages
    U_short rSeq;
    U_char bits;			
    U_char HostId;
    Short fill;
    Bool bNoMessages[MHmaxHostReg][MHmaxNets];
};

// Host deletion message
struct MHhostDel {
    Long ptype;		// priority type
    MHqid sQid;		// Source queue
    MHqid toQue;	// The qid sent TO - used by MSGH for network sends
    Short mtype;	// message type 
    Short msgSz;	// message size - used by MSGH only
    U_short seq;				// Sequence number of messages
    U_short rSeq;
    U_char bits;			
    U_char HostId;
    Short clusterLead;
    Short enet;		// Enet on which message sent
    Short onEnet;	// Enet that was used for this message
    Bool  bReply;	// True if reply is expected
    Char  fill1;
};

// Reject message
struct MHreject {
    Long ptype;         // priority type
    MHqid sQid;         // Source queue
    MHqid toQue;        // The qid sent TO - used by MSGH for network sends
    Short mtype;        // message type
    Short msgSz;        // message size - used by MSGH only
    U_short seq;				// Sequence number of messages
    U_short rSeq;
    U_char bits;			
    U_char HostId;
    Short fill;
    U_short nMissing;	// Number of messages missing
    Short   fillshort;
};
 
struct MHbcast {
    Long ptype;         // priority type
    MHqid sQid;         // Source queue
    MHqid toQue;        // The qid sent TO - used by MSGH for network sends
    Short mtype;        // message type
    Short msgSz;        // message size - used by MSGH only
    U_short seq;				// Sequence number of messages
    U_short rSeq;
    U_char bits;			
    U_char HostId;
    Short fill;
    Long  time;
    char data[4];	// The real message data, must be always be last
};

#endif
