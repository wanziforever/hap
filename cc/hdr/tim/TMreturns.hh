#ifndef __TMRETURNS_H
#define __TMRETURNS_H

/*
**	File ID:	@(#): <MID10336 () - 08/17/02, 29.1.1.1>
**
**	File:					MID10336
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:35:45
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**	Definition of TMtimers class functions return constants
**
** OWNER:
**
** NOTES:
*/

#include "hdr/GLreturns.h"

const GLretVal TMERANGE = TM_FAIL;	/* Time/timer arg out of range */
const GLretVal TMUNINIT = (TM_FAIL-1);	/* Tmr library not initialized */
const GLretVal TMENOTCB = (TM_FAIL-2);	/* No TCBs available */
const GLretVal TMINVINIT = (TM_FAIL-3);	/* Tmr. init requested after */
					/* timers were initialized */
const GLretVal TMATCHFAIL = (TM_FAIL-4); /*Failed to attach to shared memory */
					/* seg. for absolute timer offset */
const GLretVal TMNOTALOC = (TM_FAIL-5); /* attempt to clear an unused tmr */
const GLretVal TMNOTIME  = (TM_FAIL-6); /* attempt to stop time...twice! */
const GLretVal TMTIMEON  = (TM_FAIL-7); /* attempt to start time while */
					/* it's already running	*/
const GLretVal TMNOTSHORT = (TM_FAIL-8);/* call to "tmrExp()" w/a U_short */
					/* timer tag but the expired timer's */
					/* tag does not fit in a short	*/
const GLretVal TMAUNINIT = (TM_FAIL-9); /* Absolute timers are not	*/
					/* initialized */

const GLretVal TMNOMEM = (TM_FAIL-10);	/* Failed to malloc timer memory	*/
const GLretVal TMTOOMANY = (TM_FAIL-11);/* Too many timers requested		*/

const GLretVal TMHIGHINTERR = (TM_FAIL-30); /* Lower bound for int. errors*/
const GLretVal TMINTERR0 = (TMHIGHINTERR);
const GLretVal TMINTERR1 = (TMHIGHINTERR-1);
const GLretVal TMINTERR2 = (TMHIGHINTERR-2);
const GLretVal TMINTERR3 = (TMHIGHINTERR-3);
const GLretVal TMINTERR4 = (TMHIGHINTERR-4);
const GLretVal TMINTERR5 = (TMHIGHINTERR-5);
const GLretVal TMINTERR6 = (TMHIGHINTERR-6);
const GLretVal TMINTERR7 = (TMHIGHINTERR-7);
const GLretVal TMINTERR8 = (TMHIGHINTERR-8);
const GLretVal TMINTERR9 = (TMHIGHINTERR-9);
const GLretVal TMINTERR10 = (TMHIGHINTERR-10);
const GLretVal TMINTERR11 = (TMHIGHINTERR-11);
const GLretVal TMINTERR12 = (TMHIGHINTERR-12);
const GLretVal TMINTERR13 = (TMHIGHINTERR-13);
const GLretVal TMINTERR14 = (TMHIGHINTERR-14);
const GLretVal TMINTERR15 = (TMHIGHINTERR-15);
const GLretVal TMINTERR16 = (TMHIGHINTERR-16);
const GLretVal TMINTERR17 = (TMHIGHINTERR-17);
const GLretVal TMINTERR18 = (TMHIGHINTERR-18);
const GLretVal TMINTERR19 = (TMHIGHINTERR-19);
const GLretVal TMINTERR20 = (TMHIGHINTERR-20);
const GLretVal TMINTERR21 = (TMHIGHINTERR-21);
const GLretVal TMINTERR22 = (TMHIGHINTERR-22);
const GLretVal TMINTERR23 = (TMHIGHINTERR-23);
const GLretVal TMINTERR24 = (TMHIGHINTERR-24);
const GLretVal TMINTERR25 = (TMHIGHINTERR-25);
const GLretVal TMINTERR26 = (TMHIGHINTERR-26);
const GLretVal TMINTERR27 = (TMHIGHINTERR-27);
const GLretVal TMINTERR28 = (TMHIGHINTERR-28);
const GLretVal TMINTERR29 = (TMHIGHINTERR-29);
const GLretVal TMINTERR30 = (TMHIGHINTERR-30);
const GLretVal TMINTERR31 = (TMHIGHINTERR-31);
const GLretVal TMINTERR32 = (TMHIGHINTERR-32);
const GLretVal TMINTERR33 = (TMHIGHINTERR-33);

/*
** NOTE: when adding internal error codes, the upper bound for internal
**	 errors must be adjusted as well!
*/
const GLretVal TMLOINTERR = TMINTERR33;	/* Upper bound on int errors */

/*
** The following macro returns "TRUE" if the error code passed it is
** an internal timer error and "FALSE" otherwise.
*/
#define	TMINTERR(tmerror)	\
	(((tmerror >= TMLOINTERR) && (tmerror <= TMHIGHINTERR)) ? \
		TRUE : FALSE)
#endif
