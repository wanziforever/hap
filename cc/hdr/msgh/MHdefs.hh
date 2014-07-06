#ifndef _MHDEFS_H
#define _MHDEFS_H

// maximal number of hosts registered to MSGH. Can't be more than
// 64 since that would make some of the Short mhqid's negative.
// and a lot of code checks for negative MHqid as an error or null.

#define MHmaxHostReg 1000 // Maximum number of real hosts
#define MHmaxAllHosts 1024 // Maximum number of hosts including
                           // virtual hosts
#define MHgQHost 1023 // Global queue host id
#define MHdQHost 1022 // Distributive queue host id
#define MHmaxRealQs 32 // Maximum number of real Qs mapped to global Q
#define MHmaxDistQs 256 // Maximum number of real Qs mapped to distributive Q

#define MHmsghQ 0 // Fixed queue for MSGH
#define MHrprocQ 1 // Fixed queue for MHRPROC
#define MHlongAlign 0xfffffffffffffff8LL // mask for long aligned address
#define MHintAlign 0xfffffffffffffffcLL // mask for int aligned address

#define MHlongLeft 0x3 // Mask for long leftover bits
#define MHmaxNets 2 // Number of networks supported

#endif


