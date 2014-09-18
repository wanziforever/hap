/*
**	File ID: 	@(#): <MID8484 () - 08/17/02, 29.1.1.1>
**
**	File:			MID8484
**	Release:		29.1.1.1
**	Date:			08/26/02
**	Time:			18:05:30
**	Newest applied delta:	08/17/02 04:31:46
**
** DESCRIPTION:
** 	This file contains the methods for message classes used
**	exclusively by INIT (the classes are defined in INmsgs.hh).
**
** FUNCTIONS:
**
** NOTES:
*/
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>

#ifdef EES
#include <strings.h>
#else
#include <string.h>
#endif

#include "hdr/GLtypes.h"
#include "hdr/GLmsgs.h"

#include "cc/hdr/msgh/MHinfoExt.hh"	/* MHmsgh class instance */
#include "cc/hdr/msgh/MHpriority.hh"

#include "cc/hdr/init/INmtype.hh"
#include "cc/hdr/init/INrunLvl.hh"

#include "cc/init/proc/INmsgs.hh"

INlrunLvlAck::INlrunLvlAck(Short nprocs, U_char rlvl) {
	num_procs = nprocs;
	run_lvl = rlvl;
	priType = MHoamPtyp;
	msgType = INsetRunLvlAckTyp;
}

GLretVal
INlrunLvlAck::send(MHqid toQid, MHqid fromQid, Long time) {
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INlrunLvlAck),
			   time);
}

/*
 *  Constructor and "send()" methods for "INlrunLvlFail"
 */
INlrunLvlFail::INlrunLvlFail(GLretVal err_ret, U_char rlvl) {
	priType = MHoamPtyp;
	run_lvl = rlvl;
	ret = err_ret;
	msgType = INsetRunLvlFailTyp;
}

GLretVal
INlrunLvlFail::send(MHqid toQid, MHqid fromQid, Long time) {
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INlrunLvlFail),
			   time);
}

/*
 *  Constructor and "broadcast()" methods for "INlrunLvl"
 */
INlrunLvl::INlrunLvl(U_char rlvl) {
	priType = MHoamPtyp;
	run_lvl = rlvl;
	msgType = INrunLvlTyp;
}

GLretVal
INlrunLvl::broadcast(MHqid fromQid, Long time) {
	srcQue = fromQid;
	return MHmsgh.broadcast(fromQid, (Char *) this, sizeof(INlrunLvl),
			   time);
}

/*
 * INlpDeath:
 */
INlpDeath::INlpDeath(const Char *pname, IN_PERMSTATE permflg, Bool restart,
		     Bool upd, pid_t procid, SN_LVL lvl) {
	// Must try both name and _name versions in case the process
	// was a global queue process. Also insure that the name
	// that is found is on local machine only so that we don't pass
	// qid for a another instance of a process on another machine
	char	altName[IN_NAMEMX];
	altName[0] = '_';
	altName[1] = 0;
	strncat(altName,pname, IN_NAMEMX - 2);
	strcpy(msgh_name, pname);
	if (MHmsgh.getMhqid((const Char *)msgh_name, msgh_qid, TRUE) != GLsuccess ||
		MHmsgh.Qid2Host(msgh_qid) != MHmsgh.getLocalHostIndex()) {
		if (MHmsgh.getMhqid((const Char *)altName, msgh_qid, TRUE) != GLsuccess ||
			MHmsgh.Qid2Host(msgh_qid) != MHmsgh.getLocalHostIndex()){
			msgh_qid = MHnullQ;
		}
	}
	perm_proc = (permflg == IN_PERMPROC) ? TRUE:FALSE;
	rstrt_flg = restart;
	upd_flg = upd;
	priType = MHoamPtyp;
	msgType = INpDeathTyp;
	pid = procid;
	sn_lvl = lvl;
}

GLretVal
INlpDeath::broadcast(MHqid fromQid, Long time, Bool bAllNodes) {
	srcQue = fromQid;
	return MHmsgh.broadcast(fromQid, (Char *) this, sizeof(INlpDeath),
			   time, bAllNodes);
}

/*
 *  INlmissedSan:
 */
INlmissedSan::INlmissedSan() {
	priType = MHsanityPtyp;
	msgType = INmissedSanTyp;
}

GLretVal
INlmissedSan::send(const Char *name, MHqid fromQid, Long time) {
	srcQue = fromQid;
	return MHmsgh.send(name, (Char *) this, sizeof(INlmissedSan),
			   time);
}

/*
 *  INlsetRstrtAck:
 */
INlsetRstrtAck::INlsetRstrtAck(const Char *name, Bool inh_flg) {
	strcpy(msgh_name, name);
	inh_restart = inh_flg;
	priType = MHoamPtyp;
	msgType = INsetRstrtAckTyp;
}

GLretVal
INlsetRstrtAck::send(MHqid toQid, MHqid fromQid, Long time) {
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INlsetRstrtAck),
			   time);
}

/*
 *  INlsetRstrtFail
 */
INlsetRstrtFail::INlsetRstrtFail(const Char *name, GLretVal err_ret) {
	strcpy(msgh_name, name);
	ret = err_ret;
	priType = MHoamPtyp;
	msgType = INsetRstrtFailTyp;
}

GLretVal
INlsetRstrtFail::send(MHqid toQid, MHqid fromQid, Long time) {
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INlsetRstrtFail),
			   time);
}

/*
 *  INlsetSoftChkAck:
 */
INlsetSoftChkAck::INlsetSoftChkAck(const Char *name, Bool inh_flg) {
	strcpy(msgh_name, name);
	inh_softchk = inh_flg;
	priType = MHoamPtyp;
	msgType = INsetSoftChkAckTyp;
}

GLretVal
INlsetSoftChkAck::send(MHqid toQid, MHqid fromQid, Long time) {
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INlsetSoftChkAck),
			   time);
}

/*
 *  INlsetSoftChkFail
 */
INlsetSoftChkFail::INlsetSoftChkFail(const Char *name, GLretVal err_ret) {
	strcpy(msgh_name, name);
	ret = err_ret;
	priType = MHoamPtyp;
	msgType = INsetSoftChkFailTyp;
}

GLretVal
INlsetSoftChkFail::send(MHqid toQid, MHqid fromQid, Long time) {
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INlsetSoftChkFail),
			   time);
}

/*
 *  INlswccAck
 */
INlswccAck::INlswccAck() {
	priType = MHoamPtyp;
	msgType = INswccAckTyp;
}

GLretVal
INlswccAck::send(MHqid toQid, MHqid fromQid, Long time) {
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INlswccAck),
			   time);
}

/*
 *  INlswccFail
 */
INlswccFail::INlswccFail(GLretVal err_ret) {
	retval = err_ret;
	priType = MHoamPtyp;
	msgType = INswccFailTyp;
}

GLretVal
INlswccFail::send(MHqid toQid, MHqid fromQid, Long time) {
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INlswccFail),
			   time);
}

/*
 *  INlprocCreateAck:
 */
INlprocCreateAck::INlprocCreateAck(const Char *name, pid_t proc_id) {
	strcpy(msgh_name, name);
	pid = proc_id;
	priType = MHoamPtyp;
	msgType = INprocCreateAckTyp;
}

GLretVal
INlprocCreateAck::send(MHqid toQid, MHqid fromQid, Long time) {
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INlprocCreateAck),
			   time);
}

/*
 *  INlprocCreateFail:
 */
INlprocCreateFail::INlprocCreateFail(const Char *name, GLretVal err_ret) {
	strcpy(msgh_name, name);
	ret = err_ret;
	priType = MHoamPtyp;
	msgType = INprocCreateFailTyp;
}

GLretVal
INlprocCreateFail::send(MHqid toQid, MHqid fromQid, Long time) {
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INlprocCreateFail),
			   time);
}

/*
 *  INlprocUpdateAck:
 */
INlprocUpdateAck::INlprocUpdateAck(const Char *name) {
	strcpy(msgh_name, name);
	priType = MHoamPtyp;
	msgType = INprocUpdateAckTyp;
}

GLretVal
INlprocUpdateAck::send(MHqid toQid, MHqid fromQid, Long time) {
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INlprocUpdateAck),
			   time);
}

/*
 *  INlprocUpdateFail:
 */
INlprocUpdateFail::INlprocUpdateFail(const Char *name, GLretVal err_ret) {
	strcpy(msgh_name, name);
	ret = err_ret;
	priType = MHoamPtyp;
	msgType = INprocUpdateFailTyp;
}

GLretVal
INlprocUpdateFail::send(MHqid toQid, MHqid fromQid, Long time) {
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INlprocUpdateFail),
			   time);
}

/*
 *  INlkillProcAck:
 */
INlkillProcAck::INlkillProcAck(const char *pname) {
	strcpy(msgh_name, pname);
	priType = MHoamPtyp;
	msgType = INkillProcAckTyp;
}

GLretVal
INlkillProcAck::send(MHqid toQid, MHqid fromQid, Long time) {
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INlkillProcAck),
			   time);
}

/*
 *  INlkillProcFail:
 */
INlkillProcFail::INlkillProcFail(GLretVal err_ret, const Char *name) {
	strcpy(msgh_name, name);
	ret = err_ret;
	priType = MHoamPtyp;
	msgType = INkillProcFailTyp;
}

GLretVal
INlkillProcFail::send(MHqid toQid, MHqid fromQid, Long time) {
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INlkillProcFail),
			   time);
}
///*
// *  INlinitSCNAck:
// */
//INlinitSCNAck::INlinitSCNAck(SN_LVL req_sn_lvl, const char *pname) {
//	sn_lvl = req_sn_lvl;
//	if ((req_sn_lvl == SN_LV0) || (req_sn_lvl == SN_LV1)) {
//		strcpy(msgh_name, pname);
//	}
//	else {
//		msgh_name[0] = 0;
//	}
//	priType = MHoamPtyp;
//	msgType = INinitSCNAckTyp;
//}
//
//GLretVal
//INlinitSCNAck::send(MHqid toQid, MHqid fromQid, Long time) {
//	srcQue = fromQid;
//	return MHmsgh.send(toQid, (Char *) this, sizeof(INlinitSCNAck),
//			   time);
//}
//
///*
// *  INlinitSCNFail:
// */
//INlinitSCNFail::INlinitSCNFail(GLretVal err_ret, SN_LVL req_sn_lvl, const Char *name) {
//	ret = err_ret;
//	sn_lvl = req_sn_lvl;
//	if ((req_sn_lvl == SN_LV0) || (req_sn_lvl == SN_LV1)) {
//		Short len = strlen(name);
//		if (len >= IN_NAMEMX) {
//			strncpy(msgh_name, name, (IN_NAMEMX-1));
//			msgh_name[IN_NAMEMX-1] = (Char)0;
//		}
//		else {
//			strcpy(msgh_name, name);
//		}
//	}
//	else {
//		msgh_name[0] = 0;
//	}
//
//	ret = err_ret;
//	priType = MHoamPtyp;
//	msgType = INinitSCNFailTyp;
//}
//
//GLretVal
//INlinitSCNFail::send(MHqid toQid, MHqid fromQid, Long time) {
//	srcQue = fromQid;
//	return MHmsgh.send(toQid, (Char *) this, sizeof(INlinitSCNFail),
//			   time);
//}
//
