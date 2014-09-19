/*
**      File ID:        @(#): <MID18627 () - 08/17/02, 29.1.1.1>
**
**	File:					MID18627
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:39:50
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**	ALARM process - Routines to manipulate/send the MSGH object
**	to ALARM.
**
** OWNER: 
**	N. Scott Abderhalden
**
** NOTES:
**
*/

#include <string.h>
#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "cc/hdr/msgh/MHmsgBase.H"
#include "cc/hdr/msgh/MHinfoExt.H"
#include "cc/hdr/cr/CRmtype.H"
#include "cc/cr/hdr/CRalarmMsg.H"
#include "cc/hdr/cr/CRalarmLevel.H"
#include "cc/hdr/cr/CRindStatus.H"

static
const char*
CRalarmIndName(CRALARMLVL lvl)
{
	const char* indName = NULL;

	switch (lvl)
	{
	    case POA_CRIT:
		indName = "CRomCR";
		break;
	    case POA_MAJ:
		indName = "CRomMJ";
		break;
	    case POA_MIN :
		indName = "CRomMN";
		break;
	}
	return indName;
}

static
const char*
CRalarmValue(CRALARMLVL lvl)
{
	const char* val = NULL;

	switch (lvl)
	{
	    case POA_CRIT:
		val = "CRITICALOM";
		break;
	    case POA_MAJ:
		val = "MAJOROM";
		break;
	    case POA_MIN :
		val = "MINOROM";
		break;
	}
	return val;
}

//	Name:	CRalarmMsg::CRalarmMsg()
//	Function:
//		Constructor: Set default or null values for the class.

CRalarmMsg::CRalarmMsg()
{
	priType	= 1L ;
	msgType = CRalarmTyp ;
	alarm_msg_qid = MHnullQ ;
	action = CR_NO_ACTION ;
}

	// Do we need to know the name of the sending queue?

//	Name::CRalarmMsg::send
//	Function:
//		Send the CRalarmMsg-type message to ALARM for further
//		action.
//	Called By:
//		soundAlarm, cancelAlarm, muteAlarm, reportAlarmStatus

GLretVal
CRalarmMsg::send()
{
	Short		msg_size ;
	GLretVal	ret_val ;

        if ( alarm_msg_qid == MHnullQ ) {
                if ( MHmsgh.getMhqid(ALM_PROC_NAME, alarm_msg_qid) != GLsuccess ) {
                        alarm_msg_qid = MHnullQ ;
                        ret_val = GLfail ;
                }
        }

        msg_size = CRalarmMsgSz ;
        ret_val = MHmsgBase::send( alarm_msg_qid, MHnullQ, msg_size, 100 ) ;

        return ret_val ;

} /* send */
 
//	Name:	soundAlarm
//	Function:
//		Send an "sound alarm" message to ALARM of the
//		priority "lvl", where "lvl" is the alarm level
//		used by CSOP. See "cc/hdr/cr/CRalarmLevel.H.
//	Called By:
//		Any process/CEP wishing to activate an alarm.

GLretVal
CRalarmMsg::soundAlarm( CRALARMLVL lvl )
{
	action = CR_ACTIVATE ;
	level = lvl ;
	
	/* Tell SYSTAT to turn the appropriate indicator on */
	CRindStatusMsg indMsg;
	indMsg.add(CRalarmIndName(lvl), "NORMAL");
	indMsg.send(CRalarmIndName(lvl), CRalarmValue(lvl));

	return GLsuccess;
}

//	Name:	cancelAlarm
//	Function:
//		Send an "cancel alarm" message to ALARM of the
//		priority "lvl", where "lvl" is the alarm level
//		used by CSOP. See "cc/hdr/cr/CRalarmLevel.H.
//	Called By:
//		Any process/CEP wishing to cancel an active alarm.

GLretVal
CRalarmMsg::cancelAlarm( CRALARMLVL lvl )
{
	action = CR_RETIRE ;
	level = lvl ;

	/* Tell SYSTAT to change the appropriate indicator to NORMAL */
	CRindStatusMsg indMsg;
	indMsg.send(CRalarmIndName(lvl), "NORMAL");

	return GLsuccess;
}

//	Name:	cancelAlarms
//	Function:
//		Send an "cancel alarms" message to ALARM 
//		so that all alarm levels will be retired.
//	Called By:
//		Any process/CEP wishing to cancel an active alarm.

GLretVal
CRalarmMsg::cancelAlarms( )
{
	action = CR_RETIRE_ALL ;
	return send() ;
}

//	Name:	muteAlarm
//	Function:
//		Send an "mute alarm" message to ALARM of the
//		priority "lvl", where "lvl" is the alarm level
//		used by CSOP. See "cc/hdr/cr/CRalarmLevel.H.
//	Called By:
//		Any process wishing to jerk ALARM's chain.

GLretVal
CRalarmMsg::muteAlarm( CRALARMLVL lvl )
{
	action = CR_MUTE ;
	level = lvl ;
	return send() ;
}

//	Name:	muteAlarms
//	Function:
//		Send an "mute alarms" message to ALARM so that
//		all alarm levels will be muted.  Designed for
//		used by the LMT process.
//	Called By:
//		Any process wishing to jerk ALARM's chain.

GLretVal
CRalarmMsg::muteAlarms( )
{
	action = CR_MUTE_ALL ;
	return send() ;
}

//	Name:	alarm_status
//	Function:
//		Send a message to the ALARM process and request that
//		a report be initiated of the status of the alarms.
//		Normally report on off-normal state, but if the
//		"all_flag" is TRUE, report on the alarms regardless of
//		its state.

GLretVal
CRalarmMsg::reportAlarmStatus( Short all_flag )
{
	if ( all_flag == TRUE )
		action = CR_REPORT_ALL_ALARMS ;
	else	action = CR_REPORT_ACTIVE_ALARMS ;

	return send() ;
}

//	Name:	link_originating_terminal
//	Function:
//		Add CRomInfo object to alarm message.
//
void
CRalarmMsg::link_originating_terminal(CRcmdLine &cmdline)
{
	omInfo = *cmdline.getOMinfo();
}

//	Name: getOMinfo
//	Function:
//		return pointer to CRomInfo object.

CRomInfo*
CRalarmMsg::getOMinfo()
{
	return &omInfo;
}

//	Name:	get_alarm_action
//	Function:
//		Return the action attached to the CRalarmMsg message.
//		I.e. For what reason was the message sent?
//	Called By:
//		process_message (CRalarm.C)

Short
CRalarmMsg::get_alarm_action()
{
	return action ;
}

//	Name:	get_alarm_level
//	Function:
//		Return the alarm level affected by the CRalarmMsg
//		message.
//	Called By:
//		process_message (CRalarm.C)

CRALARMLVL
CRalarmMsg::get_alarm_level()
{
	return level ;
}
