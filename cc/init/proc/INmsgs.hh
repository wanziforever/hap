#ifndef __INMSGS_H
#define __INMSGS_H
//
// DESCRIPTION:
// 	This file contains message class definitions for messages
//	which only INIT sends.
//
// NOTES:
//
#include <sys/types.h>
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/init/INrunLvl.hh"
#include "cc/hdr/init/INpDeath.hh"
#include "cc/hdr/init/INmissSan.hh"
#include "cc/hdr/init/INsetRstrt.hh"
#include "cc/hdr/init/INprcCreat.hh"
#include "cc/hdr/init/INprcUpd.hh"
#include "cc/hdr/init/INkillProc.hh"
//#include "cc/hdr/init/INinitSCN.hh"
#include "cc/hdr/init/INsetSoftChk.hh"
#include "cc/hdr/init/INswcc.hh"

#include "cc/hdr/init/INproctab.hh"

class INlrunLvlAck : public INsetRunLvlAck {
    public:
			INlrunLvlAck(Short num_procs, U_char run_lvl);
	GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
};

class INlrunLvlFail : public INsetRunLvlFail {
    public:
			INlrunLvlFail(GLretVal ret, U_char run_lvl);
	GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
};

class INlrunLvl : public INrunLvl {
    public:
			INlrunLvl(U_char rlvl);
	GLretVal	broadcast(MHqid fromQid, Long time);
};

class INlpDeath : public INpDeath {
    public:
			INlpDeath(const Char *pname, IN_PERMSTATE permflg,
				  Bool restart, Bool update, 
				  pid_t procid = -1,
				  SN_LVL sn_lvl = SN_LV0);
	GLretVal	broadcast(MHqid fromQid, Long time, Bool bAllNodes);
};

class INlmissedSan : public INmissedSan {
    public:
			INlmissedSan();
	GLretVal	send(const Char *name, MHqid fromQid, Long time);
};

class INlsetRstrtAck : public INsetRstrtAck {
    public:
			INlsetRstrtAck(const Char *name, Bool inh_flg);
	GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
};

class INlsetRstrtFail : public INsetRstrtFail {
    public:
			INlsetRstrtFail(const Char *name, GLretVal err_ret);
	GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
};

class INlsetSoftChkAck : public INsetSoftChkAck {
    public:
			INlsetSoftChkAck(const Char *name, Bool inh_flg);
	GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
};

class INlsetSoftChkFail : public INsetSoftChkFail {
    public:
			INlsetSoftChkFail(const Char *name, GLretVal err_ret);
	GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
};

class INlswccAck : public INswccAck {
    public:
			INlswccAck(void);
	GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
};

class INlswccFail : public INswccFail {
    public:
			INlswccFail(GLretVal err_ret);
	GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
};

class INlprocCreateAck : public INprocCreateAck {
    public:
			INlprocCreateAck(const Char *name, pid_t proc_id);
	GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
};

class INlprocCreateFail : public INprocCreateFail {
    public:
			INlprocCreateFail(const Char *name, GLretVal err_ret);
	GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
};

class INlprocUpdateAck : public INprocUpdateAck {
    public:
			INlprocUpdateAck(const Char *name);
	GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
};

class INlprocUpdateFail : public INprocUpdateFail {
    public:
			INlprocUpdateFail(const Char *name, GLretVal err_ret);
	GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
};

class INlkillProcAck : public INkillProcAck {
    public:
			INlkillProcAck(const Char *name);
	GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
};

class INlkillProcFail : public INkillProcFail {
    public:
			INlkillProcFail(GLretVal err_ret, const Char *name);
	GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
};

//class INlinitSCNAck : public INinitSCNAck {
//    public:
//			INlinitSCNAck(SN_LVL sn_lvl, const Char *name);
//	GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
//};
//
//class INlinitSCNFail : public INinitSCNFail {
//    public:
//			INlinitSCNFail(GLretVal err_ret, SN_LVL sn_lvl,
//						const Char *name);
//	GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
//};

class INcheckLead : public MHmsgBase {
	public:
	
	INcheckLead(){
		priType = MHoamPtyp;
		msgType = INcheckLeadTyp;
	};
};

#define	INmaxProcInfo	500	

class INcheckLeadAck : public MHmsgBase {
	public:
	
	INcheckLeadAck(){
		priType = MHoamPtyp;
		msgType = INcheckLeadAckTyp;
	};

	int		m_nProcs;
	INprocInfo	m_ProcInfo[INmaxProcInfo];
};

class INsetLead : public MHmsgBase {
	public:
	
	INsetLead(){
		priType = MHoamPtyp;
		msgType = INsetLeadTyp;
	};
	
};

class INoamLead : public MHmsgBase {
	public:
	
	INoamLead(){
		priType = MHoamPtyp;
		msgType = INoamLeadTyp;
	};
};

class INsetActiveVhost : public MHmsgBase {
	public:
	
	INsetActiveVhost(){
		priType = MHoamPtyp;
		msgType = INsetActiveVhostTyp;
	};
};

class INnodeReboot : public MHmsgBase {
	public:
	
	INnodeReboot(){
		priType = MHoamPtyp;
		msgType = INnodeRebootTyp;
	};
};

#endif
