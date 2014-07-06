#ifndef _MHLRGMSG_H
#define _MHLRGMSG_H

#include <sys/types.h>
#include "hdr/GLtypes.hh"
#include "hdr/GLreturns.hh"
#include "cc/hdr/msgh/MHmsgBase.hh"

// The following is the "signal" that is sent via the normal MSGH queue mechanism
// to the receiver process when the sender process has requested a large message send
// This signal indicates to the receiver that a large message arrived for it
// in the shmem
class MHlrgMsgSignal : public MHmsgbase {
public:
  Short shmblk_indx; // index of the block in shmem containing the "large message"
  Long msgSize; // Size of the large message
};

class MHlrgMsg : public MHmsgBase {
public:
  Long m_size;  // Total size of the large message, NOT the size of m_data
  pid_t m_pid;  // Pid of the sender, srcQid cannot be used since
                // queue is not required to send a message
  U_short m_id; // Id of this message
  U_shor m_seq; // Block sequence number
  Long m_data;  // large message data, must be last
};

#define MHlgDataHdrSz (sizeof(MHlrgMsg) - sizeof(Long))
#define MHlgDataSz (MHmsgLimit - MHlgDataHdrSz)
#endif 
