#ifndef __TMTIMERS_H
#define __TMTIMERS_H

// DESCRIPTION:
//  This file defines global timer library data elements

#include <unistd.h>
#include <values.h>
#include <sys/param.h>
#include <pthread.h>
#include <synch.h>

// already defined in /usr/include/values.h
//#define MAXSHORT (0x7FFF)

#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "cc/hdr/tim/TMreturns.hh"

// constants go here...
#define TM_DEF_nTCBs 8000 // Default number of TCB

#define TMTCBNUM (TMtimers::nTCBs) // Timer control blocks
const Long TMHZ = HZ;


// TITIME define time, as used internally by OSDS: a 45-bit number of
// milliseconds since timers were last initialized on this processor.
// The low-order 30 bits of an TMITIME are stored in a long, which has
// at least 32 bits. One of thoese bits is left over so that overflow
// is not a problem (an TMITIME is incremented or decremented only by
// small deltas); the sign is simply unused (an "unsigned long" is not
// used because that type is not supported by most c compilers). The
// high-order 15 bits are stored in a short, which has at least 16 bits;
// the sign is left unused, in order to give an easy way to check for
// overflow (which will occur within a little less than 115 years) or trash

typedef struct {
  short hi_time;
  Long lo_time;
} TMITIME;

// TMTSTATE defines the possible TCB states.
typedef enum {
  TEMPTY, // Unused
  ORTIMER, // One-shot relative interval timer
  CRTIMER, // Cyclic relative interval timer
  OATIMER, // One-shot absolute interval timer
  CATIMER, // Cyclic absolute interval timer
  TLIMBO   // Temporarily stateless
} TMTSTATE;

// TMTCB defines a timer control block (TCB) (should be a power of two in size)
typedef struct {
  Long llink; // Left link
  Long rlink; // Right link
  Long thi; // Back link to the heap
  TMTSTATE tstate; // State of the timer
  TMITIME go_off; // When the timer will go off
  TMITIME period; // The period for CTIMERS
  U_long tag; // Timer tag
} TMTCB;

typedef struct {
  Long fftheap; // First free timer heap slot
  Long* theap; // Timer heap
} TMHEAP;

// This class supports relative one-shot and cyclic timers and
// will be modified in the future to support absolute timers.
//
// After the initial implementation, the timer tag has been changed from
// a U_short to a U_long. In order to be backward-compatible with
// the previous version, timer access methods are provided to convert
// between U_short arguments and the internal U_long tag representation.
class TMtimers {
public:
  TMtimers();
  // initialize timers
  GLretVal tmrInit(Bool atim = FALSE, Long nTCB = TM_DEF_nTCBs);
  Void tmrReinit();
  Short setRtmr(Long time, U_short tag,
                Bool c_flag = FALSE, Bool hz_flag = FALSE) {
    if (nTCBs <= MAXSHORT) {
      return((short)setlRtmr(time, (U_long)tag, c_flag, hz_flag));
    } else {
      return(TMNOTSHORT);
    }
  }
  Short setRtmr(Long time, U_long tag,
                Bool c_flag = FALSE, Bool hz_flag = FALSE) {
    if (nTCBs <= MAXSHORT) {
      return((short)setlRtmr(time, tag, c_flag, hz_flag));
    } else {
      return(TMNOTSHORT);
    }
  }
  Long setlRtmr(Long time, U_long tag,
                Bool c_flag = FALSE, Bool hz_flag = FALSE);
  Short setAtmr(Long time, U_long tag, U_char days_per_cycle = 0) {
    if (nTCBs <= MAXSHORT) {
      return((short)setlAtmr(time, tag, days_per_cycle));
    } else {
      return(TMNOTSHORT);
    }
  }
  Long setlAtmr(Long time, U_long tag, U_char days_per_cycle = 0);
  GLretVal tmrExp(U_long *tag, Long *time);
  GLretVal tmrExp(U_short *gag, Long *time); // Original version
  GLretVal clrTmr(Long timer);
  GLretVal stopTime();
  GLretVal startTime();
  inline Long nTimers() { return(nTCBs); };
  Void updoffset();
protected:
  Bool RTinit_flg; // relative timers setup
  Bool ATinit_flg; // absolute timers setup
  Bool time_frozen; // has time been "frozen"

private:
  static Long nTCBs; // Number of TCBs configured
  // mutex to allow timer setting from multiple threads
  static mutex_t tmrLock;
  GLretVal tsched(Long t, TMHEAP* heap_p);
  GLretVal tunsched(Long t, TMHEAP *heap_p);
  Long gettcb();
  GLretVal freetcb(Long t);
  GLretVal updtime(Bool);
  Void sectohz(Long time, TMITIME *interval);
  Void inittime();

  TMTCB* TMbtcb; // Timer control blocks
  TMHEAP TMrtheap; // Relative timer heap
  TMHEAP TMatheap; // Absolute timer heap
  TMITIME TMoffset; // Offset between "time()" and "times()"
                    // return values -- used for abs. tmrs
  Long TMfftcb; // First free tmr control block
  Long TMlftcb; // Last free timer control block

  Long TMidletcb; // TCBs on the free list
  TMITIME TMnow;  // "Current" time in clock ticks
  U_long TMthen; // Last ret value from times()
  TMITIME TMlastupd; // last time TMoffset was updated
  pid_t mypid; // pid to send SIGALRM to when setting timer
};

#endif

