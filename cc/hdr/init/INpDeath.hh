#ifndef __IMPDEATH_H
#define __IMPDEATH_H

#include <sys/types.h>
#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/init/INproctab.hh"

// "INpDeath" message class:
// This class defines the message broadcast to all CC processes when
// one of the following types of process events is detected by INIT;
//
// 1) A process dies
//
// 2) A process fails to peg its sanity flag for two consecutive
//    sanity intervals
//
// Note that if either of these events is detected during a system
// initialization (or an increase in the system's run level) then
// this message is not sent util the current initialization is
// completed.

class INpDeath: public MHmsgBase {
public:
  // Since INIT is the only sender for this message, no methods
  // need to be defined here. All processes receiving this
  // message simply need to switch on the message type (INpDeathTyp
  // defined in "cc/hdr/init/INmtype.hh") and cast the message with this
  // class to access the class' data members.

  // message-specific data
  Char msgh_name[IN_NAMEMX]; // Null-terminated MSGH name of process which died
  MHqid msgh_qid; // MSGH QID of process which died
  Bool perm_proc; // TRUE: permanent process type
                  // FALSE: termproary process type
  Bool rstrt_flg; // TRUE: process will be restarted
                  // FALSE: process will NOT be restarted
  Bool upd_flg; // TRUE: process is being updated - every
                // attempt should be made to mainain
                // this process's resources so that the
                // update has minmum impact on srvc
                // FALSE: Plain old death
  pid_t pid; // Process ID of dead process, if known. or -1, if not known
  SN_LVL sn_lvl; // Initialization level that will be used
                 // when the process is restarted.
                 // The level has no meaning if rstrt_flg is FALSE.
};

#endif
