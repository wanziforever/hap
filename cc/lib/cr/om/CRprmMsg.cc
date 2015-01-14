/*
**
**      File ID:        @(#): <MID27342 () - 08/17/02, 26.1.1.1>
**
**	File:					MID27342
**	Release:				26.1.1.1
**	Date:					08/21/02
**	Time:					19:39:55
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**	This contains functions of CRmsg class that are called for CR_PRM 
**	macro only 
**	
**
** OWNER:
**      Yash Gupta
**
** NOTES:
**
*/

#include <String.h>
#include <stdlib.h>
#include "cc/hdr/cr/CRmsg.hh"
#include "cc/hdr/cr/CRmsgInt.hh"
#include "cc/hdr/cr/CRdebugMsg.hh"
#include "cc/hdr/cr/CRomInfo.hh"
#include "cc/hdr/cr/CRspoolMsg.hh"
//#include "cc/cr/hdr/CRshtrace.hh"

//#include "cc/cr/hdr/CRomDest.hh"


/* this function is only used by the CR_PRM macro only */
void
CRmsg::prmSpool(CRALARMLVL al, const Char *format,...)
{

	mutex_lock(&CRlockVarible);
	va_list ap;
	va_start(ap, format);

	add_va_list(format, ap);
	va_end(ap);

	Bool isIntLog = FALSE;
	if(al > POA_MAXALARM)
	{
		al=al - POA_MAXALARM;
		isIntLog = TRUE;
	}

	msg->setAlarmLevel(al);
	//msg->clearX733();
  //msg->setAlarmType(alarmTypeUndefined);

	if(isIntLog)
	{
		// this will spool both incident and PRM
    msg->intSpool();
	}
	else
	{
		// this is PRM only
		msg->prmSpool();
	}

	msg->setAlarmLevel(alarmLevel);
	msg->setClass(OMclass);
	msg->setState();
	mutex_unlock(&CRlockVarible);
}

/* this function is only used by the CR_PRM macro only */
void
CRmsg::miscSpool(short destDev, CRALARMLVL al, const Char *format,...)
{
	va_list ap;
	va_start(ap, format);

	add_va_list(format, ap);
	va_end(ap);

	msg->setAlarmLevel(al);
	//msg->clearX733();
  //msg->setAlarmType(alarmTypeUndefined);
	msg->miscSpool(destDev);
	msg->setAlarmLevel(alarmLevel);
	msg->setClass(OMclass);
	msg->setState();
}

/* this function is only used by the CR_X733PRM macro only */
//void
//CRmsg::x733prmSpool(CRALARMLVL al, 
//                    const Char* objectName,
//                    CRX733AlarmType alarmType,
//                    CRX733AlarmProbableCause probableCause, 
//                    const Char* specificProblem,
//                    const Char* additionalText,
//                    const Char* format,...)
//{
//	mutex_lock(&CRlockVarible);
//	va_list ap;
//	va_start(ap, format);
//
//	add_va_list(format, ap);
//	va_end(ap);
//
//	Bool isIntLog = FALSE;
//	if(al > POA_MAXALARM)
//	{
//		al=al - POA_MAXALARM;
//		isIntLog = TRUE;
//	}
//
//	msg->setAlarmLevel(al);
//
//	msg->clearX733();
//
//	if( objectName != NULL) 
//     msg->setAlarmObjectName(objectName);
//
//	msg->setAlarmType(alarmType);
//	msg->setProbableCause(probableCause);
//
//	if(specificProblem != NULL) 
//     msg->setSpecificProblem(specificProblem);
//
//	// INIT will coredump if additionalText is NULL
//	if(additionalText != NULL) 
//     msg->setAdditionalText(additionalText);
//
//
//	if(isIntLog)
//	{
//		// this will spool both incident and PRM
//    msg->intSpool();
//	}
//	else
//	{
//		// this is PRM only
//		msg->prmSpool();
//	}
//
//	msg->setAlarmLevel(alarmLevel);
//	msg->setClass(OMclass);
//	msg->setState();
//
//	mutex_unlock(&CRlockVarible);
//}
