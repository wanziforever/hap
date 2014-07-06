#ifndef __INSHMEM_H
#define __INSHMEM_H

// DESCRIPTION:
//  This file includes the definition of the shared memory keys.

// Max number of shared memory segments allocated for system
#define IN_NUMSHMIDS 101

// Shared memory keys used by shared memory class to identify specific
// users of private memory segments for later retrieval.

typedef enum {
  IN_GD_MAX = 300, // Key range reserved for GDO memory
  IN_PRIVATE, // No key is specified for private shared memory
  IN_MSGH_KEY, // Large message sender shared memory
  IN_MALLOC_KEY, // Shared memory used by shared memory based malloc()
  IN_SFSCH_KEY, // SFSCH configuration shared memory
  IN_TSSCH_KEY, // TSSCH configuration shared memory
  IN_EXT1_KEY, // Spare keys which allow adding private shared
  IN_EXT2_KEY, // memory segments by external users of INIT library
  IN_EXT3_KEY, // without requiring that this header is updated
  IN_EXT4_KEY, // and causing rebuild of INIT library
  IN_EXT5_KEY, // Those keys only have to be unique per process and
  IN_EXT6_KEY, // multiple processes can use the same private keys.
  IN_MAX_KEY   // Maximum key value
} IN_SHM_KEY;

#endif
