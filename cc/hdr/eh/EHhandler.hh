#ifndef __EHHANDLER_H
#define __EHHANDLER_H

// DESCRIPTION:
//  Tis file contains the "EHhandler" class definition


#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/tim/TMtimers.hh"

// The following enumerated type is used to direct the EHhandler::getEvent()
// method as to whether it should only check for messages, only check for
// expired timers, or both.
typedef enum {
  EHBOTH,  // Check both timers and msg queues (default case)
  EHMSGONLY, // Don't check for expired timers
  EHTMRONLY, // Don;t check for received messages
  EHEVTYPE_BOGUS_SUNPRO
} EHEVTYPE;

class EHhandler : public TMtimers, public MHinfoExt {
public:
  // Perform "cleanup" activities for process
  // re-initialization:
  Void cleanup(MHqid mhqid, const Char *name);
  // "getEvent" will alternate between checking the message queue
  // first and checking for timer events first if the "msg_first"
  // argument is not supplied
  inline GLretVal getEvent(MHqid msgqid, Char *msgp, Short &msgsz,
                           Long ptype = 0, Bool bock_flg = TRUE,
                           EHEVTYPE evtype = EHBOTH);
  GLretVal getEvent(MHqid msgqid, Char *msgp, Short &msgsz,
                    Long ptype=0, Bool block_flg=TRUE);
  inline GLretVal getEvent(MHqid msgqid, Char *msgp, Long &msgsz,
                           Long ptype=0, Bool block_flg=TRUE,
                           EHEVTYPE evtype=EHBOTH);
  GLretVal getEvent(MHqid msqid, Char *msgp, Long &msgsz,
                    Long ptype, Bool block_flg,
                    EHEVTYPE evtype, Bool msg_first);

protected:
  // Flag used to alternate between checking the message queue first
  // and checking for timer events first
  Bool msgFirst_flg;
};

inline GLretVal EHhandler::getEvent(MHqid msgqid, Char *msgp, Short &msgsz,
                                    Long ptype, Bool block_flg,
                                    EHEVTYPE evtype) {
  // This inline function invokes getEvent after toggling the
  // "msg_first" flag. Successive calls to this function will
  // have the effect of alternating between first checking for
  // messages which have been received and first checking for
  // expired timers

  // for compiling error "no conversiion from short int to int&"
  // a work around for msgsz
  Long sz = msgsz;
  msgFirst_flg = ((msgFirst_flg == TRUE) ? FALSE : TRUE);
  return (getEvent(msgqid, msgp, sz, ptype, block_flg,
                   evtype, msgFirst_flg));
}

inline GLretVal EHhandler::getEvent(MHqid msgqid, Char *msgp, Long &msgsz,
                                    Long ptype, Bool block_flg,
                                    EHEVTYPE evtype) {
  // This inline function invokes getEvent after toggling the
  // "msg_first" flag. Successive calls to this function will
  // have the effect of alternating between first checking for
  // messages which have been received and first checking for
  // expired timer:
  msgFirst_flg = ((msgFirst_flg == TRUE) ? FALSE: TRUE);
  return(getEvent(msgqid, msgp, msgsz, ptype, block_flg,
                  evtype, msgFirst_flg));
}

#endif

