#ifndef __CRMTYPE_H
#define __CRMTYPE_H
/*
**      File ID:        @(#): <MID6930 () - 08/17/02, 29.1.1.1>
**
**	File:					MID6930
**	Release:				29.1.1.1
**	Date:					08/26/02
**	Time:					18:05:15
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**	This file defines the USLI-defined messages types.
**
** OWNER:
**      Roger McKee
**
** NOTES:
**
**
*/

#include "hdr/GLtypes.h"
#include "hdr/GLmsgs.h"
#include "cc/hdr/msgh/MHpriority.hh"

/* USLI message types */
enum CRmtype {
	CRdebugMsgTyp = CRMSGBASE,  /* CRDEBUG message */
	CRdbCmdMsgTyp = CRMSGBASE+1, /* TRACE Command message */
	CRshlAckTyp = CRMSGBASE+2, /* USL Parser acknowledgement */
	CRgenOMTyp = CRMSGBASE+3, /* CSOP Output Message */
	CRropMsgTyp = CRMSGBASE+4, /* ROP rmv/rst message */
	CRomdbTyp = CRMSGBASE+5, /* OMDB Output Message */
	CRalarmTyp = CRMSGBASE+6, /* ALARM Message Type */
	CRrpInitMsgTyp = CRMSGBASE+7, /* RPROC init Message */
	CRrprocMsgTyp = CRMSGBASE+8, /* RPROC keyboard input Message */
	CRsopMsgTyp = CRMSGBASE+9, /* CSOP to SOP output Message */
	CRsysDapReqTyp = CRMSGBASE+10, /* LMT to SYSTAT */
	CRsysDapRepTyp = CRMSGBASE+11, /* SYSTAT to LMT */
	CRindQryMsgTyp = CRMSGBASE+12, /* SYSTAT to APP */
	CRindStatusMsgTyp = CRMSGBASE+13, /* APP to SYSTAT */
	CRsysRetMsgTyp = CRMSGBASE+14, /* LMT to SYSTAT */
	CRsysAliveMsgTyp = CRMSGBASE+15, /* SYSTAT to LMT (broadcast) */
	CRrpHwFailMsgTyp = CRMSGBASE+16, /* RPROC to LMT */
	CRsopRegMsgTyp = CRMSGBASE+17, /* SOP to CSOP */
	CRcsopAliveMsgTyp = CRMSGBASE+18, /* CSOP to SOP (broadcast) */
	CRindRetireMsgTyp = CRMSGBASE+19, /* APP to SYSTAT */
	CRlocalLogMsgTyp = CRMSGBASE+20,  /* CSOP (standby) to CSOPHPR */
	CRsgenOMTyp = CRMSGBASE+21, /* STANDBY CSOP OUTPUT MESSAGE */
	CRsomdbTyp= CRMSGBASE+22, /* STANDBY OMDB OUTPUT MESSAGE */
	CRgetLocalLogMsgTyp = CRMSGBASE+40, /* CSOP to CSOP saying that theres 
	                                    locallogs to get */
	CRlistOffNormalMsgTyp = CRMSGBASE+42, /* APP to SYSTAT */
	CRoffNormalMsgAckTyp = CRMSGBASE+43, /* SYSTAT to APP */
	
	CRperfMonMsgTyp = CRMSGBASE+44, /* from PERFMON */
	CRperfMonMsgAckTyp = CRMSGBASE+45, /* to PERFMON */

	CRlogFileSwitchUpdTyp = CRMSGBASE + 50, /* APP TO CSOP */
	CRlogFileSwitchEventTyp = CRMSGBASE + 51, /* CSOP TO APP */
	CRmtype_BOGUS_SUNPRO
};

/* USLI specific message priorities */
enum {
	CRrprocMsgPri = MHoamPtyp+1,
	CRshlAckPri   = MHoamPtyp+2
};

#endif

