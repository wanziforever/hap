#ifndef _MHMTYPES_H
#define _MHMTYPES_H
/*
** File ID:	@(#): <MID51165 () - 05/12/03, 9.1.1.1>
**
** File:		MID51165
** Release:		9.1.1.1
** Date:		06/18/03
** Time:		19:19:16
** Newest applied delta:05/12/03
*/
#include "hdr/GLmsgs.hh"
const U_short MHregNameTyp = MHMSGBASE;         // register name msg 
const U_short MHregAckTyp = MHMSGBASE+1;        // register name ACK msg
const U_short MHrmNameTyp = MHMSGBASE+2;        // remove name msg
const U_short MHlrgMsgSig = MHMSGBASE+3;        // Large Message Signal
const U_short MHnameMapTyp = MHMSGBASE+4;       // name map message
const U_short MHenetStateTyp = MHMSGBASE+5;     // status of enet message
const U_short MHconnTyp = MHMSGBASE+6;          // status of enet message
const U_short MHhostDelTyp = MHMSGBASE+7;       // status of enet message
const U_short MHrejectTyp =  MHMSGBASE+8;       // reject message
const U_short MHregGdTyp = MHMSGBASE+9;         // register GDO 
const U_short MHregGdAckTyp = MHMSGBASE+10;     // register GDO ack
const U_short MHlrgMsgTyp = MHMSGBASE+11;       // large message
const U_short MHgdAudTyp = MHMSGBASE+12;        // global data audit 
const U_short MHgdAudReqTyp = MHMSGBASE+13;     // global data audit request
const U_short MHgqRcvTyp = MHMSGBASE+14;        // notification of global queue
const U_short MHgqRcvAckTyp = MHMSGBASE+15;     // notification of global queue ack
const U_short MHgdSyncReqTyp = MHMSGBASE+16;    // GDO synchronization req 
const U_short MHgdSyncDataTyp = MHMSGBASE+17;   // GDO synchronization data
const U_short MHgQMapTyp = MHMSGBASE+18;        // Global queue map message
const U_short MHgqInitTyp = MHMSGBASE+19;       // Global queue initialization message
const U_short MHgqInitAckTyp = MHMSGBASE+20;    // Global queue initialization ack
const U_short MHbcastTyp = MHMSGBASE+21;        // Broadcast message
const U_short MHgQSetTyp = MHMSGBASE+22;        // Global queue status change
const U_short MHgdUpdTyp = MHMSGBASE+23;        // Global data object update 
const U_short MHfailoverTyp = MHMSGBASE+24;     // Indicates failover occured
const U_short MHrmGdTyp = MHMSGBASE+25;         // Remove GDO 
const U_short MHrmGdAckTyp = MHMSGBASE+26;      // Remove GDO ack
const U_short MHaudGdTyp = MHMSGBASE+27;        // Audit GDO 
const U_short MHaudGdAckTyp = MHMSGBASE+28;     // Audit GDO ack
const U_short MHdQMapTyp = MHMSGBASE+29;        // Distributive queue map
const U_short MHdQSetTyp = MHMSGBASE+30;        // Distributive queue set
const U_short MHnetStChgTyp = MHMSGBASE+31;	// Network state change
const U_short MHcargoTyp = MHMSGBASE+32;	// MSGH cargo message
const U_short MHraceNotifyTyp = MHMSGBASE+33;	// MSGH GDO diff notify message
const U_short MHnodeStChgTyp = MHMSGBASE+34;	// MSGH GDO diff notify message
const U_short MHregMapTyp = MHMSGBASE+35;	// Registration map

#endif
