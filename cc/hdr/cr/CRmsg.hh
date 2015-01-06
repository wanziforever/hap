#ifndef __CRMSG_H
#define __CRMSG_H

/*
**
**      File ID:        @(#): <MID6929 () - 08/17/02, 29.1.1.1>
**
**      File:                                   MID6929
**      Release:                                29.1.1.1
**      Date:                                   08/21/02
**      Time:                                   19:32:30
**      Newest applied delta:   08/17/02
**
** DESCRIPTION:
**      This file defines a USLI class that is used to generate and
**	send output messages (OM) to the spooler process (CSOP).
**
**	Examples (not necessarily realistic or advised):
**
**		CRmsg om("REPT ");
**		om.add("DGN ISLU %d", unit_num);
**              om.setMsgClass(CL_MAINT);
**		om.spool();
**
**		CRmsg om2(CL_MAINT, POA_CRIT);
**		om2.spool("text of a critical message");
**
**              CRmsg om3(CL_MAINT, POA_MAN);
**              om3.spool("REPT DGN ISLU %d", unit_num);
** 		
**	In case segmentation is expected, there are routines which should be 
**	to make the information across the segments readable. 
**
**		CRmsg om;
**
**		om.title("OP RTGTBL IN PROGRESS");
**		om.add("any data"
**		om.spool();	
** 		
**	In the above scienario, the title will get printed on every segment
**	which will make the data readable in both the segments. 
**
**	Also to avoid segmentation, avoidSegmentation() routine be called 
** 	before calling add() routine, e.g.
**
**		CRmsg om;
**		om.avoidSegmentation();
**		om.add("any thing");
**		om.spool();
**
**	The affect of avoiding segmentation is only for the above OM. If 
** 	any subsequent big OMs are produced, segmentation will occur unless
**	avoidSegmentation() routine is called every time a new OM is 
**	construced.
**
** OWNER:
**      Roger McKee
**
** NOTES:
**	The member function add_va_list MUST only be called with a
**	variable argument list (like printf).  It is not meant to
**	for general users of the class.  That is why it is 'private'.
*/

#include "sysent.h"
#include <stdarg.h>

#include "hdr/GLreturns.h"

#include "cc/hdr/msgh/MHnames.hh"
#include "cc/hdr/cr/CRmsgClass.hh"
#include "cc/hdr/cr/CRalarmLevel.hh"
#include "cc/hdr/cr/CRomBrevityCtl.hh"

/*#include "cc/hdr/cr/CRspoolMsg.H"*/
class CRspoolMsg;

class CRomInfo;

class CRmsg {

public:
	CRmsg(const Char *textValue, Bool buffering=YES);
	CRmsg(CROMCLASS =CL_DEBUG, CRALARMLVL =POA_INF, Bool buffering=YES);
	~CRmsg();
	CRmsg& add(const Char* format,...);
	CRmsg& add(int len, const char* str);
	void spool(const CRomInfo* cepOMinfo =0);
	void spool(const Char* format, ...);
	void title(const char* format,...);
	void setOMinfo(const CRomInfo* cepOMinfo =0);
	void setAlarmLevel(CRALARMLVL);
	void setMsgClass(CROMclass);
	void prmSpool(CRALARMLVL, const char* format,...);
	void miscSpool(short destDev, CRALARMLVL al, const Char *format,...);
	void setSegInterval(const int sleepTime);
	void avoidSegmentation();
	void genNewSeg(Bool oldTitle=YES);
	Void storeKey(const char* OMkey, int omType = CROMCRMSG);
	Void storeKey(int OMkey, int omType = CROMCRMSG);
	Void setOMkey(const char* thekey);
private:
	void add_va_list(const Char* format, va_list ap);
    private:
	CROMclass OMclass;
	CRALARMLVL alarmLevel;
	CRspoolMsg* msg;

	CRomBrevityCtl* omBrevityCtl;
	char theStoredKey[CROMKEYSZ];
};
#endif
