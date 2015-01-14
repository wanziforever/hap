/*
**      File ID:        @(#): <MID20039 () - 08/17/02, 29.1.1.1>
**
**	File:					MID20039
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:39:55
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**      This file defines a USLI class that is used to generate and
**	send output messages (OM) to the spooler process (CSOP)
**      using the Output Message Data Base (OMDB)
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/
#include <string.h>
#include "cc/hdr/cr/CRdebugMsg.hh"
#include "cc/hdr/cr/CRalarmLevel.hh"

static const char *CRPOA_to_string[] = {
	"*C ", "   ", "** ", "*  ", "CL ", "M  ", "A  ", "   ", "TC ", "TJ ", "TN "
};

const char*
CRPOA_to_str(CRALARMLVL alarmIndex)
{
	if (alarmIndex < 1 ||
	    alarmIndex > sizeof(CRPOA_to_string)/sizeof(const char*))
	{
		CRERROR("alarmIndex (%d) out of range", alarmIndex);
		alarmIndex = POA_DEFAULT;
	}

	return CRPOA_to_string[alarmIndex-1];
}

CRALARMLVL
CRstr_to_POA(const char* alarmString)
{
	int numEntries = sizeof(CRPOA_to_string)/sizeof(const char*);
	for (int i = 0; i < numEntries; i++)
	{
		if (strncmp(alarmString, CRPOA_to_string[i],
			    CRALARMSTRMAX-1) == 0)
			return i + 1;
	}
//	CRERROR("alarmString (%s) not found in table", alarmString);
	return POA_DEFAULT;
}

