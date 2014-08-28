#ifndef _MHGDAUD_H
#define _MHGDAUD_H

#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/msgh/MHmsg.hh"

class MHgdAud : public MHmsgBase {
public:

  MHgdAud() {
    priType = MHintPtyp;
    msgType = MHgdAudTyp;
  };

  Bool m_bSystemStart;
  struct {
    MHname m_Name;
    LongLong m_Size;
    Long m_bufferSz;
    Long m_msgBufSz;
    Long m_maxUpdSz;
    int m_Permissions;
    uid_t m_Uid;
    MHgdDist m_Dist;
    Bool m_doDelaySend;
    Bool m_bInUse;
    U_char m_RprocIndex;
    Bool m_cfill;
    int m_ifill;
  } gd[MHmaxGd];
};

class MHgdAudReq : public MHmsgBase {
public:
  MHgdAudReq() {
    priType = MHintPtyp;
    msgType = MHgdAudReqTyp;
  }
  Bool m_bSystemStart;
};

class MHgdSyncReg : public MHmsgBase {
public:
  MHgdSyncReg() {
    priType = MHintPtyp;
    msgType = MHgdSyncReqTyp;
  };
  Bool m_bSystemStart;
};

class MHgdSyncData : public MHmsgBase {
public:
  MHgdSyncData() {
    priType = MHintPtyp;
    msgType = MHgdSyncDataTyp;
  }
  LongLong m_SyncAddress; // Address of the data
  int m_GdId; // Id of the global data object
  Long m_Length;
  Char m_Data[1];
};

#define MHmaxDataSz (MHmsgLimit - sizeof(MHgdSyncData))

#endif
