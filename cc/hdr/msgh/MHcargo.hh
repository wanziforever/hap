#ifndef _MHCARGO_H
#define _MHCARGO_H

#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/msgh/MHmsg.hh"

class MHcargo : public MHmsgBase {
public:
  MHcargo() {
    priType = MHintPtyp;
    msgType = MHcargoTyp;
    m_count = 0;
    msgSz = sizeof(MHcargo);
  };
  int m_count;
};

#endif
