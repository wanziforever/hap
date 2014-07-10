#ifndef __MHPRIORITY_H
#define __MHPRIORITY_H

// DESCRIPTION:
//  The MSGH supports priority queue capability.
//  This file contains a number of message priority types.
//  Priority types are currently an U_short.

#include "hdr/GLtypes.h"

enum {
  MHsanityPtyp = 1, // priority type of Sanity messages
  MHdebugPtyp = 2, // priority type of SLL Debugging messages
  MHcpPtyp = 10, // priority type of CP messages
  MHoamPtyp = 20, // priority type of OA&M messages
  MHmsghPtyp = 100, // priority type of MSGH internal messages
  MHinitPtyp = 150, // priority of INIT internal messages
  MHdbiPtyp = 200, // priority type of DB internal messages

  MHsignalSoftStartPtyp = 300,  // start of block of
                                // SignalSoft priorities
  MHsignalSoftStopPtyp = 699,   // end of block of SignalSoft
                                // priorities
  // Constant representing maximal priority type
  MHmaxPtyp = MHsignalSoftStopPtyp
};

#endif

