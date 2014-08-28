#ifndef __INMTYPE_H
#define __INMTYPE_H
/*
**	File ID:	@(#): <MID8486 () - 08/17/02, 29.1.1.1>
**
**	File:			MID8486
**	Release:		29.1.1.1
**	Date:			08/21/02
**	Time:			19:32:59
**	Newest applied delta:	08/17/02
**
**	Description:
**		Definition of all INIT-related messages types.
**
** NOTES:
*/


#include "hdr/GLtypes.h"
#include "hdr/GLmsgs.h"

/*
 * Messages broadcast by INIT to all other CC processes:
 */
const Short INrunLvlTyp = (INMSGBASE);  /* system run level change notice */
const Short INpDeathTyp = (INMSGBASE+1); /* Process death event notice */
const Short INfailoverTyp = (INMSGBASE+2); /* failover event notice */

/*
 * Messages sent from INIT to other processes on the CC:
 */
const Short INmissedSanTyp   = (INMSGBASE + 11); /* Missed sanity tmr notice*/
const Short INprocCreateAckTyp  = (INMSGBASE + 12); /*Succ. ACK INprocCreate*/
const Short INprocCreateFailTyp = (INMSGBASE + 13); /*Fail ACK- INprocCreate*/
const Short INsetRunLvlAckTyp = (INMSGBASE + 14);/* Succ. ACK to INsetRunLvl */
const Short INsetRunLvlFailTyp = (INMSGBASE + 15);/* Fail ACK to INsetRunLvl */
const Short INinitSCNAckTyp  = (INMSGBASE + 16); /* Succ. ACK to INinitSCN */
const Short INinitSCNFailTyp = (INMSGBASE + 17); /* Fail ACK to INinitSCN */
const Short INkillProcAckTyp = (INMSGBASE + 18); /* Succ. ACK to INkillProc */
const Short INkillProcFailTyp = (INMSGBASE + 19); /* Fail ACK to INkillProc */
const Short INsetRstrtAckTyp = (INMSGBASE + 20); /* Succ. ACK to INsetRstrt */
const Short INsetRstrtFailTyp = (INMSGBASE + 21); /* Fail ACK to INsetRstrt */
const Short INinitProcAckTyp = (INMSGBASE + 22); /* Succ. ACK to INinitProc */
const Short INinitProcAckFail = (INMSGBASE + 23); /* Fail ACK to INinitProc */
const Short INsetSoftChkAckTyp  = (INMSGBASE + 24);/* Succ. ACK to INsetSoftChk	*/
const Short INsetSoftChkFailTyp  = (INMSGBASE + 25); /* Fail ACK to INsetSoftChk */
const Short INprocUpdateAckTyp  = (INMSGBASE + 26); /*Succ. ACK INprocUpdate*/
const Short INprocUpdateFailTyp = (INMSGBASE + 27); /*Fail ACK- INprocUpdate*/
const Short INswccAckTyp	= (INMSGBASE + 28); /* Succ ACK	- INswcc*/
const Short INswccFailTyp	= (INMSGBASE + 29); /* Fail ACK - INswcc*/
const Short INinitdataRespTyp = ( INMSGBASE + 30 );/* Resp. from INIT */

/*
 * Messages sent to INIT from CC processes:
 */
const Short INprocCreateTyp   = (INMSGBASE + 31); /* Req to create TEMP proc*/
const Short INsetRunLvlTyp = (INMSGBASE + 32); /* Req. to set current runlvl*/
const Short INinitSCNTyp   = (INMSGBASE + 33); /* Request an sys.-wide init */
const Short INinitProcTyp  = (INMSGBASE + 34); /* Req. re-init of a specific*/
					       /* process */
const Short INsetRstrtTyp  = (INMSGBASE + 35); /* Req. to inhibit or allow   */
					       /* the restart or re-init of a*/
					       /* proc. if it dies           */
const Short INkillProcTyp  = (INMSGBASE + 36); /* Req. that INIT kill a perm.*/
					       /* proc. (w/SIGKILL) after a  */
					       /* specified interval.	     */
const Short INprocUpdateTyp   = (INMSGBASE + 37); /* Req to update info for a TEMP proc*/
const Short INsetSoftChkTyp   = (INMSGBASE +38); /* Req to change software checks */
const Short INswccTyp     = (INMSGBASE +39); 	  /* Req to switch CCs 		*/
const Short INcheckLeadTyp = (INMSGBASE + 40);    /* Check lead status 		*/
const Short INcheckLeadAckTyp = (INMSGBASE + 41); /* Lead status response	*/
const Short INinitializeTyp = (INMSGBASE + 42);	
const Short INinitializeAckTyp = (INMSGBASE + 43);
const Short INinitdataReqTyp = ( INMSGBASE + 44 );/* Req. to INIT for porc. data */
const Short INwdSendTyp = (INMSGBASE + 45);	/* Message to the watchdog		*/
const Short INwdRcvTyp = (INMSGBASE + 46);	/* Message received from watchdog	*/
const Short INsetLeadTyp = (INMSGBASE + 47);    /* INIT message controlling current lead */
const Short INoamInitializeTyp = (INMSGBASE + 48);/* oam lead init */

const Short INoamLeadTyp = (INMSGBASE + 49); 	/* Used to negotiate OA&M Lead	*/
const Short INnodeRebootTyp = (INMSGBASE + 50); /* Ask INITMON to reboot a Tomix blade 	*/
const Short INsetActiveVhostTyp = (INMSGBASE + 51); /* set the active vhost		*/
const Short INvhostInitializeTyp = (INMSGBASE + 52);/* active vhost init 		*/

#endif
