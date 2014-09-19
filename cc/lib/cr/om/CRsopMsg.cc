/*
**      File ID:        @(#): <MID18886 () - 09/06/02, 29.1.1.1>
**
**	File:					MID18886
**	Release:				29.1.1.1
**	Date:					09/12/02
**	Time:					10:39:22
**	Newest applied delta:	09/06/02
**
** DESCRIPTION:
**      This file defines the CRsopMsg class which represents the MSGH
**      message that contains a formatted Ouput Message (OM)
**      sent by CSOP to an individual SOP.
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**      It is assumed that the formatted OM will fit in one MSGH message.
**      There is no provision for OMs that do not fit.
*/
#include <string.h>
#include "cc/hdr/cr/CRsopMsg.H"
#include "cc/hdr/cr/CRmtype.H"
#include "cc/cr/hdr/CRshtrace.H"
#include "cc/hdr/cr/CRlocalLogMsg.H"
#include "cc/hdr/msgh/MHinfoExt.H"

short CRsopMsg::nextMsgNumber = 1;

CRsopMsg::CRsopMsg()
{
	priType = MHoamPtyp;
	msgType = CRsopMsgTyp;
	msghead.blockNumber = 0;
	msghead.lastBlock = YES;
	alarmLevel(POA_INF);
	msghead.x733alarms.init();

}

void
CRsopMsg::alarmLevel(CRALARMLVL value)
{
	msghead.alarmlvl = value;
}

CRALARMLVL
CRsopMsg::alarmLevel() const
{
	return msghead.alarmlvl;
}

GLretVal
CRsopMsg::send(const char* msghName, const char* text, int text_len)
{
	GLretVal retval = GLsuccess;

	msghead.msgNumber = nextMsgNumber++;

	const int maxBlock = sizeof(msgText) - 1;
	const char* textToMove = text;

	for (int lenToMove = text_len; lenToMove > 0;
	     lenToMove -= maxBlock, textToMove += maxBlock)
	{
		int lenToSend;
		if (lenToMove > maxBlock)
		{
			msghead.lastBlock = NO;
			lenToSend = maxBlock;
		}
		else
		{
			msghead.lastBlock = YES;
			lenToSend = lenToMove;
		}
		strncpy(msgText, textToMove, lenToSend);
		msgText[lenToSend] = '\0';
		if ((retval = send(msghName, lenToSend + 1)) != GLsuccess)
       break;

		msghead.blockNumber++;
	}

	alarmLevel(POA_INF);
	return retval;
}

/* send with time of 500ms so SOP does not get overwhelmed */
const Long CRsopMsgSendTime = 500L;

GLretVal
CRsopMsg::send(const char* msghName, int textLenToSend)
{
	Short msgsz = MHmsgBaseSz + sizeof(msghead) + textLenToSend;
	return MHmsgBase::send(msghName, MHnullQ, msgsz, CRsopMsgSendTime);
}

void
CRsopMsg::init()
{
	priType = MHoamPtyp;
	msgType = CRsopMsgTyp;
	msghead.blockNumber = 0;
	msghead.msgNumber = nextMsgNumber++;
	msghead.lastBlock = YES;
	msgText[0] = '\0';
	alarmLevel(POA_INF);
	msghead.x733alarms.init();

}

//
//	This message type is used for ACTIVE/ACTIVE. FTMON
//	sends me(CSOP) one of these when a processer comesup
//	ie. if CC1 is the ACTIVE processer whem FTMON comes up
//	I'll get this message. Then CSOP cp's the locallogs form
//	the active provess and dumps them in the OMlog
//

CRlocalLogMsg::CRlocalLogMsg()
{
	priType = MHoamPtyp;
	msgType = CRlocalLogMsgTyp;
	msgSz = MHmsgBaseSz;
}

CRlocalLogMsg::~CRlocalLogMsg()
{
}

GLretVal
CRlocalLogMsg::send(MHqid fromQid)
{

	MHqid toQid;
  MHmsgh.getMhqid("CSOP",toQid);

	return MHmsgBase::send(toQid, fromQid, (Long) sizeof(*this), 0);
}

/*
**	This will  send CSOP a message saying to get the 
**	files in /snlog/locallogs. This message is being
**	sent by CSOP to CSOP
*/
CRgetLocalLogMsg::CRgetLocalLogMsg()
{
	msgType = CRgetLocalLogMsgTyp;
}

GLretVal
CRgetLocalLogMsg::send()
{

	MHqid toQid;
	MHqid fromQid;
  MHmsgh.getMhqid("CSOP",toQid);
  fromQid=toQid;

	return MHmsgBase::send(toQid, fromQid, (Long) sizeof(*this), 0);
}



/* setting the x733 alarm stuff */


void
CRsopMsg::setAlarmObjectName(char* objectName )
{
	msghead.x733alarms.setAlarmObjectName(objectName);
}

void
CRsopMsg::setAlarmType(CRX733AlarmType alarmType)
{
	msghead.x733alarms.setAlarmType(alarmType);
}

void
CRsopMsg::setProbableCause(CRX733AlarmProbableCause pCause)
{
	msghead.x733alarms.setProbableCause(pCause);
}

void
CRsopMsg::setSpecificProblem(char* specProb)
{
	msghead.x733alarms.setSpecificProblem(specProb);
}

void
CRsopMsg::setAdditionalText(char* addText)
{
	msghead.x733alarms.setAdditionalText(addText);
}
