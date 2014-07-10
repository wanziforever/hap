#ifndef __MHNAMES_H
#define __MHNAMES_H

// DESCRIPTION:
//  The number of characters in each registered name. recognized by
//  the MSGH, is defined by MHmaxNameLen. Therefore, the first
//  MHmaxNameLen characters of each registered name must be unique.
//  Note that this name restriction is only for the Queue name.
//  The machine name can be another MHmaxNameLen bytes long.

#include "hdr/GLtypes.h"

// Number of characters in a name recognized by the MSGH
// If this is ever changed, be sure to update the version
// for the shared memory definition in cc/hdr/init/INproctab.h!!!
// Note - this value should be one less than a multiple of four
// to make some internal MSGH messages align.

#define MHmaxNameLen 15

// max MSGH qid - Used by INIT for a range check. qids are valid from
// 0 through MHmaxQid. Processes assigned qids in the INIT list should
// start at 0 and go up through MHmaxPermProc. "Temporary" processes
// started by init through a INprocCreate should use msgh qids from
// MHminTempProc through MHmaxQid. Non-INIT started processes are
// given random QIDs starting at MHminTempProc-1 and going down, so
// be sure to leave plenty of space for them!
#define MHmaxPermProc 100 // Maximum preassigned qid for a perm process
#define MHminTempProc 700 // Minium qid that can be used by an SPA
#define MHmaxQid 1024 // Total number of queues per node
#define MHmaxgQid 350 // Maximum number of global qids
#define MHclusterGlobal 70 // Maximum number of cluster global queues
#define MHsystemGlobal (MHclusterGlobal + MHmaxgQid)
#define MHmaxdQid 100 // Maximum number of distributive qids

#endif
