#ifndef _MHNODESTCHG_H
#define _MHNODESTCHG_H

#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHmtypes.hh"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHnames.hh"

class MHnodeStChg : public MHmsgBase {
public:
  MHnodeStChg() {
    priType = MHoamPtyp;
    msgType = MHnodeStChgTyp;
  };
  char hostname[MHmaxNameLen + 1];
  Bool isActive;
};

#endif 
