#ifndef _CRTMSTAMP_H
#define _CRTMSTAMP_H

/*
**      File ID:        @(#): <MID6954 () - 09/09/02, 29.1.1.1>
**
**	File:					MID6954
**	Release:				29.1.1.1
**	Date:					09/12/02
**	Time:					10:38:46
**	Newest applied delta:	09/09/02
**
** DESCRIPTION:
**	This file defines the interface(s) to the function(s)
**	that are used to format the date and time for the
**	ULSI subsystem.
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/


#include	<String.h>
#include	"hdr/GLreturns.h"

static const int	timebufLen = 80;

enum fields {
	MONTH,
	DAY,
	YEAR2,							/** Two digit years **/
	YEAR4,							/** Four digit years **/
	NUMFIELDS
};

extern void CRgetCurTime(char timebuf[]);
extern void CRformatTime(long time, char timebuf[]);
extern Bool CRformatDate(long time, const char* fmt, char timebuf[]);
extern std::string CRgetFormat();
extern int	CRgetPosition(fields f);
extern void CRstripNl(char* str);
extern GLretVal	CRchgDateStrToTime(const std::string& dateStr, class Time& datev,
                                   std::string& ymd);

#endif
