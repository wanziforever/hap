/*
**      File ID:        @(#): <MID17161 () - 09/06/02, 29.1.1.1>
**
**	File:					MID17161
**	Release:				29.1.1.1
**	Date:					09/12/02
**	Time:					10:38:31
**	Newest applied delta:	09/06/02
**
** DESCRIPTION:
**	ALARM process - MSGH Interface to ALARM.
**
** OWNER: 
**	N. Scott Abderhalden
**
** NOTES:
**
*/

#ifndef __CRALARMMSG_H
#define __CRALARMMSG_H

#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
//#include "cc/hdr/cr/CRcmdLine.hh"
#include "cc/hdr/cr/CRomInfo.hh"
#include "cc/hdr/cr/CRalarmLevel.hh"

#define	ALM_PROC_NAME "ALARM"

		// The following are values of the  "action" element of
		// the CRalarmMsg class.
const Short CR_NO_ACTION		= 0 ;
const Short CR_ACTIVATE			= 1 ;
const Short CR_RETIRE			= 2 ;
const Short CR_MUTE			= 3 ;
const Short CR_REPORT_ALL_ALARMS	= 4 ;
const Short CR_REPORT_ACTIVE_ALARMS	= 5 ;
const Short CR_MUTE_ALL			= 6 ;
const Short CR_RETIRE_ALL		= 7 ;

class CRalarmMsg : private MHmsgBase
{
public:
	CRalarmMsg() ;
	GLretVal	soundAlarm(  CRALARMLVL ) ; // send msg to ALARM
	GLretVal	cancelAlarm( CRALARMLVL ) ; // send msg to ALARM
	GLretVal	muteAlarm(   CRALARMLVL ) ; // send msg to ALARM
	GLretVal	reportAlarmStatus( Short ) ;// send msg to ALARM
	GLretVal	muteAlarms( );	    	    // send msg to ALARM
	GLretVal	cancelAlarms( );	    // send msg to ALARM

	Short		get_alarm_action( Void ) ;// ALARM uses to interpret msg
	CRALARMLVL	get_alarm_level( Void ) ; // ALARM uses to interpret msg
	//void		link_originating_terminal(CRcmdLine  &cmdline);
	CRomInfo*	getOMinfo();

private:
	GLretVal	send() ;	// send msg to ALARM

private:
	CRALARMLVL	level ;		// One of POA_??? vals
					//  from cc/hdr/cr/CRalarmLevel.hh

	Short	action ;		// One of CR_ACTIVATE
					//	  CR_RETIRE
					//        CR_MUTE
					//	  CR_NO_ACTION
					//	  CR_REPORT_ACTIVE_ALARMS
					//	  CR_REPORT_ALL_ALARMS
					//	  CR_MUTE_ALL
					//	  CR_RETIRE_ALL
	MHqid	alarm_msg_qid ;
	CRomInfo	omInfo;
} ;

const int CRalarmMsgSz = sizeof( CRalarmMsg ) ;

#endif
