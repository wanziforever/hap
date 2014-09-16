#ifndef _MHNETSTCHG_H
#define _MHNETSTCHG_H

#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHmtypes.hh"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"

class MHnetStChg : public MHmsgBase {
public:
  MHnetStChg() {
    priType = MHoamPtyp;
    msgType = MHnetStChgTyp;
  };
  inline GLretVal send(MHqid fromQid);
};

inline GLretVal MHnetStChg::send(MHqid fromQid) {
  srcQue = fromQid;
  return MHmsgh.send("FTMON", (Char *)this, sizeof(*this), 0L);
}

#endif
