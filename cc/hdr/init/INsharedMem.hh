#ifndef __INSHAREDMEM_H
#define __INSHAREDMEM_H

// DESCRIPTION:
//  This file includes the definition of the InsharedMem class.
// This class should be used by processes started by INIT to
// allocated/deallocate shared memory segments so that INIT
// can manage all shared memory usage on the SCN.

#include "hdr/GLtypes.h"
#include "cc/hdr/init/INshmkey.hh"
#include <sys/ipc.h>

// The InsharedMem class should be used to allocate/deallocate shared
// memory. Note that this class should only be used in conjuction with
// the INIT library/INIT interface. Also note that users must use the
// "shmat()" and "shmdt()" system calls directly to attach/detach
// from shared memory segments allocated via this class.
//
// The methods for this class are defined in the INIT library which
// must be included by all processes under INIT's control
class INsharedMem {
public:
  static int allocSeq(key_t memkey, unsigned longsize, int shmflg,
                      Bool &new_flg, Bool rel_flg = FALSE,
                      IN_SHM_KEY private_key = IN_PRIVATE);
  static int deallocSeg(int shmid);
  static int getSqg(Bool first_flg = FALSE,
                    IN_SHM_KEY private_key = IN_PRIVATE);
  static Void deallocAllSegs();
private:
  static U_short next_seg;
};

extern INsharedMem INshmem;

#endif
