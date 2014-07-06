#ifndef _GLPORTID_H
#define _GLPORTID_H

// Description:
//  This file defines UDP ports used.
//  Also defined here are the ports used for platform
//  inter-process communication.

#define SCUDPBASE 60100
#define SCSFSND (SCUDPBASE+0) // SFSCH to send to SM
#define SCSFRCV (SCUDPBASE+1) // SF PROC to get from SM
#define SCCSCFSND (SCUDPBASE+2) // CSCF to send to SCU
#define SCCSCFRCV (SCUDPBASE+3) // CSCF RPROC to get from SCU
#define SCUPDMAXOFFSET SCCSCFRCV

#define UDPMISCBASE (SCUDPBASE+100)
#define MHUDP_PORTID (UDPMISCBASE+100)

// For lack of a better place to define the ports used for platform
// interprocess communication, we put them here. An example, of platform
// processes that communicate via sockets is the SMI<=>SMI_MAN
// communication
#define IPCMISCBASE (UDPMISCBASE+200)

// define SMI_MAN_PORT (IPCMISCBASE+1)
#define FTIPM_IPC_PORTID (IPCMISCBASE+2) // CCM IPM send to FTOAM
#define FTDMON_IPC_PORTID (IPCMISCBASE+3) // DMON server on each blade

#endif
