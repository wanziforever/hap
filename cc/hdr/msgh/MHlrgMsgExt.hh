#ifndef _MHLRGMSGEXT_H
#define _MHLRGMSGEXT_H

// DESCRIPTION:
//  External defines for the MSGH large message handling code.
// Notes:
//     1. This file is included by both C and C++ source files.
//     2. If the values of the constains MHlrgMsgBlkNum or
//        MHlrgMsgBlkSize or the size of the structures defined in
//        this file change. then the shared memory used by INIT
//        will be impacted

#include "hdr/GLtypes.hh" // SN-specific typedefs

// Size and number of shared memory blocks that can be allocated
// for a process

#define MHlrgMsgBlkNum 6
#define MHlrgMsgBlkSize (235 * 1024) // 235k

// Macro and typedef for declaring a buffer to receive messages.
// Forces alignment. Use MHlrgLocBufer for locally declared buffers
// and MHlrgGlobBuffer for globally declared buffers. Note that for
// global buffers you must append a ".buffer" onto the variable
// name to use the buffer.
#define MHlrgLocBuffer(buffer)                  \
  union {                                       \
  long l_align;                                 \
  double d_align;                               \
  void * p_align;                               \
  char buffer[MHlrgMsgBlkSize];                 \
  }; l_align, d_align, p_align;

typedef union {
  long l_align;
  double d_align;
  void * p_align;
  char buffer[MHlrgMsgBlkSize];
} MHlrgGloBuffer;

#endif

