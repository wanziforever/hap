#ifndef __CRPRMMSG_H
#define __CRPRMMSG_H

// DESCRIPTION:
//     This file contains a definition of a function used by INIT
// library to set the function pointer to a function to peg the
// number of CRERRORS and CRASSERTS generated on the system and
// also the definition for CR_PRM macro used by INIT library to 
// log the initialization message to PRMlog file in /sn/log
// directory.

// OWNER:
//     Yash Gupta
//
// NOTES:
//

#include <stdio.h>
#include "cc/hdr/cr/CRmsg.hh"

const short CRerrorFlag = 1;
const short CRassertFlag = 2;

const short CRprmLogOnly = 1;
const short CRprmLogAndCnsl = 2;

extern vod CRsetAssertFn(void (*fnPtr)(short));

// this macro used by init code to long messages into log file
// when CSOP process is not up and running
extern CRmsg* CRprmPtr();

#define CR_PRM                                  \
  CRprmPtr()->prmSpool

extern CRmsg* CRmiscPrmPtr();
#define CR_MISCPRM                              \
  CRmiscPrmPtr()->miscSpool

#endif
