#ifndef __DBI_H
#define __DBI_H

/*
**
**	File ID:	@(#): <MID7011 () - 08/17/02, 29.1.1.1>
**
**	File:					MID7011
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:21:43
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**	  Define constant variables shared by DBI and DBI helpers
**
** OWNER:
**	eDB team
**
** History:
**	Lucyliang: Added for feature 61284
**
** NOTES:
*/

#include "hdr/GLtypes.h"

//max peg count 
#define DBI_MAXERRPEG	6

// Constants for timer
const Long DBItmrMax = 5L;          // maximum number of timers
const Long DBIsanityTmrTyp = 1L;    // timer for sanity pegging
const Long DBIsanityTime = 45L;     // 45 seconds

#endif

