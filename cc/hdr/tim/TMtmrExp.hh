#ifndef __TMTMREXP_H
#define __TMTMREXP_H

// DESCRIPTION:
//  Definition of the "message type" the event handler will
//  use to return timer expired events.

#include "hdr/GLmsgs.h"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/tim/TMmtype.hh"

class TMtmrExp : public MHmsgbase {
public:
  U_long tmrTag; // Tag of expired timer
};

#endif
