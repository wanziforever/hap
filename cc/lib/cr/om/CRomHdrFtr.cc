/*
**      File ID:        @(#): <MID27769 () - 08/17/02, 26.1.1.1>
**
**	File:					MID27769
**	Release:				26.1.1.1
**	Date:					08/21/02
**	Time:					19:39:56
**	Newest applied delta:	08/17/02
**
**
**
** DESCRIPTION:
** 	This file contains functions to generate the ouput 
** 	message header and footer. The example of OM header
**	and footers as following;
**
**	+++SCP1 93-06-08 12:00:00 MAINT /CR001 #000999 SEG#1>
**	   body of OM
**	END OF REPORT #000999++-
**
**
** OWNER: 
**	Stephen J. W. Shiou
**
** NOTES:
**
*/

#include <stdio.h>
#include <stdlib.h>
#include "cc/hdr/cr/CRomHdrFtr.hh"
//#include "cc/cr/hdr/CRomClEnt.hh"


/* 
** The general function to generate OM header likes;
** +++office date time msgclass omdbkey #seq_num segment_num>
*/

char*
CRgenOMhdr(char* time_buf, char* msgclass, char* omdbkey, int seqnum, 
           char* segstr, char* machName, const char* machState)
{
	static char hdr_str[200];

#ifdef LX
	sprintf(hdr_str, "%s%s %s %s %s %s %s%.6d%s %s %s\n",
	       CRDEFSPACE,
	       CRDEFHDR,
	       (char*) CRswitchName.getName(),
	       (char*) time_buf,
	       (char*) msgclass,
	       (char*) omdbkey,
	       CRDEFSEQHDR,
	       seqnum,
	       (char*) segstr,
	       machName,
	       CRDEFHDREND);
#else
	sprintf(hdr_str, "%s%s %s %s %s %s %s%.6d%s %s %s %s\n",
	       CRDEFSPACE,
	       CRDEFHDR,
	       (char*) CRswitchName.getName(),
	       (char*) time_buf,
	       (char*) msgclass,
	       (char*) omdbkey,
	       CRDEFSEQHDR,
	       seqnum,
	       (char*) segstr,
	       machName,
	       machState,
	       CRDEFHDREND);
#endif

	return hdr_str;

}

/* 
** The general function to generate OM footer likes;
** END OF REPORT #seq_num++-
*/

char*
CRgenOMftr(int seqnum)
{
	static char ftr_str[200];

	sprintf(ftr_str, "\n%s%s %s%.6d%s\n",
		CRDEFSPACE,
		CRDEFRPT,
		CRDEFSEQHDR,
		seqnum,
		CRDEFFTR);
	return ftr_str;
}


