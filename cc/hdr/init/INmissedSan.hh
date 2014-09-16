#ifndef __INMISSEDSAN_H
#define __INMISSEDSAN_H


// DESCRIPTION:
// 	This file contain a definition of the "INmissedSan" class.  This
//	class defines the message sent by INIT to processes which fail
//	to peg their sanity timer.
//
// NOTES:
//

#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHmsgBase.hh"

/*
 *  The "INsanMsg" class defines the message type sent by INIT to processes
 *  when they fail to peg their sanity flag.  Processes which recieve
 *  this message should peg their sanity flag via the "IN_SANPEG()"
 *  macro defined in "cc/hdr/init/INusrinit.h":
 */
class INmissedSan : public MHmsgBase { };

#endif
