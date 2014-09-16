#ifndef __MHMSGBASE_H
#define __MHMSGBASE_H

// NOTES:
//  Any classes used for messages can not have pointer
//  or virtual functions. (And anything in the class
//  can not contain any.)

#include "hdr/GLreturns.h"
#include "cc/hdr/msgh/MHpriority.hh"
#include "cc/hdr/msgh/MHqid.hh"

class MHmsgBase {
public:
  MHmsgBase() : priType(0), msgType(0) {}
  Void printName();
  Void display();
  GLretVal send(MHqid toQid, MHqid fromQid, Long len,
                Long time, Bool buffered=TRUE);
  GLretVal send(const char *name, MHqid fromQid, Long len,
                Long time, Bool buffered=TRUE);
  Long priType; // priority type
  MHqid srcQue; // source mhqid
  MHqid toQue;  // The qid send TO - used by MSGH for network sends
  Short msgType; // message type
  Short msgSz; // message size - used by MSGH only

  // the following field are for networked MSGH internal use.
  // They should never be referenced by user code
  U_short seq; // Sequence number - internal net. MSGH counts
  U_short rSeq; // received sequence number
  U_char bits; // Could be used for version , swabinfo. etc
  U_char HostId; // Sender hostid
  Short fill;
};

#define MHmsgBaseSz ((short) sizeof(MHmsgBase))

#endif
