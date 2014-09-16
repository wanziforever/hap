#ifndef __EHRETURNS_H
#define __EHRETURNS_H


// DESCRIPTION:
//	Definition of "EHevent" class functions return constants
//
// OWNER:
//
// NOTES:
//

#include "hdr/GLreturns.h"
#include "cc/hdr/msgh/MHresult.hh"
#include "cc/hdr/tim/TMreturns.hh"

const GLretVal EHNOEVENT   = EH_FAIL;     /* No tmrs set and no msgs queued */
const GLretVal EHNOEVENTNB = (EH_FAIL-1); /* No msgs queued, no tmrs expired*/
					  /* and "noblock_flg" is TRUE	    */
const GLretVal EHNOTMREXP  = (EH_FAIL-2); /* After sleep/blocking msg rcv.  */
					  /* still didn't get an exp tmr!?  */
const GLretVal EHNOTINIT   = (EH_FAIL-3); /* Neither the timers nor the MSGH*/
					  /* queue has been initialized     */
const GLretVal EHTOOSMALL  = (EH_FAIL-4); /* Message size passed to getEvent*/
					  /* is less than "TMtmrExp" class sz*/
#endif
