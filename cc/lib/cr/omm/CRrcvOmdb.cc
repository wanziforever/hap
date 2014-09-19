/*
**      File ID:        @(#): <MID13761 () - 02/19/00, 23.1.1.1>
**
**	File:					MID13761
**	Release:				23.1.1.1
**	Date:					05/13/00
**	Time:					13:26:47
**	Newest applied delta:	02/19/00
**
** DESCRIPTION:
**      This file defines a USLI class that is used to process CRomdbMsg
**      objects received by the CSOP the output message spooler process (CSOP).
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

#include "cc/hdr/cr/CRdebugMsg.H"
#include "CRrcvOmdb.H"
#include "cc/hdr/cr/CRomdbMsg.H"
#include "CRomdb.H"
#include "cc/hdr/cr/CRloadName.H"
#include "cc/hdr/cr/CRofficeNm.H"
#include "cc/hdr/cr/CRspoolMsg.H"
#include "CRcsopMsg.H"
#include "CRcsopOMs.H"
#include "CRomdbEnt.H"
#include "cc/cr/hdr/CRomHdrFtr.H"
#include "CRcsopTrace.H"
#include "cc/cr/hdr/CRomClEnt.H"
#include "cc/hdr/cr/CRassertIds.H"

char CRrcvOmdbMsg::format_buf[CRrcvOmdbMsg::CRmaxOutBufSz+1];
char CRrcvOmdbMsg::formatedText[CRrcvOmdbMsg::CRmaxOutBufSz+1];
char CRrcvOmdbMsg::x733Buf[200];

CRrcvOmdbMsg::CRrcvOmdbMsg(CRomdbMsg* inmsg) : msg(inmsg)
{
	format_buf[0] = '\0';
	formatedText[0] = '\0';
}

CRrcvOmdbMsg::~CRrcvOmdbMsg() 
{
}

extern CRomdb omdb;

/*
** This is being called in CRomdb.C to get the OM text with the
** %s and %d filled out
*/
void
CRrcvOmdbMsg::fillInFormattedMsg()
{
	String omdb_key = msg->getEntryName();
	
	const CRomdbEntry* omdbEntry = omdb.getEntry(omdb_key);
	if (omdbEntry)
	{
		const char* formatStr = omdbEntry->getFormat();
		int numchars = doprint(formatStr, format_buf,
                           format_buf + CRmaxOutBufSz,
                           getAlarmStr(), CRDEFALMSTR);

		/* need to remove the first 3 spaces */
		char* bp2 = &format_buf[3];
		strlcpy(formatedText,bp2,sizeof(formatedText));
	}
}

void
CRrcvOmdbMsg::dump()
{
	String omdb_key = msg->getEntryName();
/*	fprintf(stderr, "omdb_key=%s\n", (const char*) omdb_key);*/
	
	const CRomdbEntry* omdbEntry = omdb.getEntry(omdb_key);
	if (omdbEntry)
	{
		const char* formatStr = omdbEntry->getFormat();
		fprintf(stderr, "formatStr=%s", formatStr);
		int numchars = doprint(formatStr, format_buf,
                           format_buf + CRmaxOutBufSz,
                           getAlarmStr(), CRDEFALMSTR);
		fprintf(stderr, "numchars formatted=%d\n", numchars);
		fprintf(stderr, "formatted buffer=|%s|\n", format_buf);

		if (!msg->allVarsFormatted())
		{
			fprintf(stderr, "CRomdbMsg has extra variables for OMDB entry %s\n",
              (const char*) msg->getEntryName());
		}
		/* need to remove the first 3 spaces */
		char* bp2 = &format_buf[3];
		strncpy(formatedText,bp2,sizeof(formatedText));
	}
}

const char*
CRrcvOmdbMsg::getSenderName() const
{
	return msg->getSenderName();
}

const char*
CRrcvOmdbMsg::getUSLIname() const
{
	return msg->getUSLIname();
}

const char*
CRrcvOmdbMsg::getMsgClass() const 
{
	return msg->getMsgClass();
}

const char*
CRrcvOmdbMsg::getMsgName() const 
{
	return msg->getEntryName();
}

void
CRrcvOmdbMsg::format(const char* line1prefix, const char* format,
                     const char* mclass, const char* lineNprefix) 
{

	char*    time_buf;
	char*    msgclass;
	char*    omdbkey;
	char*    segstr;
	int      seqnum;
	char	 hdr_str[200];
	char	 ftr_str[200];

	time_buf = (char *)msg->getTime();
	msgclass = (char *)mclass;
	omdbkey = (char *)msg->getEntryName();

	CRomClassEntry* messageclass;
	messageclass = omdb.getClassEntry(mclass);

	/*
	** if the cr_msgcls table is not been corruptted then 
	** we will get a reqular sequence number to this OM 
	** otherwise, we give 999999 (the special seq number)
	** to this OM to indicate that there's something wrong
	** about the cr_msgcls table in ORACLE Data Base
	*/
	
	if (messageclass != NULL)
     seqnum = messageclass->genNextSeqnum();
	else
     seqnum = CRDEFSEQNUM;

	segstr = (char *)CRDEFSEGMENT;

	/*
	** pass in segstr as CRDEFSEGMENT=NULL, no segmentation 
	** in CRomdbMsg class
	*/

#ifndef __mips
	strcpy(hdr_str , CRgenOMhdr(time_buf, msgclass, omdbkey, 
                              seqnum, segstr, msg->h.senderMachine, 
                              msg->h.senderState));

#else
	strcpy(hdr_str , CRgenOMhdr(time_buf, msgclass, omdbkey, seqnum, 
                              segstr));
#endif

	strcpy(format_buf, hdr_str);

	char* bp = format_buf + strlen(format_buf);
	
	if (format)
	{
		if (doprint(format, bp, format_buf+CRmaxOutBufSz,
                line1prefix, lineNprefix))
		{

      /*
      ** Attach the OM footer also check to make sure
      ** that the whole message will not cause overflow.
      */

//
//Append the x733 stuff here
//
			strcat(format_buf, x733Buf);
 
//
//End x733 stuff
//

			strcpy(ftr_str , CRgenOMftr(seqnum));
			if (strlen(format_buf) + strlen(ftr_str) <= CRmaxOutBufSz)
			{
				strcat(format_buf, ftr_str);
			}
			else 
         overFlowError(strlen(format_buf)+strlen(ftr_str));

			if (msg->allVarsFormatted())
         return;

			CRCFTASSERT(CRomTooManyVarsId,
                  (CRomTooManyVarsFmt, 
                   (const char*) msg->getEntryName(),
                   format,
                   getSenderName()));
		}
		CRcsopMsg om;
		om.spool(&CRfmtErrorOM, (const char*) msg->getEntryName(),
             getSenderName());
	}

	noformatPrint(bp, format_buf+CRmaxOutBufSz,
                line1prefix, lineNprefix);

	/*
	** Attach the OM footer also check to make sure
	** that the whole message will not cause overflow.
	*/
//
//Append the x733 stuff here
//
	strcat(format_buf, x733Buf);
//
//End x733 stuff
//

	strcpy(ftr_str , CRgenOMftr(seqnum));
	if (strlen(format_buf) + strlen(ftr_str) <= CRmaxOutBufSz)
	{
		strcat(format_buf, ftr_str);
	}
	else 
     overFlowError(strlen(format_buf)+strlen(ftr_str));
}

const char*
CRrcvOmdbMsg::getAlarmStr() const 
{
	return msg->getAlarmStr();
}

CRALARMLVL
CRrcvOmdbMsg::getAlarmLevel() const 
{
	return msg->getAlarmLevel();
}

int
CRrcvOmdbMsg::noformatPrint(char* bufstart, const char* bufend,
                            const char* line1prefix,
                            const char* lineNprefix)
{
	char* bp = bufstart;
	strcpy(bp, line1prefix);
	bp += strlen(line1prefix);
	sprintf(bp, CRnoFormatMsg, getMsgName());
	bp += strlen(bp);

	return msg->printVars(bp, bufend, lineNprefix);
}

const char*
CRrcvOmdbMsg::getFormattedMsg() const
{
	return format_buf;
}

const char*
CRrcvOmdbMsg::getFormattedTextMsg() const
{
	return formatedText;
}

/* getting the x733 alarm stuff */
 
const char*
CRrcvOmdbMsg::getAlarmObjectName()
{
  return msg->getAlarmObjectName();
}

const char*
CRrcvOmdbMsg::getAlarmTypeValue()
{
  return msg->getAlarmTypeValue();
}
 
const char*
CRrcvOmdbMsg::getProbableCauseValue()
{
  return msg->getProbableCauseValue();
}
 
CRX733AlarmType
CRrcvOmdbMsg::getAlarmType()
{
  return msg->getAlarmType();
}
 
CRX733AlarmProbableCause
CRrcvOmdbMsg::getProbableCause()
{
  return msg->getProbableCause();
}
 
const char*
CRrcvOmdbMsg::getSpecificProblem()
{
  return msg->getSpecificProblem();
}
 
const char*
CRrcvOmdbMsg::getAdditionalText()
{
  return msg->getAdditionalText();
 
}

Void
CRrcvOmdbMsg::x733Fields( const char* pCause, const char* alarmType, const char* objectName, const char* specProb, const char* addText)
{
  if( strcmp(alarmType,"alarmTypeUndefined") != NULL)
	{
		if(omdb.x733reptTblPtr->getX733Report() == TRUE)
		{
      snprintf(x733Buf, sizeof(x733Buf) - 1,
               "\n::%s::%s::%s::%s::%s::",
               (const char*) alarmType,
               (const char*) pCause,
               (const char*) objectName,
               (const char*) specProb,
               (const char*) addText);
		}
		else
		{
      x733Buf[0]=NULL;
		}
	}
	else
	{
		x733Buf[0]=NULL;
	}
}
	

const char*
CRrcvOmdbMsg::getSenderMachine()
{
  return msg->h.senderMachine;
}
