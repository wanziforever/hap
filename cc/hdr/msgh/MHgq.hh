#ifndef _MHGQ_H
#define _MHGQ_H

#include "hdr/GLtypes.h"
#include "cc/hdr/init/INinit.hh"
#include "cc/hdr/msgh/MHmtypes.hh"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"

// MHgqRcv message
// This class defines the message that the process will receive when
// it receives the ownership of the global queue. Only the processes
// that register for the global queue will get this message.
// The receiveing process must complete any addtional initialization
// necessary when gettingg the global queue and reply with MHgqRcvAck.
// if MHgdRcvAsk time out, the process will be reinitialized.
// The global queue will not be available until the MHgqRcvAck is send.

class MHgqRcv : public MHmsgBase {
public:
  MHgqRcv() {
    priType = MHoamPtyp;
    msgType = MHgqRcvTyp;
    level = SN_LV1;
    gqid = MHnullQ;
  };
  inline MHgqRcv(SN_LVL lvl, MHqid qid) {
    priType = MHoamPtyp;
    msgType = MHgqRcvTyp;
    level = lvl;
    gqid= qid;
  };
  inline GLretVal send(MHqid toQid, MHqid fromQid);
  inline GLretVal send(Char *name, MHqid fromQid);
  SN_LVL level;
  MHqid gqid;
};

inline GLretVal MHgqRcv::send(MHqid toQid, MHqid fromQid) {
  srcQue = fromQid;
  return MHmsgh.send(toQid, (Char *)this, sizeof(MHgqRcv), 0L);
}

inline GLretVal MHgqRcv::send(Char *name, MHqid fromQid) {
  srcQue = fromQid;
  return MHmsgh.send(name, (Char *)this, sizeof(MHgqRcv), 0L);
}

class MHgqRcvAck : public MHmsgBase {
public:
  MHgqRcvAck() {
    priType = MHoamPtyp;
    msgType = MHgqRcvAckTyp;
    srcQue = MHnullQ;
  };
  inline MHgqRcvAck(MHqid qid) {
    priType = MHoamPtyp;
    msgType = MHgqRcvAckTyp;
    srcQue = MHnullQ;
    gqid = qid;
  };
  inline GLretVal send(MHqid toQid);
  MHqid gqid;
};

inline GLretVal MHgqRcvAck::send(MHqid toQid) {
  return MHmsgh.send(toQid, (Char *)this, sizeof(MHgqRcvAck), 0L);
}

class MHfailover : public MHmsgBase {
public:
  MHfailover() {
    priType = MHoamPtyp;
    msgType = MHfailoverTyp;
  };
  int planned; // True if failover planned, not ad bard hang
};

#endif
