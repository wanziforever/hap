#ifndef __CRRCVOMDB_H
#define __CRRCVOMDB_H
/*
**      File ID:        @(#): <MID13762 () - 02/19/00, 23.1.1.1>
**
**	File:					MID13762
**	Release:				23.1.1.1
**	Date:					05/13/00
**	Time:					13:26:48
**	Newest applied delta:	02/19/00
**
** DESCRIPTION:
**      This file defines a USLI class that is used to generate and
**	send output messages (OM) to the spooler process (CSOP)
**      using the Output Message Data Base (OMDB)
**
**	Examples (not necessarily realistic):
**
**              CROMDBKEY(USL0, "/USL000");
**		CRomdbMsg om;
**		om.add(unit_num).add("some explanation text");
**		om.spool(USL0);
**
**              CROMDBKEY(USL5, "/USL005");
**		CRomdbMsg om2;
**              om2.setAlarmLevel(POA_CRIT);
**		om2.add("variable text of a critical message");
**              om2.spool(USL5);
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

#include "cc/hdr/msgh/MHnames.H"
#include "cc/hdr/cr/CRalarmLevel.H"
#include "cc/hdr/cr/CRx733.H"


class CRomdbMsg;

class CRrcvOmdbMsg
{
    public:
	CRrcvOmdbMsg(CRomdbMsg* inmsg);
	~CRrcvOmdbMsg();
	void dump();
	void fillInFormattedMsg();
	void format(const char* line1prefix, const char* format,
		    const char* msgclass,
		    const char* lineNprefix =CRDEFALMSTR);
	const char* getMsgClass() const;
	const char* getAlarmStr() const;
	const char* getMsgName() const;
	const char* getFormattedMsg() const;
	// getFormattedTextMsg() this is used to get the TEXT of the OM 
	// after the %s etc have been fill out
	const char* getFormattedTextMsg() const;
	const char* getSenderName() const;
	const char* getUSLIname() const;
	CRALARMLVL  getAlarmLevel() const;

	//x733 stuff
	const char*  getProbableCauseValue();
        const char*  getAlarmTypeValue();
	CRX733AlarmProbableCause getProbableCause();
        CRX733AlarmType getAlarmType();
        const char* getAlarmObjectName();
        const char* getSpecificProblem();
        const char* getAdditionalText();
	Void x733Fields(const char* pCause, const char* alarmType,
			const char* objectName , const char* specProb,
			const char* addText);

	enum { CRmaxOutBufSz = 5000 };

	const char* getSenderMachine();

	
    private:
	void init();

	int doprint(const char* format, char* bufstart, const char* bufend,
		    const char* line1prefix, const char* lineNprefix);
	int noformatPrint(char* bufstart, const char* bufend,
		    const char* line1prefix, const char* lineNprefix);
	void overFlowError(int bufsize);
		 
    private:
	CRomdbMsg* msg;
	static char format_buf[CRmaxOutBufSz+1];
	static char formatedText[CRmaxOutBufSz+1]; //text of OM minus hdr and ftr
	static const int maxLineLen;
	static char x733Buf[200];
};

#endif
