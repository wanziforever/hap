#ifndef __CROMHDRFTR_H
#define __CROMHDRFTR_H

/*
**      File ID:        @(#): <MID28489 () - 08/17/02, 26.1.1.1>
**
**	File:					MID28489
**	Release:				26.1.1.1
**	Date:					08/21/02
**	Time:					19:15:50
**	Newest applied delta:	08/17/02
**
**
**
** DESCRIPTION:
** 	This file contains the definition of the functions 
**	to generate the ouput message header and footer. 
**	The example of OM header and footers as following;
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


#include "cc/hdr/cr/CRofficeNm.hh"


/*
** The following defines are all for the 
** OM header and footer.
*/
static	const 	char*	CRDEFSPACE = "   ";
static	const	char*	CRDEFHDR = "+++";
static	const 	char*	CRDEFSEQHDR = "#";
static	const 	char*	CRDEFHDREND = ">";
static	const 	char*	CRDEFFTR = "++-";
static	const 	char*	CRDEFRPT = "END OF REPORT";
static	const 	char*	CRDEFOMDBKEY = "      ";
static	const 	char*	CRDEFSEGMENT = "";

/*
** The sequence number 999999 is to be used as
** a special indicator to indicate that the OM
** has been log locally. The maximun sequence 
** number per msg class can only be 999998.
*/

static	const	int	CRDEFSEQNUM = 999999;
static	const	int	CRMAXSEQNUM = 999998;

/*
** The return value the char string from the 
** CRgenOMhdr and CRgenOMftr function calls
*/

//static	char*	hdr_str;
//static	char*	ftr_str;

/*
** The declaration of CRgenOMhdr and CRgenOMftr, since these two 
** functions are going to be used for both CRmsg and CRomdbMsg 
** following default value may pass in as argument like;
**
** 1.CRmsg has no omdbkey so when in CRmsg, CRDEFOMDBKEY a six 
**   blanks string will be passed in for argument omdbkey.
** 2.CRomdbMsg has no implementation of segmentation of a very 
**   large output, so CRDEFSEGMENT a NULL char string will be 
**   passed in for argument segstr.
** 3.In case the CSOP die, special sequence number 999999 will 
**   be the indicator, so the maximun value the sequence 
**   number can be is 999998 ( the mininum is 1 ) per msg class.
*/


extern	char*	CRgenOMhdr(char* time_buf, char* msgclass, char* omdbkey, 
                         int seqnum, char* segstr, char *machName,
                         const char *machState);

extern	char*	CRgenOMftr(int seqnum);
#endif
